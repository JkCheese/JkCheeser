#include "board.h"
#include "magic.h"
#include "moveformat.h"
#include "movegen.h"
#include <string.h>
#include <stdio.h>

int encode_move(int from_sq, int to_sq, int move_flag) {
    // Ensure the move is within bounds
    if (from_sq < 0 || from_sq > 63 || to_sq < 0 || to_sq > 63) {
        return 0; // Invalid move
    }
    
    // Encode the move as a single integer
    return (move_flag << 12) | (from_sq << 6) | to_sq ;
}

// Converts a square index (0-63) to algebraic notation like e4
void square_to_coords(int sq, char* buf) {
    buf[0] = 'a' + (sq % 8);
    buf[1] = '1' + (sq / 8);
    buf[2] = '\0';
}

// Check if two moves match in type and destination, but come from different origins
int move_matches(const Position* pos, int piece_type, int from, int to, int except_sq, const MagicData* magic) {
    MoveList list;
    generate_pseudo_legal_moves(pos, &list, pos->side_to_move, magic);

    for (int i = 0; i < list.count; ++i) {
        int move = list.moves[i];
        int src = (move >> 6) & 0x3F;
        int dst = move & 0x3F;
        int flag = (move >> 12) & 0xF;

        if (src == except_sq) continue;
        if (dst != to) continue;

        // Make sure it's the same piece type
        for (int p = 0; p < 12; ++p) {
            if ((pos->pieces[p] >> src) & 1ULL && p % 6 == piece_type % 6 && (p < 6) == (piece_type < 6))
                return 1;
        }
    }
    return 0;
}

// Get SAN from move
void move_to_san(const Position* pos, int move, char* san, const MagicData* magic) {
    int from = (move >> 6) & 0x3F;
    int to = move & 0x3F;
    int flag = (move >> 12) & 0xF;

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

    char disambig[4] = "";
    char promo[3] = "";
    char check_status[2] = "";
    char capture = (flag == CAPTURE || flag >= PROMOTE_N_CAPTURE || flag == EN_PASSANT) ? 'x' : 0;
    char piece_char = "PNBRQK"[piece_type % 6];

    if (piece_char == 'P') piece_char = 0; // No piece letter for pawns

    // Disambiguation
    int need_file = 0, need_rank = 0;
    for (int i = 0; i < 64; ++i) {
        if (i == from) continue;
        for (int p = 0; p < 12; ++p) {
            if ((pos->pieces[p] >> i) & 1ULL && p % 6 == piece_type % 6 && (p < 6) == (piece_type < 6)) {
                MoveList list;
                generate_pseudo_legal_moves(pos, &list, side, magic);
                for (int m = 0; m < list.count; ++m) {
                    int alt_from = (list.moves[m] >> 6) & 0x3F;
                    int alt_to = list.moves[m] & 0x3F;
                    if (alt_from == i && alt_to == to) {
                        if ((i % 8) == (from % 8)) need_rank = 1;
                        if ((i / 8) == (from / 8)) need_file = 1;
                        if (!need_file && !need_rank) { need_file = need_rank = 1; }
                    }
                }
            }
        }
    }

    if (piece_char) {
        disambig[0] = piece_char;
        disambig[1] = 0;
        if (need_file || need_rank) {
            disambig[1] = from_str[need_file ? 0 : 1];
            disambig[2] = need_file && need_rank ? from_str[1] : 0;
            disambig[3] = 0;
        }
    } else if (capture) {
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
    make_move(&temp, &state, move);

    int opponent = temp.side_to_move; // side to move now
    // The opponent may be in check if just moved side attacked them
    if (is_in_checkmate(&temp, opponent, magic)) {
        check_status[0] = '#';
        check_status[1] = 0;
    }
    else if (is_in_check(&temp, opponent, magic)) {
        check_status[0] = '+';
        check_status[1] = 0;
    }
    else {
        check_status[0] = 0;
        check_status[1] = 0;
    }

    // Build SAN string
    if (capture)
        sprintf(san, "%s%c%s%s%s", disambig, capture, to_str, promo, check_status);
    else
        sprintf(san, "%s%s%s%s", disambig, to_str, promo, check_status);

    printf("Move: %s, Check: %d, Checkmate: %d\n", san,
        is_in_check(&temp, opponent, magic),
        is_in_checkmate(&temp, opponent, magic));
    unmake_move(&temp, &state);
}