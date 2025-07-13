#ifndef TUNE_H
#define TUNE_H

#include "board.h"
#include "evalsearch.h"
#include "magic.h"
#include <stdint.h>
#include <stdio.h>

#define MAX_POSITIONS 7153654
#define MAX_LINE_LEN 256
#define MAX_FEN_LEN 128

typedef struct {
    char fen[MAX_FEN_LEN];
    int sf_wdl;  // stockfish centipawn evaluation
} TrainingEntry;

static inline int count_total_params() {
    return
        6 + 6;
}

int load_training_data(const char* filename, TrainingEntry* data, int max_entries);

double compute_loss_mem(const TrainingEntry* data, int n, Position* pos, MagicData* magic, EvalParamsDouble* params);
double* get_param_ref(EvalParamsDouble* params, int index);
void tune_parameters(const TrainingEntry* data, int n, Position* pos, MagicData* magic, EvalParamsDouble* params);
int tuner(Position* pos, EvalParams* params, MagicData* magic, int argc, char* argv[]);

void save_params(const char* filename, EvalParams* params);
void convert_params(const EvalParamsDouble* in, EvalParams* out);
void print_params_as_c_file(FILE* out, const EvalParams* p);

#endif