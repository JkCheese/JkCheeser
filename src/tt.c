#include "tt.h"

TTEntry transposition_table[TT_SIZE];

void tt_init() {
    for (int i = 0; i < TT_SIZE; i++) {
        transposition_table[i].key = 0ULL;
        transposition_table[i].depth = -1;
        transposition_table[i].score = 0;
        transposition_table[i].best_move = 0;
        transposition_table[i].flag = TT_NONE;
    }
}

void tt_store(uint64_t key, int depth, int score, int best_move, TTFlag flag) {
    int index = tt_index(key);
    TTEntry* entry = &transposition_table[index];

    // Always store if the slot is empty or if this entry is deeper
    if (entry->key == 0 || depth >= entry->depth) {
        entry->key = key;
        entry->depth = depth;
        entry->score = score;
        entry->best_move = best_move;
        entry->flag = flag;
    }
}

int tt_probe(uint64_t key, int depth, int alpha, int beta, int* out_score, int* out_move) {
    int index = tt_index(key);
    TTEntry* entry = &transposition_table[index];

    if (entry->key == key && entry->depth >= depth) {
        *out_move = entry->best_move;

        if (entry->flag == TT_EXACT) {
            *out_score = entry->score;
            return 1;
        }
        if (entry->flag == TT_ALPHA && entry->score <= alpha) {
            *out_score = alpha;
            return 1;
        }
        if (entry->flag == TT_BETA && entry->score >= beta) {
            *out_score = beta;
            return 1;
        }
    }

    return 0; // Not usable
}