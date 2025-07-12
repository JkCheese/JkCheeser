#include "tune.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

clock_t start_time, end_time;

int load_training_data(const char* filename, TrainingEntry* data, int max_entries) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Error opening training data file");
        return -1;
    }

    char line[256];
    int count = 0;

    while (count < max_entries && fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\r\n")] = '\0';

        char* tab_pos = strchr(line, '\t');
        if (!tab_pos) continue;

        size_t fen_len = tab_pos - line;
        if (fen_len >= MAX_FEN_LEN) continue;

        strncpy(data[count].fen, line, fen_len);
        data[count].fen[fen_len] = '\0';

        data[count].sf_eval = atoi(tab_pos + 1);

        count++;
    }

    fclose(file);
    return count;  // number of entries loaded
}

// Parse one line of "FEN\tEvaluation"
int parse_line(const char* line, char* fen_out, int* eval_out) {
    const char* tab_pos = strchr(line, '\t');
    if (!tab_pos) return 0;

    size_t fen_len = tab_pos - line;
    if (fen_len >= MAX_FEN_LEN) return 0;

    strncpy(fen_out, line, fen_len);
    fen_out[fen_len] = '\0';

    *eval_out = atoi(tab_pos + 1);
    return 1;
}

// Compute mean squared error loss over entire dataset
uint64_t compute_loss_mem(const TrainingEntry* data, int n, Position* pos, MagicData* magic, EvalParams* params) {
    uint64_t total_loss = 0;

    for (int i = 0; i < n; i++) {
        init_position(pos, data[i].fen);
        int engine_eval = evaluation(pos, params, magic);
        int diff = engine_eval - data[i].sf_eval;
        total_loss += (uint64_t)diff * diff;
    }

    if (n > 0) {
        return (uint64_t)(total_loss / n);
    } else {
        return 1000000000;  // large error if no data
    }
}

// Safely get pointer to a parameter inside EvalParams by linear index
int* get_param_ref(EvalParams* p, int index) {
    if (index < 3) {
        switch (index) {
            case 0: return &p->king_attackers_penalty;
            case 1: return &p->king_pawn_shield_bonus;
            case 2: return &p->uncastled_king_penalty;
        }
    }
    index -= 3;

    if (index < 4) {
        switch (index) {
            case 0: return &p->queen_tropism;
            case 1: return &p->rook_tropism;
            case 2: return &p->bishop_tropism;
            case 3: return &p->knight_tropism;
        }
    }
    index -= 4;

    if (index < 3) {
        switch (index) {
            case 0: return &p->rook_open_file_bonus;
            case 1: return &p->rook_semi_open_file_bonus;
            case 2: return &p->rook_on_7th_bonus;
        }
    }
    index -= 3;

    if (index < 64) {
        return &p->knight_outpost_pst[index];
    }
    index -= 64;

    if (index == 0) return &p->knight_outpost_defended_bonus;
    index--;

    if (index < 4) {
        switch (index) {
            case 0: return &p->bishop_long_diagonal_bonus;
            case 1: return &p->bishop_open_diagonal_bonus;
            case 2: return &p->bishop_pair_mg;
            case 3: return &p->bishop_pair_eg;
        }
    }
    index -= 4;

    if (index < 8) {
        return &p->passed_pawn_bonus[index];
    }
    index -= 8;

    if (index < 4) {
        switch (index) {
            case 0: return &p->isolated_pawn_penalty;
            case 1: return &p->intd_pawn_penalty;
            case 2: return &p->backward_pawn_penalty;
            case 3: return &p->connected_passed_bonus;
        }
    }
    index -= 4;

    if (index == 0) return &p->space_bonus;
    index--;

    if (index == 0) return &p->piece_activity_bonus;
    index--;

    if (index < 9) {
        return &p->knight_mobility_bonus[index];
    }
    index -= 9;

    if (index < 15) {
        return &p->bishop_mobility_bonus[index];
    }
    index -= 15;

    if (index < 15) {
        return &p->rook_mobility_bonus[index];
    }
    index -= 15;

    if (index < 28) {
        return &p->queen_mobility_bonus[index];
    }
    index -= 28;

    return NULL; // invalid index
}

void tune_parameters(const TrainingEntry* data, int n, Position* pos, MagicData* magic, EvalParams* params) {
    const int epochs = 10;
    const int delta = 1;
    const int learning_rate = 1;
    const int scale_factor = 100;
    const int param_count = count_total_params();

    for (int epoch = 0; epoch < epochs; ++epoch) {
        printf("Epoch %d\n", epoch + 1);

        for (int i = 0; i < param_count; ++i) {
            int* param = get_param_ref(params, i);
            int original = *param;

            *param = original + delta;
            uint64_t loss_plus = compute_loss_mem(data, n, pos, magic, params);

            *param = original - delta;
            uint64_t loss_minus = compute_loss_mem(data, n, pos, magic, params);

            printf("Loss+: %" PRIu64 ", Loss-: %" PRIu64 "\n", loss_plus, loss_minus);

            // Restore original value
            *param = original;

            int gradient = (int)((int64_t)loss_plus - (int64_t)loss_minus);  // signed gradient
            int scaled_update = (gradient * learning_rate) / scale_factor;         // scale factor prevents overjumping
            printf("Gradient: %d, Scaled Update: %d\n", gradient, scaled_update);

            *param = original - scaled_update;

            printf("  Param[%d] = %d (grad = %d, update = %d)\n", i, *param, gradient, scaled_update);
        }

        uint64_t epoch_loss = compute_loss_mem(data, n, pos, magic, params);
        printf("Epoch %d loss: %" PRIu64 "\n\n", epoch + 1, epoch_loss);
    }

    save_params("tuned_params.bin", params);
}

// Main tuner function called from main.c
int tuner(Position* pos, EvalParams* params, MagicData* magic, int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s positions.txt\n", argv[0]);
        return 1;
    }

    TrainingEntry* data = malloc(sizeof(TrainingEntry) * MAX_POSITIONS);
    if (!data) {
        perror("Failed to allocate training data array");
        return 1;
    }

    int n = load_training_data(argv[1], data, MAX_POSITIONS);
    if (n <= 0) {
        fprintf(stderr, "No training data loaded\n");
        free(data);
        return 1;
    }

    printf("Loaded %d training positions\n", n);

    // Optional: print first few for debug
    // for (int i = 0; i < n && i < 1000000; i++) {
    //     init_position(pos, data[i].fen);
    //     int engine_eval = evaluation(pos, params, magic);
    //     int error = engine_eval - data[i].sf_eval;
    //     printf("Sample %d: Eval error = %+d cp\n", i+1, error);
    // }

    printf("\nStarting parameter tuning...\n\n");

    tune_parameters(data, n, pos, magic, params);

    free(data);
    return 0;
}

void save_params(const char* filename, EvalParams* params) {
    FILE* f = fopen(filename, "wb");
    if (!f) {
        perror("Error saving params");
        return;
    }
    fwrite(params, sizeof(EvalParams), 1, f);
    fclose(f);
    printf("Saved tuned params to %s\n", filename);

    FILE* cfile = fopen("tuned_params.c", "w");
    if (!cfile) {
        perror("Error opening tuned_params.c for writing");
        return;
    }
    print_params_as_c_file(cfile, params);
    fclose(cfile);
    printf("C initializer written to tuned_params.c\n");
}

void print_params_as_c_file(FILE* out, const EvalParams* p) {
    fprintf(out, "EvalParams tuned_params = {\n");

    fprintf(out, "    .king_attackers_penalty = %d,\n", p->king_attackers_penalty);
    fprintf(out, "    .king_pawn_shield_bonus = %d,\n", p->king_pawn_shield_bonus);
    fprintf(out, "    .uncastled_king_penalty = %d,\n", p->uncastled_king_penalty);

    fprintf(out, "    .queen_tropism = %d,\n", p->queen_tropism);
    fprintf(out, "    .rook_tropism = %d,\n", p->rook_tropism);
    fprintf(out, "    .bishop_tropism = %d,\n", p->bishop_tropism);
    fprintf(out, "    .knight_tropism = %d,\n", p->knight_tropism);

    fprintf(out, "    .rook_open_file_bonus = %d,\n", p->rook_open_file_bonus);
    fprintf(out, "    .rook_semi_open_file_bonus = %d,\n", p->rook_semi_open_file_bonus);
    fprintf(out, "    .rook_on_7th_bonus = %d,\n", p->rook_on_7th_bonus);

    fprintf(out, "    .knight_outpost_pst = {");
    for (int i = 0; i < 64; i++) {
        fprintf(out, "%d%s", p->knight_outpost_pst[i], (i == 63) ? "" : ", ");
    }
    fprintf(out, "},\n");

    fprintf(out, "    .knight_outpost_defended_bonus = %d,\n", p->knight_outpost_defended_bonus);

    fprintf(out, "    .bishop_long_diagonal_bonus = %d,\n", p->bishop_long_diagonal_bonus);
    fprintf(out, "    .bishop_open_diagonal_bonus = %d,\n", p->bishop_open_diagonal_bonus);
    fprintf(out, "    .bishop_pair_mg = %d,\n", p->bishop_pair_mg);
    fprintf(out, "    .bishop_pair_eg = %d,\n", p->bishop_pair_eg);

    fprintf(out, "    .passed_pawn_bonus = {");
    for (int i = 0; i < 8; i++) {
        fprintf(out, "%d%s", p->passed_pawn_bonus[i], (i == 7) ? "" : ", ");
    }
    fprintf(out, "},\n");

    fprintf(out, "    .isolated_pawn_penalty = %d,\n", p->isolated_pawn_penalty);
    fprintf(out, "    .intd_pawn_penalty = %d,\n", p->intd_pawn_penalty);
    fprintf(out, "    .backward_pawn_penalty = %d,\n", p->backward_pawn_penalty);
    fprintf(out, "    .connected_passed_bonus = %d,\n", p->connected_passed_bonus);

    fprintf(out, "    .space_bonus = %d,\n", p->space_bonus);
    fprintf(out, "    .piece_activity_bonus = %d,\n", p->piece_activity_bonus);

    fprintf(out, "    .knight_mobility_bonus = {");
    for (int i = 0; i < 9; i++) {
        fprintf(out, "%d%s", p->knight_mobility_bonus[i], (i == 8) ? "" : ", ");
    }
    fprintf(out, "},\n");

    fprintf(out, "    .bishop_mobility_bonus = {");
    for (int i = 0; i < 15; i++) {
        fprintf(out, "%d%s", p->bishop_mobility_bonus[i], (i == 14) ? "" : ", ");
    }
    fprintf(out, "},\n");

    fprintf(out, "    .rook_mobility_bonus = {");
    for (int i = 0; i < 15; i++) {
        fprintf(out, "%d%s", p->rook_mobility_bonus[i], (i == 14) ? "" : ", ");
    }
    fprintf(out, "},\n");

    fprintf(out, "    .queen_mobility_bonus = {");
    for (int i = 0; i < 28; i++) {
        fprintf(out, "%d%s", p->queen_mobility_bonus[i], (i == 27) ? "" : ", ");
    }
    fprintf(out, "}\n");

    fprintf(out, "};\n");
}