#ifndef EVALUATION_H
#define EVALUATION_H

#include "board.h"
#include "evalparams.h"
#include "evaltuner.h"
#include "magic.h"

static inline int manhattan(int sq1, int sq2) {
    int f1 = sq1 % 8, r1 = sq1 / 8;
    int f2 = sq2 % 8, r2 = sq2 / 8;
    return abs(f1 - f2) + abs(r1 - r2);
}

void evaluate_passed_pawns(const Position* pos, FeatureCounts* counts, const EvalParams* params, const EvalParamsDouble* dparams, int side, int* mg, int* eg, double* dmg, double* deg);
void evaluate_knight_outposts(const Position* pos, FeatureCounts* counts, const EvalParams* params, const EvalParamsDouble* dparams, int side, int* mg, int* eg, double* dmg, double* deg);
void evaluate_rook_activity(const Position* pos, FeatureCounts* counts, const EvalParams* params, const EvalParamsDouble* dparams, int side, int* mg, int* eg, double* dmg, double* deg);
void evaluate_tropism(const Position* pos, FeatureCounts* counts, const EvalParams* params, const EvalParamsDouble* dparams, int side, int* mg, int* eg, double* dmg, double* deg);
int evaluation(const Position* pos, const EvalParams* params);

#endif