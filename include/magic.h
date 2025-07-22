#ifndef MAGIC_H
#define MAGIC_H

#include "board.h"
#include <stdint.h>

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
    uint32_t rook_shifts[64];
    uint32_t bishop_shifts[64];
    Bitboard rook_masks[64];
    Bitboard bishop_masks[64];
    Bitboard rook_attack_table[64][MAX_ROOK_MOVES];
    Bitboard bishop_attack_table[64][MAX_BISHOP_MOVES];
} MagicData;

static const int rook_deltas[4]   = {8, -8, 1, -1};
static const int bishop_deltas[4] = {9, -9, 7, -7};

// Mask between-board edges to prevent wrap-around
static inline int is_valid_rook(int from, int to) {
    return (RANK(from) == RANK(to)) || (FILE(from) == FILE(to));
}

static inline int is_valid_bishop(int from, int to) {
    return abs(RANK(from) - RANK(to)) == abs(FILE(from) - FILE(to));
}

// Given index and mask, return blocker permutation
static inline Bitboard set_blockers_from_index(uint64_t index, Bitboard mask) {
    Bitboard blockers = 0ULL;
    int i = 0;
    for (int sq = 0; sq < 64; sq++) {
        if (mask & (1ULL << sq)) {
            if (index & (1ULL << i)) blockers |= (1ULL << sq);
            i++;
        }
    }
    return blockers;
}

// Compute blocker mask for bishops
static inline Bitboard mask_bishop_blockers(int sq) {
    Bitboard mask = 0ULL;
    for (int d = 0; d < 4; d++) {
        int delta = bishop_deltas[d];
        int s = sq + delta;
        while (ON_BOARD(s) && is_valid_bishop(sq, s)) {
            if (RANK(s) == 0 || RANK(s) == 7 || FILE(s) == 0 || FILE(s) == 7) break;
            mask |= (1ULL << s);
            s += delta;
        }
    }
    // printf("Bishop mask for sq %d: ", sq);
    // for (int i = 0; i < 64; i++) {
    //     if (mask & (1ULL << i)) printf("%d ", i);
    // }
    // printf("\n");
    
    return mask;
}
static inline Bitboard compute_bishop_attacks(int sq, Bitboard blockers) {
    Bitboard attacks = 0ULL;
    for (int d = 0; d < 4; d++) {
        int delta = bishop_deltas[d];
        int s = sq + delta;
        while (ON_BOARD(s) && is_valid_bishop(sq, s)) {
            attacks |= (1ULL << s);
            if (blockers & (1ULL << s)) break;
            s += delta;
        }
    }
    return attacks;
}
// Compute blocker mask for rooks
static inline Bitboard mask_rook_blockers(int sq) {
    Bitboard mask = 0ULL;
    int rank = sq / 8;
    int file = sq % 8;

    // Up
    for (int r = rank + 1; r <= 6; r++) mask |= (1ULL << (r * 8 + file));
    // Down
    for (int r = rank - 1; r >= 1; r--) mask |= (1ULL << (r * 8 + file));
    // Right
    for (int f = file + 1; f <= 6; f++) mask |= (1ULL << (rank * 8 + f));
    // Left
    for (int f = file - 1; f >= 1; f--) mask |= (1ULL << (rank * 8 + f));

    return mask;
}
// Compute attacks given a square and blockers
static inline Bitboard compute_rook_attacks(int sq, Bitboard blockers) {
    Bitboard attacks = 0ULL;
    for (int d = 0; d < 4; d++) {
        int delta = rook_deltas[d];
        int s = sq + delta;
        while (ON_BOARD(s) && is_valid_rook(sq, s)) {
            attacks |= (1ULL << s);
            if (blockers & (1ULL << s)) break;
            s += delta;
        }
    }
    return attacks;
}

void init_magic(MagicData* magic);

int save_magic_tables(const MagicData* magic, const char* path);
int load_magic_tables(MagicData* magic, const char* path);

#endif