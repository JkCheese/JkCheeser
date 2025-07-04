#ifndef MAGIC_H
#define MAGIC_H

#include <stdint.h>

#define Bitboard uint64_t
#define MAX_ROOK_MOVES 4096
#define MAX_BISHOP_MOVES 512
#define MAX_BLOCKER_VARIATIONS 4096

extern const Bitboard rook_magics_const[64];
extern const Bitboard bishop_magics_const[64];
extern const int rook_shifts_const[64];
extern const int bishop_shifts_const[64];

typedef struct {
    Bitboard rook_magics[64];
    Bitboard bishop_magics[64];
    int rook_shifts[64];
    int bishop_shifts[64];
    Bitboard rook_masks[64];
    Bitboard bishop_masks[64];
    Bitboard rook_attack_table[64][MAX_ROOK_MOVES];
    Bitboard bishop_attack_table[64][MAX_BISHOP_MOVES];
} MagicData;

Bitboard set_blockers_from_index(uint64_t index, Bitboard mask);

Bitboard mask_bishop_blockers(int sq);
Bitboard compute_bishop_attacks(int sq, Bitboard blockers);
Bitboard mask_rook_blockers(int sq);
Bitboard compute_rook_attacks(int sq, Bitboard blockers);

void init_magic(MagicData* magic);

#endif
