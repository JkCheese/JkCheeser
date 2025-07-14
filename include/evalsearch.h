#ifndef EVALSEARCH_H
#define EVALSEARCH_H

#include "board.h"
#include "movegen.h"
#include "magic.h"

#define MATE_SCORE 100000
#define DRAW_SCORE 0

#define MAX_PLY 64  // Max search depth you expect
extern int killer_moves[MAX_PLY][2];  // Two killer moves per ply

extern int history_table[64][64];

extern int piece_values[];

extern const int
    pawn_pst_mg[64], pawn_pst_eg[64],
    knight_pst_mg[64], knight_pst_eg[64],
    bishop_pst_mg[64], bishop_pst_eg[64],
    rook_pst_mg[64], rook_pst_eg[64],
    queen_pst_mg[64], queen_pst_eg[64],
    king_pst_mg[64], king_pst_eg[64];


static inline int mirror(int sq) {
    return sq ^ 56;
}

typedef struct {
    int mg_value[6];
    int eg_value[6];
} EvalParams;

typedef struct {
    double mg_value[6];  // Floating-point parameters for tuning
    double eg_value[6];
} EvalParamsDouble;

extern const EvalParamsDouble base_params;

void sort_moves(Position* pos, MoveList* list, int ply);
int move_order_heuristic(const Position* pos, int move, int ply);
int evaluation(const Position* pos, EvalParams* params, const MagicData* magic);
int evaluation_with_double(const Position* pos, const EvalParamsDouble* dparams, const MagicData* magic);
int search(Position* pos, int depth, int ply, int alpha, int beta, EvalParams* params, const MagicData* magic, ZobristKeys* keys);
int find_best_move(Position* pos, int depth, EvalParams* params, const MagicData* magic, ZobristKeys* keys);

#endif