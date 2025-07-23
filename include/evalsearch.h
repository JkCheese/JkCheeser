#ifndef EVALSEARCH_H
#define EVALSEARCH_H

#include "board.h"
#include "evalparams.h"
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
    
// External arrays
extern int killer_moves[MAX_PLY][2];
extern int history_table[64][64];
extern int counter_moves[64][64];

void sort_moves(Position* pos, MoveList* list, int ply, int tt_move, const MagicData* magic);
int move_order_heuristic(const Position* pos, int move, int ply);
int see(const Position* pos, int move, const MagicData* magic);
int quiescence(Position* pos, int alpha, int beta, const EvalParams* params, const MagicData* magic, ZobristKeys* keys);
int search(Position* pos, int depth, int ply, int alpha, int beta, int is_pv_node, const EvalParams* params, const MagicData* magic, ZobristKeys* keys);
int find_best_move(Position* pos, int max_depth, const EvalParams* params,
                   const MagicData* magic, ZobristKeys* keys,
                   int* mate_line, int* mate_length);
int score_move(const Position* pos, int move, int ply, int tt_move, const MagicData* magic);
int get_lmr_reduction(int depth, int move_count, int is_pv, int is_capture, int gives_check);

#endif