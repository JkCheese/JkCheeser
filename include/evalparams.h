#ifndef EVALPARAMS_H
#define EVALPARAMS_H

#include "board.h"
#include "magic.h"

#define IDX_MG_VALUE 0     // [6]
#define IDX_EG_VALUE 6     // [6]

#define IDX_PAWN_PST_MG 12     // [64]
#define IDX_PAWN_PST_EG 76
#define IDX_KNIGHT_PST_MG 140
#define IDX_KNIGHT_PST_EG 204
#define IDX_BISHOP_PST_MG 268
#define IDX_BISHOP_PST_EG 332
#define IDX_ROOK_PST_MG 396
#define IDX_ROOK_PST_EG 460
#define IDX_QUEEN_PST_MG 524
#define IDX_QUEEN_PST_EG 588
#define IDX_KING_PST_MG 652
#define IDX_KING_PST_EG 716

#define IDX_PASSED_PAWN_BONUS_MG 780
#define IDX_PASSED_PAWN_BONUS_EG 781

#define IDX_KNIGHT_OUTPOST_BONUS_MG 782
#define IDX_KNIGHT_OUTPOST_BONUS_EG 783

#define IDX_ROOK_SEMI_OPEN_FILE_BONUS_MG 784
#define IDX_ROOK_SEMI_OPEN_FILE_BONUS_EG 785
#define IDX_ROOK_OPEN_FILE_BONUS_MG 786
#define IDX_ROOK_OPEN_FILE_BONUS_EG 787
#define IDX_BLIND_SWINE_ROOKS_BONUS_MG 788
#define IDX_BLIND_SWINE_ROOKS_BONUS_EG 789

#define IDX_TROPISM_KNIGHT_MG 790
#define IDX_TROPISM_KNIGHT_EG 798
#define IDX_TROPISM_BISHOP_MG 806
#define IDX_TROPISM_BISHOP_EG 814
#define IDX_TROPISM_ROOK_MG 822
#define IDX_TROPISM_ROOK_EG 830
#define IDX_TROPISM_QUEEN_MG 838
#define IDX_TROPISM_QUEEN_EG 846

#define IDX_KING_ZONE_KNIGHT_MG 854
#define IDX_KING_ZONE_KNIGHT_EG 863
#define IDX_KING_ZONE_BISHOP_MG 872
#define IDX_KING_ZONE_BISHOP_EG 881
#define IDX_KING_ZONE_ROOK_MG 890
#define IDX_KING_ZONE_ROOK_EG 899
#define IDX_KING_ZONE_QUEEN_MG 908
#define IDX_KING_ZONE_QUEEN_EG 917

#define NUM_EVAL_PARAMS 926

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
    
    int passed_pawn_bonus_mg;
    int passed_pawn_bonus_eg;
    
    int knight_outpost_bonus_mg;
    int knight_outpost_bonus_eg;

    int rook_semi_open_file_bonus_mg;
    int rook_open_file_bonus_mg;
    int blind_swine_rooks_bonus_mg;
    int rook_semi_open_file_bonus_eg;
    int rook_open_file_bonus_eg;
    int blind_swine_rooks_bonus_eg;
    
    int tropism_mg[6][8];
    int tropism_eg[6][8];

    int king_zone_attacker_mg[6][9];
    int king_zone_attacker_eg[6][9];
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
    
    double passed_pawn_bonus_mg;
    double passed_pawn_bonus_eg;

    double knight_outpost_bonus_mg;
    double knight_outpost_bonus_eg;

    double rook_semi_open_file_bonus_mg;
    double rook_open_file_bonus_mg;
    double blind_swine_rooks_bonus_mg;
    double rook_semi_open_file_bonus_eg;
    double rook_open_file_bonus_eg;
    double blind_swine_rooks_bonus_eg;

    double tropism_mg[6][8];
    double tropism_eg[6][8];

    double king_zone_attacker_mg[6][9];
    double king_zone_attacker_eg[6][9];
} EvalParamsDouble;

void set_default_evalparams(EvalParams* p);
void init_double_params(EvalParamsDouble* d);

#endif