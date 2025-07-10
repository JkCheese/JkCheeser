#ifndef UCI_H
#define UCI_H

#include "board.h"
#include "movegen.h"
#include "magic.h"
#include "zobrist.h"

void move_to_uci(int move, char out[6]);
int parse_move(const Position* pos, const char* uci_str, const MagicData* magic, const ZobristKeys* keys);
void uci_loop(Position* pos, MoveList* list, MoveState* state, int depth, const MagicData* magic, const ZobristKeys* keys);

#endif