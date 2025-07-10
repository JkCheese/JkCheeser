#ifndef TT_H
#define TT_H

#include <stdint.h>

#define TT_SIZE (1 << 20)  // 1 million entries ~ 16 MB if each entry ~16 bytes

typedef enum {
    TT_NONE,
    TT_EXACT,     // exact score
    TT_ALPHA,     // lower bound (fail low)
    TT_BETA       // upper bound (fail high)
} TTFlag;

typedef struct {
    uint64_t key;    // Zobrist hash key (position identifier)
    int depth;       // Search depth at which this was stored
    int score;       // Evaluated score of the position
    int best_move;   // Best move found in this position
    TTFlag flag;     // Type of entry
} TTEntry;

extern TTEntry transposition_table[TT_SIZE];

static inline int tt_index(uint64_t key) {
    return (int)(key & (TT_SIZE - 1));
}

void tt_init();
void tt_store(uint64_t key, int depth, int score, int best_move, TTFlag flag);
int tt_probe(uint64_t key, int depth, int alpha, int beta, int* out_score, int* out_move);

#endif