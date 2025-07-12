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

#define MAX_REP_HISTORY 1024
extern uint64_t repetition_table[MAX_REP_HISTORY];
extern int repetition_index;

extern const int
    pawn_pst_mg[64], pawn_pst_eg[64],
    knight_pst_mg[64], knight_pst_eg[64],
    bishop_pst_mg[64], bishop_pst_eg[64],
    rook_pst_mg[64], rook_pst_eg[64],
    queen_pst_mg[64], queen_pst_eg[64],
    king_pst_mg[64], king_pst_eg[64];


static inline int mirror(int sq) {
    return ((7 - sq / 8) * 8) + (sq % 8);
}

typedef struct {
    // King Safety
    int king_attackers_penalty;
    int king_pawn_shield_bonus;
    int uncastled_king_penalty;

    // Tropism
    int queen_tropism;
    int rook_tropism;
    int bishop_tropism;
    int knight_tropism;

    // Rook evaluation
    int rook_open_file_bonus;
    int rook_semi_open_file_bonus;
    int rook_on_7th_bonus;

    // Knight evaluation
    int knight_outpost_pst[64];
    int knight_outpost_defended_bonus;

    // Bishop evaluation
    int bishop_long_diagonal_bonus;
    int bishop_open_diagonal_bonus;
    int bishop_pair_mg; // bonus in centipawns
    int bishop_pair_eg; // usually less important, smaller value

    // Passed pawns
    int passed_pawn_bonus[8];  // bonus per rank (0–7), index by rank

    // Pawn structure
    int isolated_pawn_penalty;
    int intd_pawn_penalty;
    int backward_pawn_penalty;
    int connected_passed_bonus;

    // Space and activity
    int space_bonus;
    int piece_activity_bonus;

    // Mobility:
    // Number of attacked squares => bonus in centipawns
    int knight_mobility_bonus[9];
    int bishop_mobility_bonus[15];
    int rook_mobility_bonus[15];
    int queen_mobility_bonus[28];


} EvalParams;

void sort_moves(Position* pos, MoveList* list, int ply);
int move_order_heuristic(const Position* pos, int move, int ply);
int evaluation(const Position* pos, EvalParams* params, const MagicData* magic);
int search(Position* pos, int depth, int ply, int alpha, int beta, EvalParams* params, const MagicData* magic, ZobristKeys* keys);
int find_best_move(Position* pos, int depth, EvalParams* params, const MagicData* magic, ZobristKeys* keys);

#endif