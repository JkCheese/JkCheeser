#ifndef EVALPARAMS_H
#define EVALPARAMS_H

#include "board.h"
#include "magic.h"

#define IDX_MG_VALUE          0     // [6]
#define IDX_EG_VALUE          6     // [6]
#define IDX_PAWN_PST_MG      12     // [64]
#define IDX_PAWN_PST_EG      76
#define IDX_KNIGHT_PST_MG   140
#define IDX_KNIGHT_PST_EG   204
#define IDX_BISHOP_PST_MG   268
#define IDX_BISHOP_PST_EG   332
#define IDX_ROOK_PST_MG     396
#define IDX_ROOK_PST_EG     460
#define IDX_QUEEN_PST_MG    524
#define IDX_QUEEN_PST_EG    588
#define IDX_KING_PST_MG     652
#define IDX_KING_PST_EG     716

#define NUM_EVAL_PARAMS      780

typedef struct {
    int mg_value[6];  // P, N, B, R, Q, K
    int eg_value[6];  // P, N, B, R, Q, K
    int pawn_pst_mg[64];
    int pawn_pst_eg[64];
    int knight_pst_mg[64];
    int knight_pst_eg[64];
    int bishop_pst_mg[64];
    int bishop_pst_eg[64];
    int rook_pst_mg[64];
    int rook_pst_eg[64];
    int queen_pst_mg[64];
    int queen_pst_eg[64];
    int king_pst_mg[64];
    int king_pst_eg[64];
} EvalParams;

// --- Double-precision Evaluation Parameters ---
typedef struct {
    double mg_value[6];
    double eg_value[6];
    double pawn_pst_mg[64];
    double pawn_pst_eg[64];
    double knight_pst_mg[64];
    double knight_pst_eg[64];
    double bishop_pst_mg[64];
    double bishop_pst_eg[64];
    double rook_pst_mg[64];
    double rook_pst_eg[64];
    double queen_pst_mg[64];
    double queen_pst_eg[64];
    double king_pst_mg[64];
    double king_pst_eg[64];
} EvalParamsDouble;

double evaluation_double(const Position* pos, const EvalParamsDouble* params, const MagicData* magic);
void set_default_evalparams(EvalParams* p);
void init_double_params(EvalParamsDouble* d);

#endif