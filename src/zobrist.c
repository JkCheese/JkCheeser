#include "zobrist.h"
#include <stdlib.h>
#include <time.h>

static uint64_t random_u64() {
    return ((uint64_t)rand() << 48) ^
           ((uint64_t)rand() << 32) ^
           ((uint64_t)rand() << 16) ^
           ((uint64_t)rand());
}

void init_zobrist(ZobristKeys* keys) {
    srand(20250707); // fixed seed for reproducibility

    for (int p = 0; p < 12; p++)
        for (int sq = 0; sq < 64; sq++)
            keys->zobrist_pieces[p][sq] = random_u64();

    for (int i = 0; i < 16; i++)
        keys->zobrist_castling[i] = random_u64();

    for (int f = 0; f < 8; f++)
        keys->zobrist_en_passant[f] = random_u64();

    keys->zobrist_side = random_u64();
}

uint64_t compute_zobrist_hash(const Position* pos, ZobristKeys* keys) {
    uint64_t hash = 0;

    for (int p = 0; p < 12; p++) {
        Bitboard bb = pos->pieces[p];
        while (bb) {
            int sq = __builtin_ctzll(bb);
            hash ^= keys->zobrist_pieces[p][sq];
            bb &= bb - 1;
        }
    }

    hash ^= keys->zobrist_castling[pos->castling_rights];

    if (pos->en_passant != -1)
        hash ^= keys->zobrist_en_passant[pos->en_passant % 8];

    if (pos->side_to_move == BLACK)
        hash ^= keys->zobrist_side;

    return hash;
}