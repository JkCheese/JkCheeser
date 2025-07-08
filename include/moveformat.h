#ifndef MOVEFORMAT_H
#define MOVEFORMAT_H

#include "magic.h"
#include "zobrist.h"

int encode_move(int from_sq, int to_sq, int move_flag);
void square_to_coords(int sq, char* buf);
void move_to_string(int move);
void move_to_san(const Position* pos, int move, char* san, const MagicData* magic, ZobristKeys* keys);

#endif