#include "board.h"
#include "evalparams.h"
#include "evalsearch.h"
#include "evaltuner.h"
#include "magic.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

#define MAX_TRAINING 10000000
#define MAX_ITERATIONS 1000
#define LEARNING_RATE 100.0
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
        // Find the opening '[' for the WDL score
        char* bracket = strchr(line, '[');
        if (!bracket) continue;

        // Null-terminate the FEN and parse the WDL
        *bracket = '\0';
        double wdl = atof(bracket + 1);

        // Strip trailing whitespace from FEN
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

static double compute_gradient_norm(const double* gradient) {
    double norm = 0.0;
    for (int i = 0; i < NUM_EVAL_PARAMS; i++) {
        norm += gradient[i] * gradient[i];
    }
    return sqrt(norm);
}

EvalResult evaluate_with_features(const Position* pos, const EvalParamsDouble* params, const MagicData* magic) {
    EvalResult result;
    result.score = 0.0;
    result.num_features = 0;

    double mg = 0.0, eg = 0.0;
    int phase = 0;

    for (int sq = 0; sq < 64; sq++) {
        int piece = get_piece_on_square(pos, sq);
        if (piece == -1) continue;

        int side = (piece < 6) ? WHITE : BLACK;
        int type = piece % 6;
        int mirrored_sq = (side == WHITE) ? sq : mirror(sq);

        // Get parameter values and their indices
        double mg_val = params->mg_value[type];
        double eg_val = params->eg_value[type];

        int idx_mg_val = IDX_MG_VALUE + type;
        int idx_eg_val = IDX_EG_VALUE + type;

        // Get PST values and indices
        double mg_pst = 0.0, eg_pst = 0.0;
        int idx_mg_pst = -1, idx_eg_pst = -1;

        switch (type) {
            case P:
                mg_pst = params->pawn_pst_mg[mirrored_sq];
                eg_pst = params->pawn_pst_eg[mirrored_sq];
                idx_mg_pst = IDX_PAWN_PST_MG + mirrored_sq;
                idx_eg_pst = IDX_PAWN_PST_EG + mirrored_sq;
                break;
            case N:
                mg_pst = params->knight_pst_mg[mirrored_sq];
                eg_pst = params->knight_pst_eg[mirrored_sq];
                idx_mg_pst = IDX_KNIGHT_PST_MG + mirrored_sq;
                idx_eg_pst = IDX_KNIGHT_PST_EG + mirrored_sq;
                phase += 1;
                break;
            case B:
                mg_pst = params->bishop_pst_mg[mirrored_sq];
                eg_pst = params->bishop_pst_eg[mirrored_sq];
                idx_mg_pst = IDX_BISHOP_PST_MG + mirrored_sq;
                idx_eg_pst = IDX_BISHOP_PST_EG + mirrored_sq;
                phase += 1;
                break;
            case R:
                mg_pst = params->rook_pst_mg[mirrored_sq];
                eg_pst = params->rook_pst_eg[mirrored_sq];
                idx_mg_pst = IDX_ROOK_PST_MG + mirrored_sq;
                idx_eg_pst = IDX_ROOK_PST_EG + mirrored_sq;
                phase += 2;
                break;
            case Q:
                mg_pst = params->queen_pst_mg[mirrored_sq];
                eg_pst = params->queen_pst_eg[mirrored_sq];
                idx_mg_pst = IDX_QUEEN_PST_MG + mirrored_sq;
                idx_eg_pst = IDX_QUEEN_PST_EG + mirrored_sq;
                phase += 4;
                break;
            case K:
                mg_pst = params->king_pst_mg[mirrored_sq];
                eg_pst = params->king_pst_eg[mirrored_sq];
                idx_mg_pst = IDX_KING_PST_MG + mirrored_sq;
                idx_eg_pst = IDX_KING_PST_EG + mirrored_sq;
                break;
        }

        // Calculate total piece value
        double mg_score = mg_val + mg_pst;
        double eg_score = eg_val + eg_pst;

        // Apply to running totals
        double sign = (side == WHITE) ? 1.0 : -1.0;
        mg += sign * mg_score;
        eg += sign * eg_score;


        // Add feature contributions with proper weights
        if (result.num_features + 4 < MAX_FEATURES_PER_POSITION) {
            result.features[result.num_features++] = (FeatureContribution){idx_mg_val, sign};
            result.features[result.num_features++] = (FeatureContribution){idx_eg_val, sign};
            result.features[result.num_features++] = (FeatureContribution){idx_mg_pst, sign};
            result.features[result.num_features++] = (FeatureContribution){idx_eg_pst, sign};
        }
    }

    // Apply phase interpolation
    if (phase > 24) phase = 24;
    double mg_weight = (double)phase / 24.0;
    double eg_weight = (24.0 - (double)phase) / 24.0;

    // Apply phase weights to features
    for (int i = 0; i < result.num_features; i++) {
        int idx = result.features[i].index;
        
        // Determine if this is a middlegame or endgame parameter
        bool is_mg = false;
        if (idx >= IDX_MG_VALUE && idx < IDX_EG_VALUE) {
            is_mg = true;
        } else if (idx >= IDX_EG_VALUE && idx < IDX_PAWN_PST_MG) {
            is_mg = false;
        } else {
            // For PST parameters, check the range
            is_mg = (idx >= IDX_PAWN_PST_MG && idx < IDX_PAWN_PST_EG) ||
                    (idx >= IDX_KNIGHT_PST_MG && idx < IDX_KNIGHT_PST_EG) ||
                    (idx >= IDX_BISHOP_PST_MG && idx < IDX_BISHOP_PST_EG) ||
                    (idx >= IDX_ROOK_PST_MG && idx < IDX_ROOK_PST_EG) ||
                    (idx >= IDX_QUEEN_PST_MG && idx < IDX_QUEEN_PST_EG) ||
                    (idx >= IDX_KING_PST_MG && idx < IDX_KING_PST_EG);
        }
        result.features[i].weight *= is_mg ? mg_weight : eg_weight;
    }

    result.score = mg_weight * mg + eg_weight * eg;
    return result;
}

double find_best_k(const EvalParamsDouble* params, const TrainingEntry* data, int n, const MagicData* magic) {
    double best_k = K_START;
    double best_loss = 1e9;

    for (double k = K_START; k <= K_END; k += K_STEP) {
        double loss = compute_loss(params, data, n, k, magic);
        printf("Try k = %.4f -> Loss = %.6f\n", k, loss);
        if (loss < best_loss) {
            best_loss = loss;
            best_k = k;
        }
    }
    printf("Chosen k = %.4f\n", best_k);
    return best_k;
}

double sigmoid(double x, double k) {
    return 1.0 / (1.0 + exp(-k * x));
}

double sigmoid_derivative(double x, double k) {
    double s = sigmoid(x, k);
    return k * s * (1.0 - s);
}

double compute_loss(const EvalParamsDouble* params, const TrainingEntry* data, int n, double k, const MagicData* magic) {
    double loss = 0.0;
    Position pos;

    for (int i = 0; i < n; i++) {
        init_position(&pos, data[i].fen);
        EvalResult r = evaluate_with_features(&pos, params, magic);
        double p = sigmoid(r.score, k);
        double e = p - data[i].wdl;
        loss += e * e;
    }

    printf("Total loss: %.6f | Average loss: %.6f\n", loss, loss / n);
    return loss / n;
}

void convert_params_to_integer(const EvalParamsDouble* in, EvalParams* out) {
    for (int i = 0; i < 6; i++) {
        out->mg_value[i] = (int)round(in->mg_value[i]);
        out->eg_value[i] = (int)round(in->eg_value[i]);
    }

    for (int i = 0; i < 64; i++) {
        out->pawn_pst_mg[i] = (int)round(in->pawn_pst_mg[i]);
        out->pawn_pst_eg[i] = (int)round(in->pawn_pst_eg[i]);
        out->knight_pst_mg[i] = (int)round(in->knight_pst_mg[i]);
        out->knight_pst_eg[i] = (int)round(in->knight_pst_eg[i]);
        out->bishop_pst_mg[i] = (int)round(in->bishop_pst_mg[i]);
        out->bishop_pst_eg[i] = (int)round(in->bishop_pst_eg[i]);
        out->rook_pst_mg[i] = (int)round(in->rook_pst_mg[i]);
        out->rook_pst_eg[i] = (int)round(in->rook_pst_eg[i]);
        out->queen_pst_mg[i] = (int)round(in->queen_pst_mg[i]);
        out->queen_pst_eg[i] = (int)round(in->queen_pst_eg[i]);
        out->king_pst_mg[i] = (int)round(in->king_pst_mg[i]);
        out->king_pst_eg[i] = (int)round(in->king_pst_eg[i]);
    }
}

void save_evalparams_text(const char* path, const EvalParams* p) {
    FILE* f = fopen(path, "w");
    if (!f) {
        fprintf(stderr, "Failed to write %s\n", path);
        return;
    }

    fprintf(f, "// Generated tuned evaluation parameters\n");
    fprintf(f, "#include \"evalparams.h\"\n\n");
    fprintf(f, "EvalParams tuned_params = {\n");

    // mg_value and eg_value
    fprintf(f, "  .mg_value = {");
    for (int i = 0; i < 6; i++) fprintf(f, "%d%s", p->mg_value[i], (i < 5 ? ", " : ""));
    fprintf(f, "},\n");

    fprintf(f, "  .eg_value = {");
    for (int i = 0; i < 6; i++) fprintf(f, "%d%s", p->eg_value[i], (i < 5 ? ", " : ""));
    fprintf(f, "},\n");

    // PSTs
    const char* names[] = {
        "pawn_pst_mg", "pawn_pst_eg",
        "knight_pst_mg", "knight_pst_eg",
        "bishop_pst_mg", "bishop_pst_eg",
        "rook_pst_mg", "rook_pst_eg",
        "queen_pst_mg", "queen_pst_eg",
        "king_pst_mg", "king_pst_eg"
    };

    const int* arrays[] = {
        p->pawn_pst_mg, p->pawn_pst_eg,
        p->knight_pst_mg, p->knight_pst_eg,
        p->bishop_pst_mg, p->bishop_pst_eg,
        p->rook_pst_mg, p->rook_pst_eg,
        p->queen_pst_mg, p->queen_pst_eg,
        p->king_pst_mg, p->king_pst_eg
    };

    for (int a = 0; a < 12; a++) {
        fprintf(f, "  .%s = {", names[a]);
        for (int i = 0; i < 64; i++) {
            fprintf(f, "%d%s", arrays[a][i], (i < 63 ? ", " : ""));
        }
        fprintf(f, "},\n");
    }

    fprintf(f, "};\n");
    fclose(f);
    printf("Saved text params to %s\n", path);
}

void run_minibatch_training(
    EvalParamsDouble* params,
    const TrainingEntry* data,
    int num_entries,
    const MagicData* magic,
    int batch_size,
    double learning_rate,
    int iterations,
    double sigmoid_k,
    const char* output_file
) {
    double* gradient = calloc(NUM_EVAL_PARAMS, sizeof(double));
    if (!gradient) return;

    Position pos;
    
    // Start with a more conservative learning rate
    double current_lr = learning_rate;
    double prev_loss = 1e9;

    for (int iter = 0; iter < iterations; iter++) {
        memset(gradient, 0, sizeof(double) * NUM_EVAL_PARAMS);

        // Compute gradient over minibatch
        for (int i = 0; i < batch_size; i++) {
            const TrainingEntry* entry = &data[i];

            init_position(&pos, entry->fen);
            EvalResult result = evaluate_with_features(&pos, params, magic);

            float i = 0.0;
            
            // Score from white's perspective
            double white_score = result.score;
            double predicted = sigmoid(white_score, sigmoid_k);
            double error = predicted - entry->wdl;
            double dloss_dscore = 2.0 * error * sigmoid_derivative(white_score, sigmoid_k);

            // Accumulate gradients
            for (int j = 0; j < result.num_features; j++) {
                int idx = result.features[j].index;
                double contribution = result.features[j].weight;
                gradient[idx] += dloss_dscore * contribution;
            }
        }

        // Normalize by batch size
        for (int i = 0; i < NUM_EVAL_PARAMS; i++) {
            gradient[i] /= batch_size;
        }

        // Apply gradient clipping to prevent exploding gradients
        double grad_norm = compute_gradient_norm(gradient);
        if (grad_norm > 1.0) {
            double scale = 1.0 / grad_norm;
            for (int i = 0; i < NUM_EVAL_PARAMS; i++) {
                gradient[i] *= scale;
            }
        }

        // Save current parameters
        EvalParamsDouble backup = *params;

        // Apply gradient descent update
        for (int i = 0; i < 6; i++) {
            params->mg_value[i] -= current_lr * gradient[IDX_MG_VALUE + i];
            params->eg_value[i] -= current_lr * gradient[IDX_EG_VALUE + i];
        }

        #define APPLY_PST_UPDATE(field, index_base) \
            for (int i = 0; i < 64; i++) params->field[i] -= current_lr * gradient[index_base + i];

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

        // Check if loss improved
        double new_loss = compute_loss(params, data, batch_size, sigmoid_k, magic);
        
        if (new_loss < prev_loss) {
            // Good update, slightly increase learning rate
            current_lr *= 1.1;
            prev_loss = new_loss;
            printf("Iter %d: Loss = %.6f, LR = %.6f (improved)\n", iter + 1, new_loss, current_lr);
        } else {
            // Bad update, revert and reduce learning rate
            *params = backup;
            current_lr *= 0.5;
            printf("Iter %d: Loss = %.6f, LR = %.6f (reverted)\n", iter + 1, new_loss, current_lr);
        }

        // Early stopping conditions
        if (current_lr < 1e-6) {
            printf("Learning rate too small, stopping.\n");
            break;
        }   


        // save params

        EvalParams final;
        convert_params_to_integer(params, &final);

        char text_path[256], bin_path[256];
        snprintf(text_path, sizeof(text_path), "%s.c", output_file);
        snprintf(bin_path, sizeof(bin_path), "%s.bin", output_file);

        save_evalparams_text(text_path, &final);
    }

    free(gradient);
}

void run_tuner_main(const char* dataset_path, const char* output_prefix, const MagicData* magic) {
    int num_entries = load_dataset(dataset_path, training_data, MAX_TRAINING);
    if (num_entries <= 0) {
        fprintf(stderr, "Failed to load dataset from %s\n", dataset_path);
        return;
    }

    printf("Loaded %d training positions.\n", num_entries);

    EvalParamsDouble params;
    init_double_params(&params);


    double k = find_best_k(&params, training_data, num_entries, magic);

    printf("Before training: mg_value[0] = %.3f\n", params.mg_value[0]);
    
    run_minibatch_training(
        &params,
        training_data,
        num_entries,
        magic,
        num_entries,
        LEARNING_RATE,
        MAX_ITERATIONS,
        k,
        output_prefix
    );
    
    printf("After training:  mg_value[0] = %.3f\n", params.mg_value[0]);
}