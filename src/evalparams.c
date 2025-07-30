#include "evalparams.h"
#include "movegen.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

void set_default_evalparams(EvalParams* p) {
    /* static const int mg_value[6] = {82, 337, 365, 477, 1025, 0};
    static const int eg_value[6] = {94, 281, 297, 512, 936, 0};

    static const int pawn_pst_mg[64] = {
        0, 0, 0, 0, 0, 0, 0, 0,
        -35, -1, -20, -23, -15, 24, 38, -22,
        -26, -4, -4, -10, 3, 3, 33, -12,
        -27, -2, -5, 12, 17, 6, 10, -25,
        -14, 13, 6, 21, 23, 12, 17, -23,
        -6, 7, 26, 31, 65, 56, 25, -20,
        98, 134, 61, 95, 68, 126, 34, -11,
        0, 0, 0, 0, 0, 0, 0, 0
    };

    static const int pawn_pst_eg[64] = {
        0, 0, 0, 0, 0, 0, 0, 0,
        13, 8, 8, 10, 13, 0, 2, -7,
        4, 7, -6, 1, 0, -5, -1, -8,
        13, 9, -3, -7, -7, -8, 3, -1,
        32, 24, 13, 5, -2, 4, 17, 17,
        94, 100, 85, 67, 56, 53, 82, 84,
        178, 173, 158, 134, 147, 132, 165, 187,
        0, 0, 0, 0, 0, 0, 0, 0
    };

    static const int knight_pst_mg[64] = {
        -105, -21, -58, -33, -17, -28, -19, -23,
        -29, -53, -12, -3, -1, 18, -14, -19,
        -23, -9, 12, 10, 19, 17, 25, -16,
        -13, 4, 16, 13, 28, 19, 21, -8,
        -9, 17, 19, 53, 37, 69, 18, 22,
        -47, 60, 37, 65, 84, 129, 73, 44,
        -73, -41, 72, 36, 23, 62, 7, -17,
        -167, -89, -34, -49, 61, -97, -15, -107
    };

    static const int knight_pst_eg[64] = {
        -29, -51, -23, -15, -22, -18, -50, -64,
        -42, -20, -10, -5, -2, -20, -23, -44,
        -23, -3, -1, 15, 10, -3, -20, -22,
        -18, -6, 16, 25, 16, 17, 4, -18,
        -17, 3, 22, 22, 22, 11, 8, -18,
        -24, -20, 10, 9, -1, -9, -19, -41,
        -25, -8, -25, -2, -9, -25, -24, -52,
        -58, -38, -13, -28, -31, -27, -63, -99
    };

    static const int bishop_pst_mg[64] = {
        -33, -3, -14, -21, -13, -12, -39, -21,
        4, 15, 16, 0, 7, 21, 33, 1,
        0, 15, 15, 15, 14, 27, 18, 10,
        -6, 13, 13, 26, 34, 12, 10, 4,
        -4, 5, 19, 50, 37, 37, 7, -2,
        -16, 37, 43, 40, 35, 50, 37, -2,
        -26, 16, -18, -13, 30, 59, 18, -47,
        -29, 4, -82, -37, -25, -42, 7, -8
    };

    static const int bishop_pst_eg[64] = {
        -23, -9, -23, -5, -9, -16, -5, -17,
        -14, -18, -7, -1, 4, -9, -15, -27,
        -12, -3, 8, 10, 13, 3, -7, -15,
        -6, 3, 13, 19, 7, 10, -3, -9,
        -3, 9, 12, 9, 14, 10, 3, 2,
        2, -8, 0, -1, -2, 6, 0, 4,
        -8, -4, 7, -12, -3, -13, -4, -14,
        -14, -21, -11, -8, -7, -9, -17, -24
    };

    static const int rook_pst_mg[64] = {
        -19, -13, 1, 17, 16, 7, -37, -26,
        -44, -16, -20, -9, -1, 11, -6, -71,
        -45, -25, -16, -17, 3, 0, -5, -33,
        -36, -26, -12, -1, 9, -7, 6, -23,
        -24, -11, 7, 26, 24, 35, -8, -20,
        -5, 19, 26, 36, 17, 45, 61, 16,
        27, 32, 58, 62, 80, 67, 26, 44,
        32, 42, 32, 51, 63, 9, 31, 43
    };

    static const int rook_pst_eg[64] = {
        -9, 2, 3, -1, -5, -13, 4, -20,
        -6, -6, 0, 2, -9, -9, -11, -3,
        -4, 0, -5, -1, -7, -12, -8, -16,
        3, 5, 8, 4, -5, -6, -8, -11,
        4, 3, 13, 1, 2, 1, -1, 2,
        7, 7, 7, 5, 4, -3, -5, -3,
        11, 13, 13, 11, -3, 3, 8, 3,
        13, 10, 18, 15, 12, 12, 8, 5
    };

    static const int queen_pst_mg[64] = {
        -1, -18, -9, 10, -15, -25, -31, -50,
        -35, -8, 11, 2, 8, 15, -3, 1,
        -14, 2, -11, -2, -5, 2, 14, 5,
        -9, -26, -9, -10, -2, -4, 3, -3,
        -27, -27, -16, -16, -1, 17, -2, 1,
        -13, -17, 7, 8, 29, 56, 47, 57,
        -24, -39, -5, 1, -16, 57, 28, 54,
        -28, 0, 29, 12, 59, 44, 43, 45
    };

    static const int queen_pst_eg[64] = {
        -33, -28, -22, -43, -5, -32, -20, -41,
        -22, -23, -30, -16, -16, -23, -36, -32,
        -16, -27, 15, 6, 9, 17, 10, 5,
        -18, 28, 19, 47, 31, 34, 39, 23,
        3, 22, 24, 45, 57, 40, 57, 36,
        -20, 6, 9, 49, 47, 35, 19, 9,
        -17, 20, 32, 41, 58, 25, 30, 0,
        -9, 22, 22, 27, 27, 19, 10, 20
    };

    static const int king_pst_mg[64] = {
        -15, 36, 12, -54, 8, -28, 24, 14,
        1, 7, -8, -64, -43, -16, 9, 8,
        -14, -14, -22, -46, -44, -30, -15, -27,
        -49, -1, -27, -39, -46, -44, -33, -51,
        -17, -20, -12, -27, -30, -25, -14, -36,
        -9, 24, 2, -16, -20, 6, 22, -22,
        29, -1, -20, -7, -8, -4, -38, -29,
        -65, 23, 16, -15, -56, -34, 2, 13
    };

    static const int king_pst_eg[64] = {
        -53, -34, -21, -11, -28, -14, -24, -43,
        -27, -11, 4, 13, 14, 4, -5, -17,
        -19, -3, 11, 21, 23, 16, 7, -9,
        -18, -4, 21, 24, 27, 23, 9, -11,
        -8, 22, 24, 27, 26, 33, 26, 3,
        10, 17, 23, 15, 20, 45, 44, 13,
        -12, 17, 14, 17, 17, 38, 23, 11,
        -74, -35, -18, -18, -11, 15, 4, -17
    };

    static const int passed_pawn_bonus_mg = 0;
    static const int passed_pawn_bonus_eg = 0;
    
    static const int knight_outpost_bonus_mg = 0;
    static const int knight_outpost_bonus_eg = 0;

    static const int rook_semi_open_file_bonus_mg = 0;
    static const int rook_semi_open_file_bonus_eg = 0;
    static const int rook_open_file_bonus_mg = 0;
    static const int rook_open_file_bonus_eg = 0;
    static const int blind_swine_rooks_bonus_mg = 0;
    static const int blind_swine_rooks_bonus_eg = 0;

    static const int tropism_mg[6][8] = {
        [P] = { 0, 0, 0, 0, 0, 0, 0, 0 },
        [N] = { 0, 0, 0, 0, 0, 0, 0, 0 },
        [B] = { 0, 0, 0, 0, 0, 0, 0, 0 },
        [R] = { 0, 0, 0, 0, 0, 0, 0, 0 },
        [Q] = { 0, 0, 0, 0, 0, 0, 0, 0 },
        [K] = { 0, 0, 0, 0, 0, 0, 0, 0 }
    };

    static const int tropism_eg[6][8] = {
        [P] = { 0, 0, 0, 0, 0, 0, 0, 0 },
        [N] = { 0, 0, 0, 0, 0, 0, 0, 0 },
        [B] = { 0, 0, 0, 0, 0, 0, 0, 0 },
        [R] = { 0, 0, 0, 0, 0, 0, 0, 0 },
        [Q] = { 0, 0, 0, 0, 0, 0, 0, 0 },
        [K] = { 0, 0, 0, 0, 0, 0, 0, 0 }
    };

    static const int king_zone_attacker_mg[6][9] = {
        [P] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        [N] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        [B] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        [R] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        [Q] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        [K] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 }
    };

    static const int king_zone_attacker_eg[6][9] = {
        [P] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        [N] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        [B] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        [R] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        [Q] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        [K] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 }
    }; */

    /* --------------------------------------------------- */
    /* --------------------------------------------------- */
    /* --------------------------------------------------- */
    /* --------------------------------------------------- */
    /* --------------------------------------------------- */

    static const int mg_value[6] = {79, 301, 307, 399, 955, 0};
    static const int eg_value[6] = {103, 326, 336, 577, 994, 0};

    static const int pawn_pst_mg[64] = {0, 0, 0, 0, 0, 0, 0, 0, -33, -18, -25, -36, -18, 16, 24, -9, -31, -17, -18, -19, -5, -12, 19, 2, -30, -7, -21, 2, -5, 1, 7, -2, -15, -6, -3, -6, 26, 21, 15, -12, 18, 5, 53, 49, 51, 59, 45, 27, 107, 125, 101, 109, 80, 118, 15, 7, 0, 0, 0, 0, 0, 0, 0, 0};

    static const int pawn_pst_eg[64] = {0, 0, 0, 0, 0, 0, 0, 0, 14, 11, 8, 0, 11, 0, 4, -6, 4, 9, -8, 6, -2, 7, 3, -13, 15, 12, 0, -11, -9, -9, 0, -10, 35, 24, 14, 1, -4, -6, 23, 23, 106, 117, 87, 68, 59, 57, 114, 91, 131, 148, 154, 128, 192, 86, 165, 203, 0, 0, 0, 0, 0, 0, 0, 0};

    static const int knight_pst_mg[64] = {-118, -29, -44, -29, -21, -14, -19, -57, -26, -21, 14, -5, -5, 14, 0, -7, -32, -15, 10, 15, 20, 1, 3, -31, -6, -2, 17, 25, 0, 5, 7, -7, 4, -6, 37, 57, 28, 49, 14, 18, -30, 51, 74, 84, 60, 96, 44, 10, -21, 1, 35, 17, 21, 34, 37, 13, -134, -91, -44, -39, 58, -100, -16, -97};

    static const int knight_pst_eg[64] = {-45, -68, -8, -16, -1, -4, -12, -52, -48, -3, -32, -2, -12, -2, -46, -34, -35, 5, 4, -4, 18, -12, 3, -16, -14, 3, 25, 18, 32, 49, -7, 6, -15, 13, 17, 28, 23, 1, 6, -33, 4, -16, -11, -16, 5, -17, -45, -22, -29, -38, -46, 5, 5, -37, -8, -45, -71, -58, -20, -29, -54, -12, -70, -80};

    static const int bishop_pst_mg[64] = {-9, -14, -2, -20, 26, -3, 2, -5, 4, 2, 22, -7, 6, 27, 27, 15, 14, -1, 4, 15, 20, 12, 4, -8, -14, -21, 2, 40, 24, 14, 6, 10, -9, 7, 18, 18, 26, 3, 15, -27, 11, 33, 62, 20, 58, 19, 50, -3, 12, 22, -9, -33, 40, -6, 29, -26, -24, 16, -67, -80, -37, -40, -1, -8};

    static const int bishop_pst_eg[64] = {-42, 10, -28, 16, -18, -9, -24, -39, -37, -9, -16, 5, 2, -3, -22, -52, -26, -7, 13, 6, 13, 10, 13, 12, -14, 32, 8, -5, -5, 8, -3, -13, -7, 14, 12, 22, 35, 4, -4, 34, 1, -3, 5, 13, -14, 0, -17, -19, -24, 14, -12, 0, -7, 3, -4, -45, 11, -13, -11, 4, -39, 45, 4, -12};

    static const int rook_pst_mg[64] = {-1, -9, -8, -1, 4, 3, -2, -11, -32, -25, -7, -15, -26, 20, 4, -9, -10, -22, -27, -10, -35, -26, 19, -19, -37, -25, -15, -21, 33, -25, -11, -26, -1, 38, -1, 1, -1, -8, -3, 6, -20, 0, 13, 26, 31, 33, 36, 2, 49, 38, 22, 63, 63, 51, 30, 39, 49, 53, 42, 11, 17, 9, 37, 79};

    static const int rook_pst_eg[64] = {-5, 1, 10, 4, -5, -2, -2, -11, -3, -9, -19, -1, 13, -16, -6, -18, -20, 8, -8, -12, 17, 0, -26, -40, 16, 13, 12, 7, -13, -1, 3, -9, 19, 13, 4, 33, -8, 7, 14, -8, 15, 19, 11, 14, 8, -24, 9, -5, -6, 23, 12, 19, -8, -12, 20, 18, -4, 2, 26, 10, -2, 22, -10, 14};

    static const int queen_pst_mg[64] = {9, -13, -2, 7, 4, -6, -8, -36, 13, 5, 14, 20, 10, 23, -5, 0, -8, -1, 1, 9, 20, 5, -5, 21, 7, -6, -12, -21, -21, -13, -10, -15, -10, 6, -1, -23, -33, -18, -19, 5, 24, 3, 11, 18, 42, 63, 27, -15, -24, -4, -8, -5, -16, 8, -29, 54, -44, -37, -2, 22, 21, 38, 24, 11};

    static const int queen_pst_eg[64] = {-14, 10, -27, -10, -5, -14, -11, -34, -37, -18, -3, -26, 3, -27, -28, -18, -5, 2, 6, -11, 14, -6, 11, -7, -39, 33, 10, 91, 47, 34, 34, 20, 4, 45, 11, 68, 45, 46, 44, -16, -4, 9, 5, 52, 64, 19, -10, 12, 13, 14, 51, 45, 89, 24, 17, -24, -6, -10, 2, 43, -30, 19, -4, 10};

    static const int king_pst_mg[64] = {13, 55, 38, -85, -8, -27, 26, 24, 22, 41, -12, -13, -18, -11, 32, 16, -12, -32, -39, -66, -48, -31, -21, -45, -63, 5, -32, -57, -59, -51, -44, -59, -9, -29, -4, -45, -51, -31, -20, -20, -36, 17, -7, -15, -17, 18, 18, -23, 21, -8, -27, 4, -3, -5, -23, -25, -66, 22, 13, -12, -53, -43, 8, 13};

    static const int king_pst_eg[64] = {-56, -55, -50, -11, -37, -22, -40, -71, -3, -18, -4, 5, 5, 4, -16, -21, -30, 7, 11, 24, 31, 13, -6, 1, -6, -2, 32, 27, 43, 32, 17, 15, 18, 25, 8, 22, 49, 40, 38, -10, -34, 25, 37, 46, 44, 40, 39, 32, -17, 12, 5, 31, 18, 47, 24, 13, -76, -25, -26, -23, -22, -12, 19, -22};

    static const int passed_pawn_bonus_mg = -6;
    static const int passed_pawn_bonus_eg = 8;

    static const int knight_outpost_bonus_mg = 3;
    static const int knight_outpost_bonus_eg = 14;

    static const int rook_semi_open_file_bonus_mg = 14;
    static const int rook_semi_open_file_bonus_eg = 8;
    static const int rook_open_file_bonus_mg = 37;
    static const int rook_open_file_bonus_eg = 9;
    static const int blind_swine_rooks_bonus_mg = -41;
    static const int blind_swine_rooks_bonus_eg = 5;

    static const int tropism_mg[6][8] = {
        [1] = {0, 17, 31, -15, 1, -19, -22, -30},
        [2] = {0, -17, -2, -8, -4, -7, -12, -8},
        [3] = {0, 0, 0, 19, -1, -26, -33, -37},
        [4] = {0, 0, 30, 13, -4, -26, -37, -46},
    };

    static const int tropism_eg[6][8] = {
        [1] = {0, -5, 1, 27, 1, 3, 15, 4},
        [2] = {0, -22, 20, -14, 29, -5, 23, 8},
        [3] = {0, 0, 5, 3, 5, 13, 17, 23},
        [4] = {0, 0, -9, -9, 19, 9, 28, 20},
    };

    static const int king_zone_attacker_mg[6][9] = {
        [1] = {0, -5, 4, 20, 48, 0, 0, 0, 0},
        [2] = {0, 23, 15, 36, 20, 1, 0, 0, 0},
        [3] = {0, 12, 20, 38, 42, 1, 0, 0, 0},
        [4] = {0, 5, 14, 7, 23, 1, 0, 0, 0},
    };

    static const int king_zone_attacker_eg[6][9] = {
        [1] = {0, -4, -8, -28, 24, 0, 0, 0, 0},
        [2] = {0, -1, 10, 14, 11, 0, 0, 0, 0},
        [3] = {0, 0, -4, -14, 17, 0, 0, 0, 0},
        [4] = {0, 16, 19, 11, 6, 0, 0, 0, 0},
    };

    /* --------------------------------------------------- */
    /* --------------------------------------------------- */
    /* --------------------------------------------------- */
    /* --------------------------------------------------- */
    /* --------------------------------------------------- */

    // static const int eg[6] = {100, 320, 330, 500, 900, 0};
    // static const int mg[6] = {100, 320, 330, 500, 900, 0};

    // static const int pawn_pst_mg[64] = {
    //     0,   0,   0,   0,   0,   0,   0,   0,
    //     50,  50,  50,  50,  50,  50,  50,  50,
    //     10,  10,  20,  30,  30,  20,  10,  10,
    //     5,   5,  10,  25,  25,  10,   5,   5,
    //     0,   0,   0,  20,  20,   0,   0,   0,
    //     5,  -5, -10,   0,   0, -10,  -5,   5,
    //     5,  10,  10, -20, -20,  10,  10,   5,
    //     0,   0,   0,   0,   0,   0,   0,   0
    // };

    // static const int pawn_pst_eg[64] = {
    //     0,   0,   0,   0,   0,   0,   0,   0,
    //     30,  30,  30,  30,  30,  30,  30,  30,
    //     20,  20,  20,  20,  20,  20,  20,  20,
    //     10,  10,  10,  10,  10,  10,  10,  10,
    //     5,   5,   5,   5,   5,   5,   5,   5,
    //     0,   0,   0,   0,   0,   0,   0,   0,
    //     -10, -10, -10, -10, -10, -10, -10, -10,
    //     0,   0,   0,   0,   0,   0,   0,   0
    // };

    // static const int knight_pst_mg[64] = {
    //     -20, -10, -10, -10, -10, -10, -10, -20,
    //     -10,   0,   5,   0,   0,   5,   0, -10,
    //     -10,   5,  10,  10,  10,  10,   5, -10,
    //     -10,   0,  10,  15,  15,  10,   0, -10,
    //     -10,   5,  10,  15,  15,  10,   5, -10,
    //     -10,   0,  10,  10,  10,  10,   0, -10,
    //     -10,   0,   0,   0,   0,   0,   0, -10,
    //     -20, -10, -10, -10, -10, -10, -10, -20
    // };

    // static const int knight_pst_eg[64] = {
    //     -10, -10, -10, -10, -10, -10, -10, -10,
    //     -10,  -5,   0,   0,   0,   0,  -5, -10,
    //     -10,   0,   5,   5,   5,   5,   0, -10,
    //     -10,   0,   5,  10,  10,   5,   0, -10,
    //     -10,   0,   5,  10,  10,   5,   0, -10,
    //     -10,   0,   5,   5,   5,   5,   0, -10,
    //     -10,  -5,   0,   0,   0,   0,  -5, -10,
    //     -10, -10, -10, -10, -10, -10, -10, -10
    // };

    // static const int bishop_pst_mg[64] = {
    //     -10,  -5, -10, -10, -10, -10,  -5, -10,
    //     -5,   5,   0,   0,   0,   0,   5,  -5,
    //     -5,  10,  10,  10,  10,  10,  10,  -5,
    //     -5,   0,  10,  10,  10,  10,   0,  -5,
    //     -5,   5,   5,  10,  10,   5,   5,  -5,
    //     -5,   0,   5,  10,  10,   5,   0,  -5,
    //     -5,   0,   0,   0,   0,   0,   0,  -5,
    //     -10,  -5, -10, -10, -10, -10,  -5, -10
    // };

    // static const int bishop_pst_eg[64] = {
    //     -10, -10, -10, -10, -10, -10, -10, -10,
    //     -10,   0,   0,   0,   0,   0,   0, -10,
    //     -10,   0,   5,   5,   5,   5,   0, -10,
    //     -10,   0,   5,  10,  10,   5,   0, -10,
    //     -10,   0,   5,  10,  10,   5,   0, -10,
    //     -10,   0,   5,   5,   5,   5,   0, -10,
    //     -10,   0,   0,   0,   0,   0,   0, -10,
    //     -10, -10, -10, -10, -10, -10, -10, -10
    // };

    // static const int rook_pst_mg[64] = {
    //     0,   0,   5,  10,  10,   5,   0,   0,
    //     0,   0,   0,   0,   0,   0,   0,   0,
    //     -5,   0,   0,   0,   0,   0,   0,  -5,
    //     -5,   0,   0,   0,   0,   0,   0,  -5,
    //     -5,   0,   0,   0,   0,   0,   0,  -5,
    //     -5,   0,   0,   0,   0,   0,   0,  -5,
    //     5,  10,  10,  10,  10,  10,  10,   5,
    //     0,   0,   0,   5,   5,   0,   0,   0
    // };

    // static const int rook_pst_eg[64] = {
    //     0,   0,   0,   5,   5,   0,   0,   0,
    //     5,  10,  10,  10,  10,  10,  10,   5,
    //     -5,   0,   0,   0,   0,   0,   0,  -5,
    //     -5,   0,   0,   0,   0,   0,   0,  -5,
    //     -5,   0,   0,   0,   0,   0,   0,  -5,
    //     -5,   0,   0,   0,   0,   0,   0,  -5,
    //     0,   0,   0,   0,   0,   0,   0,   0,
    //     0,   0,   0,   5,   5,   0,   0,   0
    // };

    // static const int queen_pst_mg[64] = {
    //     -20, -10, -10,  -5,  -5, -10, -10, -20,
    //     -10,   0,   0,   0,   0,   0,   0, -10,
    //     -10,   0,   5,   5,   5,   5,   0, -10,
    //     -5,   0,   5,   5,   5,   5,   0,  -5,
    //     0,   0,   5,   5,   5,   5,   0,  -5,
    //     -10,   5,   5,   5,   5,   5,   0, -10,
    //     -10,   0,   5,   0,   0,   0,   0, -10,
    //     -20, -10, -10,  -5,  -5, -10, -10, -20,
    // };

    // static const int queen_pst_eg[64] = {
    //     -10, -10, -10,  -5,  -5, -10, -10, -10,
    //     -10,   0,   0,   0,   0,   0,   0, -10,
    //     -10,   0,   5,   5,   5,   5,   0, -10,
    //     -5,   0,   5,   5,   5,   5,   0,  -5,
    //     0,   0,   5,   5,   5,   5,   0,  -5,
    //     -10,   5,   5,   5,   5,   5,   0, -10,
    //     -10,   0,   5,   0,   0,   0,   0, -10,
    //     -10, -10, -10,  -5,  -5, -10, -10, -10,
    // };

    // static const int king_pst_mg[64] = {
    //     20,  30,  10,   0,   0,  10,  30,  20,
    //     20,  20,   0,   0,   0,   0,  20,  20,
    //     -10, -20, -20, -20, -20, -20, -20, -10,
    //     -20, -30, -30, -40, -40, -30, -30, -20,
    //     -30, -40, -40, -50, -50, -40, -40, -30,
    //     -30, -40, -40, -50, -50, -40, -40, -30,
    //     -30, -40, -40, -50, -50, -40, -40, -30,
    //     -30, -40, -40, -50, -50, -40, -40, -30,
    // };

    // static const int king_pst_eg[64] = {
    //     -50, -30, -30, -30, -30, -30, -30, -50,
    //     -30, -10,   0,   0,   0,   0, -10, -30,
    //     -30,   0,  10,  15,  15,  10,   0, -30,
    //     -30,   0,  15,  20,  20,  15,   0, -30,
    //     -30,   0,  15,  20,  20,  15,   0, -30,
    //     -30,   0,  10,  15,  15,  10,   0, -30,
    //     -30, -10,   0,   0,   0,   0, -10, -30,
    //     -50, -30, -30, -30, -30, -30, -30, -50,
    // };

    memcpy(p->mg_value, mg_value, sizeof(mg_value));
    memcpy(p->eg_value, eg_value, sizeof(mg_value));

    memcpy(p->pawn_pst_mg, pawn_pst_mg, sizeof(pawn_pst_mg));
    memcpy(p->pawn_pst_eg, pawn_pst_eg, sizeof(pawn_pst_eg));
    memcpy(p->knight_pst_mg, knight_pst_mg, sizeof(knight_pst_mg));
    memcpy(p->knight_pst_eg, knight_pst_eg, sizeof(knight_pst_eg));
    memcpy(p->bishop_pst_mg, bishop_pst_mg, sizeof(bishop_pst_mg));
    memcpy(p->bishop_pst_eg, bishop_pst_eg, sizeof(bishop_pst_eg));
    memcpy(p->rook_pst_mg, rook_pst_mg, sizeof(rook_pst_mg));
    memcpy(p->rook_pst_eg, rook_pst_eg, sizeof(rook_pst_eg));
    memcpy(p->queen_pst_mg, queen_pst_mg, sizeof(queen_pst_mg));
    memcpy(p->queen_pst_eg, queen_pst_eg, sizeof(queen_pst_eg));
    memcpy(p->king_pst_mg, king_pst_mg, sizeof(king_pst_mg));
    memcpy(p->king_pst_eg, king_pst_eg, sizeof(king_pst_eg));

    p->passed_pawn_bonus_mg = passed_pawn_bonus_mg;
    p->passed_pawn_bonus_eg = passed_pawn_bonus_eg;

    p->knight_outpost_bonus_mg = knight_outpost_bonus_mg;
    p->knight_outpost_bonus_eg = knight_outpost_bonus_eg;

    p->rook_semi_open_file_bonus_mg = rook_semi_open_file_bonus_mg;
    p->rook_semi_open_file_bonus_eg = rook_semi_open_file_bonus_eg;
    p->rook_open_file_bonus_mg = rook_open_file_bonus_mg;
    p->rook_open_file_bonus_eg = rook_open_file_bonus_eg;
    p->blind_swine_rooks_bonus_mg = blind_swine_rooks_bonus_mg;
    p->blind_swine_rooks_bonus_eg = blind_swine_rooks_bonus_eg;

    memcpy(p->tropism_mg, tropism_mg, sizeof(tropism_mg));
    memcpy(p->tropism_eg, tropism_eg, sizeof(tropism_eg));

    memcpy(p->king_zone_attacker_mg, king_zone_attacker_mg, sizeof(king_zone_attacker_mg));
    memcpy(p->king_zone_attacker_eg, king_zone_attacker_eg, sizeof(king_zone_attacker_eg));
}

void init_double_params(EvalParamsDouble* d) {
    EvalParams i;
    set_default_evalparams(&i); // Fill the integer defaults

    for (int j = 0; j < 6; j++) {
        d->mg_value[j] = (double)i.mg_value[j];
        d->eg_value[j] = (double)i.eg_value[j];
    }

    for (int j = 0; j < 64; j++) {
        d->pawn_pst_mg[j] = (double)i.pawn_pst_mg[j];
        d->pawn_pst_eg[j] = (double)i.pawn_pst_eg[j];
        d->knight_pst_mg[j] = (double)i.knight_pst_mg[j];
        d->knight_pst_eg[j] = (double)i.knight_pst_eg[j];
        d->bishop_pst_mg[j] = (double)i.bishop_pst_mg[j];
        d->bishop_pst_eg[j] = (double)i.bishop_pst_eg[j];
        d->rook_pst_mg[j] = (double)i.rook_pst_mg[j];
        d->rook_pst_eg[j] = (double)i.rook_pst_eg[j];
        d->queen_pst_mg[j] = (double)i.queen_pst_mg[j];
        d->queen_pst_eg[j] = (double)i.queen_pst_eg[j];
        d->king_pst_mg[j] = (double)i.king_pst_mg[j];
        d->king_pst_eg[j] = (double)i.king_pst_eg[j];
    }

    d->passed_pawn_bonus_mg = (double)i.passed_pawn_bonus_mg;
    d->passed_pawn_bonus_eg = (double)i.passed_pawn_bonus_eg;
    
    d->knight_outpost_bonus_mg = (double)i.knight_outpost_bonus_mg;
    d->knight_outpost_bonus_eg = (double)i.knight_outpost_bonus_eg;

    d->rook_semi_open_file_bonus_mg = (double)i.rook_semi_open_file_bonus_mg;
    d->rook_semi_open_file_bonus_eg = (double)i.rook_semi_open_file_bonus_eg;
    d->rook_open_file_bonus_mg = (double)i.rook_open_file_bonus_mg;
    d->rook_open_file_bonus_eg = (double)i.rook_open_file_bonus_eg;
    d->blind_swine_rooks_bonus_mg = (double)i.blind_swine_rooks_bonus_mg;
    d->blind_swine_rooks_bonus_eg = (double)i.blind_swine_rooks_bonus_eg;

    for (PieceType piece_type = N; piece_type <= Q; piece_type++) {
        for (int dist = 0; dist < 8; dist++) {
            d->tropism_mg[piece_type][dist] = (double)i.tropism_mg[piece_type][dist];
            d->tropism_eg[piece_type][dist] = (double)i.tropism_eg[piece_type][dist];
        }
    }

    for (PieceType piece_type = N; piece_type <= Q; piece_type++) {
        for (int attacker_count = 0; attacker_count < 9; attacker_count++) {
            d->king_zone_attacker_mg[piece_type][attacker_count] = (double)i.king_zone_attacker_mg[piece_type][attacker_count];
            d->king_zone_attacker_eg[piece_type][attacker_count] = (double)i.king_zone_attacker_eg[piece_type][attacker_count];
        }
    }
}