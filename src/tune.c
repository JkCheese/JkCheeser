#include "tune.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

clock_t start_time, end_time;

double sigmoid(double x, double k) {
    return 1.0 / (1.0 + exp(-k * x));
}

double compute_loss_with_k(const TrainingEntry* data, int n, Position* pos, MagicData* magic,
                           EvalParamsDouble* params, double k) {
    double total_loss = 0.0;

    for (int i = 0; i < n; i++) {
        memset(pos, 0, sizeof(Position));
        init_position(pos, data[i].fen);

        int eval_cp = evaluation_with_double(pos, params, magic);
        if (pos->side_to_move == BLACK)
            eval_cp = -eval_cp;

        double win_prob = 1.0 / (1.0 + exp(-k * eval_cp));
        double diff = win_prob - data[i].sf_wdl;

        total_loss += diff * diff;
    }

    return (n > 0) ? total_loss / (double)n : 1e9;
}

void find_best_k(const TrainingEntry* data, int n, Position* pos, MagicData* magic, EvalParamsDouble* params) {
    double best_k = 0.0;
    double min_loss = 1e9;

    for (double k = 0.001; k <= 0.010; k += 0.0005) {
        double loss = compute_loss_with_k(data, n, pos, magic, params, k);
        printf("k = %.4f, loss = %.6f\n", k, loss);

        if (loss < min_loss) {
            min_loss = loss;
            best_k = k;
        }
    }

    printf("\nBest k = %.4f with loss = %.6f\n", best_k, min_loss);
}

int load_training_data(const char* filename, TrainingEntry* data, int max_entries) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Error opening training data file");
        return -1;
    }

    char line[256];
    int count = 0;

    while (count < max_entries && fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\r\n")] = '\0'; // Strip newline

        char* bracket = strchr(line, '[');
        if (!bracket) continue;

        *bracket = '\0';  // Null-terminate FEN part

        double wdl = atof(bracket + 1);  // Parse WDL score inside brackets

        if (wdl < 0.0 || wdl > 1.0) continue;  // Skip invalid lines

        if (strlen(line) >= MAX_FEN_LEN) continue;

        strncpy(data[count].fen, line, MAX_FEN_LEN - 1);
        data[count].fen[MAX_FEN_LEN - 1] = '\0';
        data[count].sf_wdl = wdl;

        count++;
    }

    fclose(file);
    return count;
}

// Compute mean squared error loss over entire dataset
double compute_loss_mem(const TrainingEntry* data, int n, Position* pos, MagicData* magic, EvalParamsDouble* params) {
    double total_loss = 0.0;
    const double k = 0.0085;  // Sigmoid scaling factor

    for (int i = 0; i < n; i++) {
        memset(pos, 0, sizeof(Position));
        init_position(pos, data[i].fen);

        int eval_cp = evaluation_with_double(pos, params, magic);
        if (pos->side_to_move == BLACK)
            eval_cp = -eval_cp;

        double win_prob = sigmoid((double)eval_cp, k);
        // printf("Sigmoid calculation: %.6f\n", win_prob);
        double diff = win_prob - data[i].sf_wdl;

        total_loss += diff * diff;
    }

    return (n > 0) ? total_loss / (double)n : 1e9;
}

// Safely get pointer to a parameter inside EvalParamsDouble by linear index
double* get_param_ref(EvalParamsDouble* params, int index) {
    if (index < 6) {
        // printf("MG_Value: %.2f\n", params->mg_value[index]);
        return &params->mg_value[index];
    }
    index -= 6;

    if (index < 6) {
        // printf("EG_Value: %.2f\n", params->eg_value[index]);
        return &params->eg_value[index];
    }
    index -= 6;

    return NULL; // invalid index
}

void tune_parameters(const TrainingEntry* data, int n, Position* pos, MagicData* magic, EvalParamsDouble* params) {
    const int epochs = 10;
    const double delta = 1;
    const double learning_rate = 0.01;
    const double scale_factor = 100;
    const int param_count = count_total_params();

    for (int epoch = 0; epoch < epochs; epoch++) {
        printf("Epoch %d\n", epoch + 1);

        for (int i = 0; i < param_count; i++) {
            double* param = get_param_ref(params, i);
            double original = *param;

            *param = original + delta;
            double loss_plus = compute_loss_mem(data, n, pos, magic, params);

            *param = original - delta;
            double loss_minus = compute_loss_mem(data, n, pos, magic, params);

            printf("Loss+: %.6f, Loss-: %.6f\n", loss_plus, loss_minus);

            // Restore original value
            *param = original;

            double gradient = loss_plus - loss_minus;
            double scaled_update = (gradient * learning_rate);

            printf("Gradient: %.2f, Scaled Update: %.2f\n", gradient, scaled_update);

            *param = original - scaled_update;

            printf("\n\nParam[%d] = %.2f (grad = %.2f, update = %.2f)\n\n", i, *param, gradient, scaled_update);
        }

        double epoch_loss = compute_loss_mem(data, n, pos, magic, params);
        printf("Epoch %d loss: %.6f\n\n", epoch + 1, epoch_loss);
    }

    EvalParams rounded;
    convert_params(params, &rounded);
    save_params("tuned_params.bin", &rounded);
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
    
    printf("Finding most optimal k...");

        // Initialize EvalParamsDouble from base_params
        EvalParamsDouble dparams = base_params;
        for (int i = 0; i < 6; i++) {
            printf("Init MG[%d] = %.2f, EG[%d] = %.2f\n", i, dparams.mg_value[i], i, dparams.eg_value[i]);
        }
    
    find_best_k(data, n, pos, magic, &dparams);

    printf("\nStarting parameter tuning...\n\n");

    tune_parameters(data, n, pos, magic, &dparams);

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

void convert_params(const EvalParamsDouble* in, EvalParams* out) {
    for (int i = 0; i < 6; i++) {
        out->mg_value[i] = (int)(in->mg_value[i] + 0.5);  // round to nearest
        out->eg_value[i] = (int)(in->eg_value[i] + 0.5);
    }
}

void print_params_as_c_file(FILE* out, const EvalParams* p) {
    fprintf(out, "EvalParams tuned_params = {\n");

    fprintf(out, "    .mg_value = {");
    for (int i = 0; i < 6; i++) {
        fprintf(out, "%d%s", p->mg_value[i], (i == 5) ? "" : ", ");
    }
    fprintf(out, "},\n");

    fprintf(out, "    .eg_value = {");
    for (int i = 0; i < 6; i++) {
        fprintf(out, "%d%s", p->eg_value[i], (i == 5) ? "" : ", ");
    }
    fprintf(out, "},\n");
}