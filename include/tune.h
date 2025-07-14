#ifndef TUNE_H
#define TUNE_H

#include "board.h"
#include "evalsearch.h"
#include "magic.h"
#include <stdint.h>
#include <stdio.h>

#define MAX_POSITIONS 1000000
#define MAX_LINE_LEN 256
#define MAX_FEN_LEN 128

typedef struct {
    char fen[MAX_FEN_LEN];
    int sf_eval;  // stockfish centipawn evaluation
} TrainingEntry;

static inline int count_total_params() {
    return
        3 + 4 + 3 + 65 + 4 + 8 + 4 + 1 + 1 + 9 + 15 + 15 + 28;
}

int parse_line(const char* line, char* fen_out, int* eval_out);

uint64_t compute_loss_mem(const TrainingEntry* data, int n, Position* pos, MagicData* magic, EvalParams* params);
int* get_param_ref(EvalParams* p, int index);
void tune_parameters(const TrainingEntry* data, int n, Position* pos, MagicData* magic, EvalParams* params);
int tuner(Position* pos, EvalParams* params, MagicData* magic, int argc, char* argv[]);

void save_params(const char* filename, EvalParams* params);
void print_params_as_c_file(FILE* out, const EvalParams* p);

#endif