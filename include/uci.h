#ifndef UCI_H
#define UCI_H

#include "board.h"
#include "movegen.h"
#include "magic.h"

void move_to_uci(int move, char out[6]);
int parse_move(const Position* pos, const char* uci_str, const MagicData* magic);

#endif