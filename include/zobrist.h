#ifndef ZOBRIST_H
#define ZOBRIST_H

#include "board.h"
#include "tt.h"
#include <stdint.h>

#define Bitboard uint64_t

typedef struct {
    Bitboard zobrist_pieces[12][64]; // piece x square
    Bitboard zobrist_side; // side to move
    Bitboard zobrist_castling[16]; // castling rights bitmask
    Bitboard zobrist_en_passant[8]; // en passant files (0-7)
} ZobristKeys;

void init_zobrist(ZobristKeys* keys);
uint64_t compute_zobrist_hash(const Position* pos, ZobristKeys* keys);

#endif