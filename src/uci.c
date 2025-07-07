#include "board.h"
#include "movegen.h"
#include "magic.h"

void move_to_uci(int move, char out[6]) {
    int from = (move >> 6) & 0x3F;
    int to = move & 0x3F;
    int flag = (move >> 12) & 0xF;

    char files[] = "abcdefgh";
    char ranks[] = "12345678";

    out[0] = files[from % 8];
    out[1] = ranks[from / 8];
    out[2] = files[to % 8];
    out[3] = ranks[to / 8];

    // Promotion handling (e.g., q, r, b, n)
    if (flag >= 6 && flag <= 13) {
        char promo = (flag <= 9) ? "nbrq"[flag - 6] : "nbrq"[flag - 10];
        out[4] = promo;
        out[5] = '\0';
    } else {
        out[4] = '\0';
    }
}

int parse_move(const Position* pos, const char* uci_str, const MagicData* magic) {
    MoveList list;
    generate_legal_moves(pos, &list, pos->side_to_move, magic);

    for (int i = 0; i < list.count; ++i) {
        char move_str[6];
        move_to_uci(list.moves[i], move_str);
        if (strncmp(move_str, uci_str, 5) == 0) { // allow match with or without promo letter
            return list.moves[i];
        }
    }
    return 0; // Illegal move
}