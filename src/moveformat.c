#include "board.h"
#include "magic.h"
#include "moveformat.h"
#include "movegen.h"
#include "zobrist.h"
#include <string.h>
#include <stdio.h>

// Converts a square index (0-63) to algebraic notation like e4
void square_to_coords(int sq, char* buf) {
    buf[0] = 'a' + (sq % 8);
    buf[1] = '1' + (sq / 8);
    buf[2] = '\0';
}

void move_to_string(int move) {
    int from = MOVE_FROM(move);
    int to = MOVE_TO(move);;
    char from_str[3];
    char to_str[3];

    square_to_coords(from, from_str);
    square_to_coords(to, to_str);
    printf("%s -> %s, ", from_str, to_str);
}

int encode_move(int from_sq, int to_sq, int move_flag) {
    // Ensure the move is within bounds
    if (from_sq < 0 || from_sq > 63 || to_sq < 0 || to_sq > 63) {
        return 0; // Invalid move
    }
    
    // Encode the move as a single integer
    return (move_flag << 12) | (from_sq << 6) | to_sq ;
}

void move_to_san(const Position* pos, int move, char* san, const MagicData* magic, ZobristKeys* keys) {
    int from = MOVE_FROM(move);
    int to = MOVE_TO(move);
    int flag = MOVE_FLAG(move);

    int side = pos->side_to_move;
    int piece_type = -1;
    for (int p = 0; p < 12; ++p) {
        if ((pos->pieces[p] >> from) & 1ULL) {
            piece_type = p;
            break;
        }
    }

    char from_str[3], to_str[3];
    square_to_coords(from, from_str);
    square_to_coords(to, to_str);

    // Handle castling
    if (flag == CASTLE_KINGSIDE) {
        strcpy(san, "O-O");
        return;
    } else if (flag == CASTLE_QUEENSIDE) {
        strcpy(san, "O-O-O");
        return;
    }

    char piece_char = "PNBRQK"[piece_type % 6];
    char promo[3] = "";
    char check_status[2] = "";
    char disambig[2] = "";
    char capture = (flag == CAPTURE || flag >= PROMOTE_N_CAPTURE || flag == EN_PASSANT) ? 'x' : 0;

    // Only show the piece letter if not a pawn
    if (piece_char != 'P') {
        disambig[0] = piece_char;
        disambig[1] = 0;
    } else {
        disambig[0] = 0;
    }

    // Show file for pawn captures (e.g. exd5)
    if (piece_char == 'P' && capture) {
        disambig[0] = from_str[0];
        disambig[1] = 0;
    }

    // Promotion
    if (flag >= PROMOTE_N && flag <= PROMOTE_Q_CAPTURE) {
        promo[0] = '=';
        promo[1] = "NBRQ"[(flag - PROMOTE_N) % 4];
        promo[2] = 0;
    }

    Position temp = *pos;
    MoveState state;
    make_move(&temp, &state, move, keys);

    int opponent = temp.side_to_move;
    if (is_in_checkmate(&temp, opponent, magic, keys)) {
        check_status[0] = '#';
        check_status[1] = 0;
    } else if (is_in_check(&temp, opponent, magic)) {
        check_status[0] = '+';
        check_status[1] = 0;
    } else {
        check_status[0] = 0;
        check_status[1] = 0;
    }

    if (capture)
        sprintf(san, "%s%c%s%s%s", disambig, capture, to_str, promo, check_status);
    else
        sprintf(san, "%s%s%s%s", disambig, to_str, promo, check_status);

    unmake_move(&temp, &state, keys);
}