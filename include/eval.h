#ifndef EVAL_H
#define EVAL_H

extern int piece_values[];

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

int evaluation(const Position* pos);

#endif