#ifndef UCI_H
#define UCI_H

#include "board.h"
#include "movegen.h"
#include "magic.h"

#define STARTPOS_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

void move_to_uci(int move, char out[6]);
int parse_move(const Position* pos, const char* uci_str, const MagicData* magic);
void uci_loop(Position* pos, const MagicData* magic);

#endif