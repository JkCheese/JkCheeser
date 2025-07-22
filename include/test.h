#ifndef TEST_H
#define TEST_H

#include <stdbool.h>

bool is_position_valid(const Position* pos);
uint64_t perft_debug(Position* pos, int depth, const MagicData* magic, ZobristKeys* keys);
void perft_divide(Position* pos, int depth, const MagicData* magic, ZobristKeys* keys);

#endif