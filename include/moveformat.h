#ifndef MOVEFORMAT_H
#define MOVEFORMAT_H

#include "magic.h"

int encode_move(int from_sq, int to_sq, int move_flag);
void square_to_coords(int sq, char* buf);
int move_matches(const Position* pos, int piece_type, int from, int to, int except_sq, const MagicData* magic);
void move_to_san(const Position* pos, int move, char* san, const MagicData* magic);

#endif