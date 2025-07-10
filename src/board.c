#include "board.h"
#include "moveformat.h"
#include "movegen.h"
#include "operations.h"
#include "zobrist.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

const char piece_chars[12] = {
    'P', 'N', 'B', 'R', 'Q', 'K', // White pieces
    'p', 'n', 'b', 'r', 'q', 'k'  // Black pieces
};

int piece_index(char c) {
    switch (c) {
        case 'P': return WP; case 'N': return WN; case 'B': return WB;
        case 'R': return WR; case 'Q': return WQ; case 'K': return WK;
        case 'p': return BP; case 'n': return BN; case 'b': return BB;
        case 'r': return BR; case 'q': return BQ; case 'k': return BK;
        default:  return -1;
    }
}

Bitboard squares_between_inclusive(int a, int b) {
    Bitboard bb = 0;
    if (a == b) return 1ULL << a;

    int step = 0;

    if (RANK(a) == RANK(b)) step = (b > a) ? 1 : -1;
    else if (FILE(a) == FILE(b)) step = (b > a) ? 8 : -8;
    else if (abs(FILE(a) - FILE(b)) == abs(RANK(a) - RANK(b))) {
        step = (b > a) ? ((FILE(b) > FILE(a)) ? 9 : 7) : ((FILE(b) > FILE(a)) ? -7 : -9);
    } else return 0;

    for (int sq = a; ; sq += step) {
        bb |= 1ULL << sq;
        if (sq == b) break;
    }
    return bb;
}

Bitboard squares_between_exclusive(int a, int b) {
    if (a == b) return 0;
    Bitboard bb = 0;

    int step = 0;

    if (RANK(a) == RANK(b)) {
        step = (b > a) ? 1 : -1;
    } else if (FILE(a) == FILE(b)) {
        step = (b > a) ? 8 : -8;
    } else if (abs(FILE(a) - FILE(b)) == abs(RANK(a) - RANK(b))) {
        step = (b > a) ? ((FILE(b) > FILE(a)) ? 9 : 7)
                      : ((FILE(b) > FILE(a)) ? -7 : -9);
    } else {
        return 0;
    }

    for (int sq = a + step; sq != b; sq += step)
        bb |= 1ULL << sq;

    return bb;
}

void print_position(const Position* pos) {
    if (!pos) return;

    printf("Position:\n");
    printf("  Side to move: %s\n", pos->side_to_move == WHITE ? "White" : "Black");
    printf("  En passant square: %d\n", pos->en_passant);
    printf("  Castling rights: 0x%X\n", pos->castling_rights);
    printf("  Halfmove clock: %d\n", pos->halfmove_clock);
    printf("  Fullmove number: %d\n", pos->fullmove_number);
    printf("  King squares: White = %d, Black = %d\n", pos->king_from[WHITE], pos->king_from[BLACK]);

    printf("  Occupied bitboards:\n");
    printf("    White:   0x%016llX\n", pos->occupied[WHITE]);
    printf("    Black:   0x%016llX\n", pos->occupied[BLACK]);
    printf("    All:     0x%016llX\n", pos->occupied[ALL]);

    printf("  Piece bitboards:\n");
    const char* piece_names[12] = {
        "WP", "WN", "WB", "WR", "WQ", "WK",
        "BP", "BN", "BB", "BR", "BQ", "BK"
    };

    for (int i = 0; i < 12; i++) {
        printf("    %s: 0x%016llX\n", piece_names[i], pos->pieces[i]);
    }
}

void print_bitboard(Bitboard bb) {
    printf("\n  +-----------------+\n");
    for (int rank = 7; rank >= 0; --rank) {
        printf("%d | ", rank + 1);
        for (int file = 0; file < 8; ++file) {
            int sq = rank * 8 + file;
            printf("%c ", (bb & (1ULL << sq)) ? '1' : '.');
        }
        printf("|\n");
    }
    printf("  +-----------------+\n");
    printf("    a b c d e f g h\n\n");
}

void print_board(const Position* pos) {
    printf("  +------------------------+\n");
    for (int rank = 7; rank >= 0; rank--) {
        printf("%d |", rank + 1);
        for (int file = 0; file < 8; file++) {
            int sq = rank * 8 + file;
            char piece = '.';
            for (int i = 0; i < 12; i++) {
                if (pos->pieces[i] & (1ULL << sq)) {
                    piece = piece_chars[i];
                    break;
                }
            }
            printf(" %c", piece);
        }
        printf(" |\n");
    }
    printf("  +------------------------+\n");
    printf("    a b c d e f g h\n\n");
    printf("Side to move: %s\n", pos->side_to_move == 0 ? "White" : "Black");
}

void init_position(Position* pos, const char* fen) {
    // Clear position
    memset(pos, 0, sizeof(Position));
    pos->en_passant = -1;
    
    // Prepare FEN for parsing
    char fen_copy[128];
    strncpy(fen_copy, fen, sizeof(fen_copy) - 1);
    fen_copy[sizeof(fen_copy) - 1] = '\0';
    char* token = strtok(fen_copy, " "); // Split by space

    // Setting up the pieces
    int square = 56;
    int white_rook_count = 0;
    int black_rook_count = 0;

    // Rook positions after castling
    pos->rook_to[WHITE_QUEENSIDE_ROOK] = 3; // d1
    pos->rook_to[WHITE_KINGSIDE_ROOK] = 5;  // f1
    pos->rook_to[BLACK_QUEENSIDE_ROOK] = 59; // d8
    pos->rook_to[BLACK_KINGSIDE_ROOK] = 61; // f8

    for (char* c = token; *c; c++) {
        if (*c >= '1' && *c <= '8') {
            square += (*c - '0'); // Skip squares
        } else if (*c == '/') {
            square -= 16; // Move to the next rank
        } else {
            int index = piece_index(*c);
            if (index >= 0) {
                pos->pieces[index] |= 1ULL << square;
                pos->occupied[index < 6 ? 0 : 1] |= 1ULL << square;
                pos->occupied[ALL] |= 1ULL << square;

                // Track king and rook locations for castling
                if (*c == 'K') pos->king_from[WHITE] = square;
                if (*c == 'k') pos->king_from[BLACK] = square;
                if (*c == 'R') {
                    white_rook_count++;
                    if (white_rook_count == 1) {
                        pos->rook_from[WHITE_QUEENSIDE_ROOK] = square;
                    } else {
                        pos->rook_from[WHITE_KINGSIDE_ROOK] = square;
                    }                               
                }
                if (*c == 'r') {
                    black_rook_count++;
                    if (black_rook_count == 1) {
                        pos->rook_from[BLACK_QUEENSIDE_ROOK] = square;
                    } else {
                        pos->rook_from[BLACK_KINGSIDE_ROOK] = square;
                    } 
                }
            }
            square++;
        }
    }

    // Parse side to move
    token = strtok(NULL, " ");
    pos->side_to_move = (token[0] == 'w') ? 0 : 1; // Default to white

    // Parse castling rights
    token = strtok(NULL, " ");
    if (strchr(token, 'K')) pos->castling_rights |= WHITE_KINGSIDE;
    if (strchr(token, 'Q')) pos->castling_rights |= WHITE_QUEENSIDE;
    if (strchr(token, 'k')) pos->castling_rights |= BLACK_KINGSIDE;
    if (strchr(token, 'q')) pos->castling_rights |= BLACK_QUEENSIDE;
    // printf("Castling rights: %d\n", pos->castling_rights);
    pos->has_castled = false;

    // Parse en passant square
    token = strtok(NULL, " ");
    if (token[0] != '-') {
        pos->en_passant = (token[1] - '1') * 8 + (token[0] - 'a'); // Convert to square index
    } else {
        pos->en_passant = -1; // No en passant
    }

    // Parse halfmove clock
    token = strtok(NULL, " ");
    pos->halfmove_clock = atoi(token);

    // Parse fullmove number
    token = strtok(NULL, " ");
    pos->fullmove_number = atoi(token);
}

void print_moves(const Position* pos, const MoveList* list, const MagicData* magic, ZobristKeys* keys) {
    static const char* flag_names[] = {
        "QUIET", "CAPTURE", "DOUBLE_PUSH", "EN_PASSANT",
        "CASTLE_QUEENSIDE", "CASTLE_KINGSIDE",
        "PROMOTE_N", "PROMOTE_B", "PROMOTE_R", "PROMOTE_Q",
        "PROMOTE_N_CAPTURE", "PROMOTE_B_CAPTURE", "PROMOTE_R_CAPTURE", "PROMOTE_Q_CAPTURE"
    };

    char san[16];

    printf("Generated Moves:\n");
    for (int i = 0; i < list->count; i++) {
        int move = list->moves[i];
        int flag = MOVE_FLAG(move);

        move_to_san(pos, move, san, magic, keys);
        const char* flag_str = (flag >= 0 && flag < 14) ? flag_names[flag] : "UNKNOWN";

        printf("Move %d: %s, Flag: %s\n", i + 1, san, flag_str);
    }
    // printf("Checking checkmate for %s\n", pos->side_to_move == 0 ? "White" : "Black");
    // printf("White king square: %d\n", pos->king_from[WHITE]);
    // printf("Black king square: %d\n", pos->king_from[BLACK]);
    // if (list->count == 0 && is_in_checkmate(pos, pos->side_to_move, magic)) {
    //     printf("%s has no legal moves: he is in checkmate.\n", pos->side_to_move == 0 ? "White" : "Black");
    //     fflush(stdout);
    //     return;
    // } else if (list->count == 0 && is_in_stalemate(pos, pos->side_to_move, magic)) {
    //     printf("%s has no legal moves: he is in stalemate.\n", pos->side_to_move == 0 ? "White" : "Black");
    //     fflush(stdout);
    //     return;
    // }
}