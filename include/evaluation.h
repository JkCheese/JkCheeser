#ifndef EVALUATION_H
#define EVALUATION_H

#include "board.h"
#include "evalparams.h"
#include "evaltuner.h"
#include "magic.h"

void evaluate_passed_pawns(const Position* pos, FeatureCounts* counts, const EvalParams* params, const EvalParamsDouble* dparams, int side, int* mg, int* eg, double* dmg, double* deg);
void evaluate_knight_outposts(const Position* pos, FeatureCounts* counts, const EvalParams* params, const EvalParamsDouble* dparams, int side, int* mg, int* eg, double* dmg, double* deg);
void evaluate_rook_activity(const Position* pos, FeatureCounts* counts, const EvalParams* params, const EvalParamsDouble* dparams, int side, int* mg, int* eg, double* dmg, double* deg);
int evaluation(const Position* pos, const EvalParams* params);

#endif