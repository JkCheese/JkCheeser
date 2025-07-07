#ifndef BOARD_H
#define BOARD_H

#include <math.h>
#include <stdint.h>
#include <stdbool.h>

#define ON_BOARD(sq) ((sq) >= 0 && (sq) < 64)

#define RANK(sq) (sq / 8)
#define FILE(sq) (sq % 8)

#define RANK_X(rank_num) (0x00000000000000FFULL << ((rank_num - 1) * 8))
#define FILE_X(file_num) (0x0101010101010101ULL << (file_num - 1))

typedef uint64_t Bitboard;
typedef uint64_t File;
typedef uint64_t Rank;

typedef struct {
    Bitboard pieces[12]; // 0-5: white P, N, B, R, Q, K | 6-11: black P, N, B, R, Q, K
    Bitboard occupied[3]; // 0: white, 1: black, 2: all pieces
    int side_to_move; // 0 for white, 1 for black
    int castling_rights; // 4 bits: WK = 1, WQ = 2, BK = 4, BQ = 8
    int king_from[2]; // Index 0 = white king square, 1 = black
    int king_to[2][2]; // Index 0 = white king target square, 1 = black
    int rook_from[4]; // WK, WQ, BK, BQ: starting squares of the rooks
    int rook_to[4]; // WK, WQ, BK, BQ: target squares of the rooks
    int en_passant; // square index for en passant (-1 if not applicable)
    int halfmove_clock; // number of halfmoves since last pawn move or capture
    int fullmove_number; // number of full moves (starts at 1)
} Position;

typedef enum {
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8
} Squares;

typedef enum {
    WHITE, BLACK, ALL
} Color;

typedef enum {
    P, N, B, R, Q, K, EMPTY
} PieceType;

typedef enum {
    WP, WN, WB, WR, WQ, WK,
    BP, BN, BB, BR, BQ, BK,
} ColoredPieceType;

typedef enum {
    WHITE_QUEENSIDE = 1,
    WHITE_KINGSIDE = 2,
    BLACK_QUEENSIDE = 4,
    BLACK_KINGSIDE = 8
} CastlingRights;

typedef enum {
    WHITE_QUEENSIDE_ROOK,
    WHITE_KINGSIDE_ROOK,
    BLACK_QUEENSIDE_ROOK,
    BLACK_KINGSIDE_ROOK
} RookType;

typedef enum {
    QUIET = 0,
    CAPTURE = 1,
    DOUBLE_PUSH = 2,
    EN_PASSANT = 3,
    CASTLE_QUEENSIDE = 4,
    CASTLE_KINGSIDE = 5,
    PROMOTE_N = 6,
    PROMOTE_B = 7,
    PROMOTE_R = 8,
    PROMOTE_Q = 9,
    PROMOTE_N_CAPTURE = 10,
    PROMOTE_B_CAPTURE = 11,
    PROMOTE_R_CAPTURE = 12,
    PROMOTE_Q_CAPTURE = 13 
} MoveFlags;

void init_position(Position* pos, const char* fen);
Bitboard squares_between_inclusive(int a, int b);
Bitboard squares_between_exclusive(int a, int b);
void print_board(const Position* pos);
void print_bitboard(Bitboard bb);
void print_position(const Position* pos);
int piece_index(char c);

#endif