#include "board.h"
#include "evalparams.h"
#include "evalsearch.h"
#include "evaltuner.h"
#include "magic.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

#define MAX_TRAINING 100000
#define MAX_ITERATIONS 100
#define BATCH_SIZE 100000
#define LEARNING_RATE 1000000.0
#define K_START 0.0005
#define K_END 0.01
#define K_STEP 0.0005
TrainingEntry training_data[MAX_TRAINING];

int load_dataset(const char* path, TrainingEntry* entries, int max_entries) {
    FILE* file = fopen(path, "r");
    if (!file) {
        fprintf(stderr, "Failed to open dataset file: %s\n", path);
        return 0;
    }

    int count = 0;
    char line[256];

    while (fgets(line, sizeof(line), file) && count < max_entries) {
        char* bracket = strchr(line, '[');
        if (!bracket) continue;

        *bracket = '\0';
        double wdl = atof(bracket + 1);

        char* newline = strchr(line, '\n');
        if (newline) *newline = '\0';

        strncpy(entries[count].fen, line, sizeof(entries[count].fen) - 1);
        entries[count].fen[sizeof(entries[count].fen) - 1] = '\0';
        entries[count].wdl = wdl;

        count++;
    }

    fclose(file);
    return count;
}

// Utility to compute L2 norm of the gradient for debug purposes
static double compute_gradient_norm(const double* gradient) {
    double norm = 0.0;
    for (int i = 0; i < NUM_EVAL_PARAMS; i++) {
        norm += gradient[i] * gradient[i];
    }
    return sqrt(norm);
}

void run_minibatch_training(
    EvalParamsDouble* params,
    const TrainingEntry* data,
    int num_entries,
    const MagicData* magic,
    int batch_size,
    double learning_rate,
    int iterations,
    double sigmoid_k
) {
    double* gradient = calloc(NUM_EVAL_PARAMS, sizeof(double));
    if (!gradient) return;

    Position pos;

    for (int iter = 0; iter < iterations; iter++) {
        memset(gradient, 0, sizeof(double) * NUM_EVAL_PARAMS);

        for (int i = 0; i < batch_size; i++) {
            int index = rand() % num_entries;
            const TrainingEntry* entry = &data[index];

            init_position(&pos, entry->fen);

            EvalResult result = evaluate_with_features(&pos, params, magic);
            double flipped_score = (pos.side_to_move == WHITE) ? result.score : -result.score;
            double predicted = sigmoid(flipped_score, sigmoid_k);
            double error = predicted - entry->wdl;
            double dloss = 2.0 * error * sigmoid_derivative(flipped_score, sigmoid_k);

            for (int j = 0; j < result.num_features; j++) {
                int idx = result.features[j].index;
                double w = result.features[j].weight;
                gradient[idx] += dloss * w;
            }
        }

        for (int i = 0; i < NUM_EVAL_PARAMS; i++) {
            gradient[i] /= batch_size;
        }

        double grad_norm = compute_gradient_norm(gradient);
        printf("Iter %d: Grad norm = %.6f\n", iter + 1, grad_norm);

        double update_mg0 = learning_rate * gradient[IDX_MG_VALUE + 0];
        printf("Iter %d: LR = %.5f | mg_value[0] = %.4f | grad = %.6f | update = %.6f\n",
            iter + 1, learning_rate, params->mg_value[0], gradient[IDX_MG_VALUE + 0], update_mg0);

        EvalParamsDouble backup = *params;

        for (int i = 0; i < 6; i++) {
            params->mg_value[i]     -= learning_rate * gradient[IDX_MG_VALUE + i];
            params->eg_value[i]     -= learning_rate * gradient[IDX_EG_VALUE + i];
        }

        #define APPLY_PST_UPDATE(field, index_base) \
            for (int i = 0; i < 64; i++) params->field[i] -= learning_rate * gradient[index_base + i];

        APPLY_PST_UPDATE(pawn_pst_mg,   IDX_PAWN_PST_MG);
        APPLY_PST_UPDATE(pawn_pst_eg,   IDX_PAWN_PST_EG);
        APPLY_PST_UPDATE(knight_pst_mg, IDX_KNIGHT_PST_MG);
        APPLY_PST_UPDATE(knight_pst_eg, IDX_KNIGHT_PST_EG);
        APPLY_PST_UPDATE(bishop_pst_mg, IDX_BISHOP_PST_MG);
        APPLY_PST_UPDATE(bishop_pst_eg, IDX_BISHOP_PST_EG);
        APPLY_PST_UPDATE(rook_pst_mg,   IDX_ROOK_PST_MG);
        APPLY_PST_UPDATE(rook_pst_eg,   IDX_ROOK_PST_EG);
        APPLY_PST_UPDATE(queen_pst_mg,  IDX_QUEEN_PST_MG);
        APPLY_PST_UPDATE(queen_pst_eg,  IDX_QUEEN_PST_EG);
        APPLY_PST_UPDATE(king_pst_mg,   IDX_KING_PST_MG);
        APPLY_PST_UPDATE(king_pst_eg,   IDX_KING_PST_EG);

        double prev_loss = compute_loss(&backup, data, num_entries, sigmoid_k, magic);
        double new_loss  = compute_loss(params, data, num_entries, sigmoid_k, magic);

        if (new_loss > prev_loss) {
            *params = backup;
            learning_rate *= 0.5;
            printf("Iter %d: Loss increased (%.6f -> %.6f), reducing LR to %.5f\n", iter + 1, prev_loss, new_loss, learning_rate);
        } else {
            learning_rate *= 1.1;
            printf("Iter %d: Loss improved (%.6f -> %.6f), increasing LR to %.5f\n", iter + 1, prev_loss, new_loss, learning_rate);
        }

        if (learning_rate < 1e-3) {
            printf("Learning rate less than 0.001: stopping early.\n");
            break;
        }
    }

    free(gradient);
}
