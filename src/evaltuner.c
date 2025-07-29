#include "board.h"
#include "evaluation.h"
#include "evalparams.h"
#include "evalsearch.h"
#include "evaltuner.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

#define MAX_TRAINING 100000
#define MAX_ITERATIONS 1000
#define LEARNING_RATE 0.01
#define K_START 0.0005
#define K_END 0.01
#define K_STEP 0.0005
TrainingEntry training_data[MAX_TRAINING];

const int tropism_feature_idx[6][2] = {
    [N] = {IDX_TROPISM_KNIGHT_MG, IDX_TROPISM_KNIGHT_EG},
    [B] = {IDX_TROPISM_BISHOP_MG, IDX_TROPISM_BISHOP_EG},
    [R] = {IDX_TROPISM_ROOK_MG,   IDX_TROPISM_ROOK_EG},
    [Q] = {IDX_TROPISM_QUEEN_MG,  IDX_TROPISM_QUEEN_EG}
};

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

EvalResult evaluate_with_features(const Position* pos, const EvalParamsDouble* params) {
    FeatureCounts counts = {0};
    EvalResult result;
    result.score = 0.0;
    result.num_features = 0;

    double mg = 0.0, eg = 0.0;
    int phase = 0;

    for (int sq = 0; sq < 64; sq++) {
        int piece = get_piece_on_square(pos, sq);
        if (piece == -1) continue;

        int piece_color = (piece < 6) ? WHITE : BLACK;
        int piece_type = piece % 6;
        int mirrored_sq = (piece_color == WHITE) ? sq : MIRROR(sq);

        // Get parameter values and their indices
        double mg_val = params->mg_value[piece_type];
        double eg_val = params->eg_value[piece_type];

        int idx_mg_val = IDX_MG_VALUE + piece_type;
        int idx_eg_val = IDX_EG_VALUE + piece_type;

        // Get PST values and indices
        double mg_pst = 0.0, eg_pst = 0.0;
        int idx_mg_pst = -1, idx_eg_pst = -1;

        switch (piece_type) {
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
        double sign = (piece_color == WHITE) ? +1.0 : -1.0;
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

    evaluate_passed_pawns(pos, &counts, NULL, params, WHITE, NULL, NULL, &mg, &eg);
    int passed_pawn_count_white = counts.passed_pawn_bonus;
    evaluate_passed_pawns(pos, &counts, NULL, params, BLACK, NULL, NULL, &mg, &eg);
    int passed_pawn_count_black = counts.passed_pawn_bonus;

    for (int i = 0; i < passed_pawn_count_white; i++) {
        if (result.num_features + 2 < MAX_FEATURES_PER_POSITION) {
            result.features[result.num_features++] = (FeatureContribution){IDX_PASSED_PAWN_BONUS_MG, +1.0};
            result.features[result.num_features++] = (FeatureContribution){IDX_PASSED_PAWN_BONUS_EG, +1.0};
        }
    }
    
    for (int i = 0; i < passed_pawn_count_black; i++) {
        if (result.num_features + 2 < MAX_FEATURES_PER_POSITION) {
            result.features[result.num_features++] = (FeatureContribution){IDX_PASSED_PAWN_BONUS_MG, -1.0};
            result.features[result.num_features++] = (FeatureContribution){IDX_PASSED_PAWN_BONUS_EG, -1.0};
        }
    }

    evaluate_knight_outposts(pos, &counts, NULL, params, WHITE, NULL, NULL, &mg, &eg);
    int knight_outpost_count_white = counts.knight_outpost_bonus;
    evaluate_knight_outposts(pos, &counts, NULL, params, BLACK, NULL, NULL, &mg, &eg);
    int knight_outpost_count_black = counts.knight_outpost_bonus;

    for (int i = 0; i < knight_outpost_count_white; i++) {
        if (result.num_features + 2 < MAX_FEATURES_PER_POSITION) {
            result.features[result.num_features++] = (FeatureContribution){IDX_KNIGHT_OUTPOST_BONUS_MG, +1.0};
            result.features[result.num_features++] = (FeatureContribution){IDX_KNIGHT_OUTPOST_BONUS_EG, +1.0};
        }
    }
    
    for (int i = 0; i < knight_outpost_count_black; i++) {
        if (result.num_features + 2 < MAX_FEATURES_PER_POSITION) {
            result.features[result.num_features++] = (FeatureContribution){IDX_KNIGHT_OUTPOST_BONUS_MG, -1.0};
            result.features[result.num_features++] = (FeatureContribution){IDX_KNIGHT_OUTPOST_BONUS_EG, -1.0};
        }
    }

    evaluate_rook_activity(pos, &counts, NULL, params, WHITE, NULL, NULL, &mg, &eg);
    int semi_open_file_rook_count_white = counts.rook_semi_open_file_bonus;
    int open_file_rook_count_white = counts.rook_open_file_bonus;
    int blind_swine_rook_count_white = counts.blind_swine_rooks_bonus;
    evaluate_rook_activity(pos, &counts, NULL, params, BLACK, NULL, NULL, &mg, &eg);
    int semi_open_file_rook_count_black = counts.rook_semi_open_file_bonus;
    int open_file_rook_count_black = counts.rook_open_file_bonus;
    int blind_swine_rook_count_black = counts.blind_swine_rooks_bonus;

    for (int i = 0; i < semi_open_file_rook_count_white; i++) {
        if (result.num_features + 2 < MAX_FEATURES_PER_POSITION) {
            result.features[result.num_features++] = (FeatureContribution){IDX_ROOK_SEMI_OPEN_FILE_BONUS_MG, +1.0};
            result.features[result.num_features++] = (FeatureContribution){IDX_ROOK_SEMI_OPEN_FILE_BONUS_EG, +1.0};
        }
    }
    
    for (int i = 0; i < semi_open_file_rook_count_black; i++) {
        if (result.num_features + 2 < MAX_FEATURES_PER_POSITION) {
            result.features[result.num_features++] = (FeatureContribution){IDX_ROOK_SEMI_OPEN_FILE_BONUS_MG, -1.0};
            result.features[result.num_features++] = (FeatureContribution){IDX_ROOK_SEMI_OPEN_FILE_BONUS_EG, -1.0};
        }
    }

    for (int i = 0; i < open_file_rook_count_white; i++) {
        if (result.num_features + 2 < MAX_FEATURES_PER_POSITION) {
            result.features[result.num_features++] = (FeatureContribution){IDX_ROOK_OPEN_FILE_BONUS_MG, +1.0};
            result.features[result.num_features++] = (FeatureContribution){IDX_ROOK_OPEN_FILE_BONUS_EG, +1.0};
        }
    }
    
    for (int i = 0; i < open_file_rook_count_black; i++) {
        if (result.num_features + 2 < MAX_FEATURES_PER_POSITION) {
            result.features[result.num_features++] = (FeatureContribution){IDX_ROOK_OPEN_FILE_BONUS_MG, -1.0};
            result.features[result.num_features++] = (FeatureContribution){IDX_ROOK_OPEN_FILE_BONUS_EG, -1.0};
        }
    }

    for (int i = 0; i < blind_swine_rook_count_white; i++) {
        if (result.num_features + 2 < MAX_FEATURES_PER_POSITION) {
            result.features[result.num_features++] = (FeatureContribution){IDX_BLIND_SWINE_ROOKS_BONUS_MG, +1.0};
            result.features[result.num_features++] = (FeatureContribution){IDX_BLIND_SWINE_ROOKS_BONUS_EG, +1.0};
        }
    }
    
    for (int i = 0; i < blind_swine_rook_count_black; i++) {
        if (result.num_features + 2 < MAX_FEATURES_PER_POSITION) {
            result.features[result.num_features++] = (FeatureContribution){IDX_BLIND_SWINE_ROOKS_BONUS_MG, -1.0};
            result.features[result.num_features++] = (FeatureContribution){IDX_BLIND_SWINE_ROOKS_BONUS_EG, -1.0};
        }
    }

    evaluate_tropism(pos, &counts, NULL, params, WHITE, NULL, NULL, &mg, &eg);
    for (PieceType piece_type = N; piece_type <= Q; piece_type++) {
        for (int dist = 0; dist < 8; dist++) {
            int idx_mg_tropism = tropism_feature_idx[piece_type][0] + dist;
            int idx_eg_tropism = tropism_feature_idx[piece_type][1] + dist;
            
            for (int count = 0; count < counts.tropism[piece_type][dist]; count++) {
                if (result.num_features + 2 < MAX_FEATURES_PER_POSITION) {
                    result.features[result.num_features++] = (FeatureContribution){idx_mg_tropism, +1.0};
                    result.features[result.num_features++] = (FeatureContribution){idx_eg_tropism, +1.0};
                }
            }
        }
    }
    evaluate_tropism(pos, &counts, NULL, params, BLACK, NULL, NULL, &mg, &eg);
    for (PieceType piece_type = N; piece_type <= Q; piece_type++) {
        for (int dist = 0; dist < 8; dist++) {
            int idx_mg_tropism = tropism_feature_idx[piece_type][0] + dist;
            int idx_eg_tropism = tropism_feature_idx[piece_type][1] + dist;
            
            for (int count = 0; count < counts.tropism[piece_type][dist]; count++) {
                if (result.num_features + 2 < MAX_FEATURES_PER_POSITION) {
                    result.features[result.num_features++] = (FeatureContribution){idx_mg_tropism, +1.0};
                    result.features[result.num_features++] = (FeatureContribution){idx_eg_tropism, +1.0};
                }
            }
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
        } else if (idx == IDX_PASSED_PAWN_BONUS_MG || idx == IDX_KNIGHT_OUTPOST_BONUS_MG || idx == IDX_ROOK_SEMI_OPEN_FILE_BONUS_MG || idx == IDX_ROOK_OPEN_FILE_BONUS_MG || idx == IDX_BLIND_SWINE_ROOKS_BONUS_MG) {
            is_mg = true;
        } else if (idx >= IDX_TROPISM_KNIGHT_MG && idx <= IDX_TROPISM_QUEEN_EG) {
            is_mg = (idx >= IDX_TROPISM_KNIGHT_MG && idx < IDX_TROPISM_KNIGHT_EG) ||
                    (idx >= IDX_TROPISM_BISHOP_MG && idx < IDX_TROPISM_BISHOP_EG) ||
                    (idx >= IDX_TROPISM_ROOK_MG && idx < IDX_TROPISM_ROOK_EG) ||
                    (idx >= IDX_TROPISM_QUEEN_MG && idx < IDX_TROPISM_QUEEN_EG);
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

double find_best_k(const EvalParamsDouble* params, const TrainingEntry* data, int n) {
    double best_k = K_START;
    double best_loss = 1e9;

    for (double k = K_START; k <= K_END; k += K_STEP) {
        double loss = compute_loss(params, data, n, k);
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

double compute_loss(const EvalParamsDouble* params, const TrainingEntry* data, int n, double k) {
    double loss = 0.0;
    Position pos;

    for (int i = 0; i < n; i++) {
        init_position(&pos, data[i].fen);
        EvalResult r = evaluate_with_features(&pos, params);
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

    out->passed_pawn_bonus_mg = (int)round(in->passed_pawn_bonus_mg);
    out->passed_pawn_bonus_eg = (int)round(in->passed_pawn_bonus_eg);

    out->knight_outpost_bonus_mg = (int)round(in->knight_outpost_bonus_mg);
    out->knight_outpost_bonus_eg = (int)round(in->knight_outpost_bonus_eg);

    out->rook_semi_open_file_bonus_mg = (int)round(in->rook_semi_open_file_bonus_mg);
    out->rook_semi_open_file_bonus_eg = (int)round(in->rook_semi_open_file_bonus_eg);
    out->rook_open_file_bonus_mg = (int)round(in->rook_open_file_bonus_mg);
    out->rook_open_file_bonus_eg = (int)round(in->rook_open_file_bonus_eg);
    out->blind_swine_rooks_bonus_mg = (int)round(in->blind_swine_rooks_bonus_mg);
    out->blind_swine_rooks_bonus_eg = (int)round(in->blind_swine_rooks_bonus_eg);

    for (PieceType piece_type = N; piece_type <= Q; piece_type++) {
        for (int dist = 0; dist < 8; dist++) {
            out->tropism_mg[piece_type][dist] = (int)round(in->tropism_mg[piece_type][dist]);
            out->tropism_eg[piece_type][dist] = (int)round(in->tropism_eg[piece_type][dist]);
        }
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
    // fprintf(f, "EvalParams tuned_params = {\n");

    // mg_value and eg_value
    fprintf(f, "static const int mg_value[6] = {");
    for (int i = 0; i < 6; i++) fprintf(f, "%d%s", p->mg_value[i], (i < 5 ? ", " : ""));
    fprintf(f, "};\n");

    fprintf(f, "static const int eg_value[6] = {");
    for (int i = 0; i < 6; i++) fprintf(f, "%d%s", p->eg_value[i], (i < 5 ? ", " : ""));
    fprintf(f, "};\n\n");

    // PSTs
    const char* pst_names[] = {
        "pawn_pst_mg", "pawn_pst_eg",
        "knight_pst_mg", "knight_pst_eg",
        "bishop_pst_mg", "bishop_pst_eg",
        "rook_pst_mg", "rook_pst_eg",
        "queen_pst_mg", "queen_pst_eg",
        "king_pst_mg", "king_pst_eg"
    };

    const int* pst_arrays[] = {
        p->pawn_pst_mg, p->pawn_pst_eg,
        p->knight_pst_mg, p->knight_pst_eg,
        p->bishop_pst_mg, p->bishop_pst_eg,
        p->rook_pst_mg, p->rook_pst_eg,
        p->queen_pst_mg, p->queen_pst_eg,
        p->king_pst_mg, p->king_pst_eg
    };

    for (int array = 0; array < 12; array++) {
        fprintf(f, "static const int %s[64] = {", pst_names[array]);
        for (int sq = 0; sq < 64; sq++) {
            fprintf(f, "%d%s", pst_arrays[array][sq], (sq < 63 ? ", " : ""));
        }
        fprintf(f, "};\n\n");
    }

    // Passed pawn bonuses
    fprintf(f, "static const int passed_pawn_bonus_mg = %d;\n", p->passed_pawn_bonus_mg);
    fprintf(f, "static const int passed_pawn_bonus_eg = %d;\n\n", p->passed_pawn_bonus_eg);

    // Knight outpost bonuses
    fprintf(f, "static const int knight_outpost_bonus_mg = %d;\n", p->knight_outpost_bonus_mg);
    fprintf(f, "static const int knight_outpost_bonus_eg = %d;\n\n", p->knight_outpost_bonus_eg);

    // Semi-open file rook bonuses
    fprintf(f, "static const int rook_semi_open_file_bonus_mg = %d;\n", p->rook_semi_open_file_bonus_mg);
    fprintf(f, "static const int rook_semi_open_file_bonus_eg = %d;\n", p->rook_semi_open_file_bonus_eg);

    // Open file rook bonuses
    fprintf(f, "static const int rook_open_file_bonus_mg = %d;\n", p->rook_open_file_bonus_mg);
    fprintf(f, "static const int rook_open_file_bonus_eg = %d;\n", p->rook_open_file_bonus_eg);

    // Rooks on 2nd/7th rank bonuses
    fprintf(f, "static const int blind_swine_rooks_bonus_mg = %d;\n", p->blind_swine_rooks_bonus_mg);
    fprintf(f, "static const int blind_swine_rooks_bonus_eg = %d;\n\n", p->blind_swine_rooks_bonus_eg);
    
    // Tropism bonuses
    fprintf(f, "static const int tropism_mg[6][8] = {\n");
    for (PieceType piece_type = N; piece_type <= Q; piece_type++) {
        fprintf(f, "    [%d] = {", piece_type);
        for (int dist = 0; dist < 8; dist++) {
            fprintf(f, "%d%s", p->tropism_mg[piece_type][dist], (dist < 7 ? ", " : "},\n"));
        }
    }
    fprintf(f, "};\n\n");

    fprintf(f, "static const int tropism_eg[6][8] = {\n");
    for (PieceType piece_type = N; piece_type <= Q; piece_type++) {
        fprintf(f, "    [%d] = {", piece_type);
        for (int dist = 0; dist < 8; dist++) {
            fprintf(f, "%d%s", p->tropism_eg[piece_type][dist], (dist < 7 ? ", " : "},\n"));
        }
    }
    fprintf(f, "};\n\n");

    // fprintf(f, "};\n");
    fclose(f);
    printf("Saved text params to %s\n", path);
}

void run_minibatch_training(
    EvalParamsDouble* params,
    const TrainingEntry* data,
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
            EvalResult result = evaluate_with_features(&pos, params);
            
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

        params->passed_pawn_bonus_mg -= current_lr * gradient[IDX_PASSED_PAWN_BONUS_MG];
        params->passed_pawn_bonus_eg -= current_lr * gradient[IDX_PASSED_PAWN_BONUS_EG];

        params->knight_outpost_bonus_mg -= current_lr * gradient[IDX_KNIGHT_OUTPOST_BONUS_MG];
        params->knight_outpost_bonus_eg -= current_lr * gradient[IDX_KNIGHT_OUTPOST_BONUS_EG];

        params->rook_semi_open_file_bonus_mg -= current_lr * gradient[IDX_ROOK_SEMI_OPEN_FILE_BONUS_MG];
        params->rook_semi_open_file_bonus_eg -= current_lr * gradient[IDX_ROOK_SEMI_OPEN_FILE_BONUS_EG];

        params->rook_open_file_bonus_mg -= current_lr * gradient[IDX_ROOK_OPEN_FILE_BONUS_MG];
        params->rook_open_file_bonus_eg -= current_lr * gradient[IDX_ROOK_OPEN_FILE_BONUS_EG];

        params->blind_swine_rooks_bonus_mg -= current_lr * gradient[IDX_BLIND_SWINE_ROOKS_BONUS_MG];
        params->blind_swine_rooks_bonus_eg -= current_lr * gradient[IDX_BLIND_SWINE_ROOKS_BONUS_EG];

        // Update tropism weights
        for (PieceType piece_type = N; piece_type <= Q; piece_type++) {
            for (int dist = 0; dist < 8; dist++) {
                int idx_mg = tropism_feature_idx[piece_type][0] + dist;
                int idx_eg = tropism_feature_idx[piece_type][1] + dist;

                params->tropism_mg[piece_type][dist] -= current_lr * gradient[idx_mg];
                params->tropism_eg[piece_type][dist] -= current_lr * gradient[idx_eg];
            }
        }

        // Check if loss improved
        double new_loss = compute_loss(params, data, batch_size, sigmoid_k);
        
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

void run_tuner_main(const char* dataset_path, const char* output_prefix) {
    int num_entries = load_dataset(dataset_path, training_data, MAX_TRAINING);
    if (num_entries <= 0) {
        fprintf(stderr, "Failed to load dataset from %s\n", dataset_path);
        return;
    }

    printf("Loaded %d training positions.\n", num_entries);

    EvalParamsDouble params;
    init_double_params(&params);

    double k = find_best_k(&params, training_data, num_entries);

    printf("Before training: mg_value[0] = %.3f\n", params.mg_value[0]);
    
    run_minibatch_training(
        &params,
        training_data,
        num_entries,
        LEARNING_RATE,
        MAX_ITERATIONS,
        k,
        output_prefix
    );
    
    printf("After training:  rook_open_file_bonus_eg = %.10f\n", params.rook_open_file_bonus_eg);
}