#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "board.h"
#include "magic.h"
#include "operations.h"
#include <stdbool.h>

#define BOARD_SIZE 64
#define MAX_MOVES 1024

#define MOVE_FROM(move) ((move >> 6) & 0x3F)
#define MOVE_TO(move) (move & 0x3F)
#define MOVE_FLAG(move) ((move >> 12) & 0xF)

#define SAFE_ADD_MOVE(list, from_sq, to_sq, move_flag) do { \
    if ((from_sq) < 0 || (from_sq) > 63 || (to_sq) < 0 || (to_sq) > 63) { \
        /* Silent skip for invalid squares */ \
    } else if ((list)->count >= MAX_MOVES) { \
        /* Silent skip for overflow */ \
    } else { \
        int move = encode_move((from_sq), (to_sq), (move_flag)); \
        if (move != 0) { \
            (list)->moves[(list)->count++] = move; \
        } \
    } \
} while(0)

typedef struct {
    int moves[MAX_MOVES];
    int count;
} MoveList;

typedef struct {
    int from;
    int to;
    int king_sq[2];
    int moved_piece;
    int captured_piece;
    int promoted_piece; // -1 if not a promotion
    int flag;
    int side_to_move;
    int en_passant;
    int castling_rights; // Bitmask for castling rights
    int halfmove_clock;
    int fullmove_number;
    int rook_from_before[4];
    bool has_castled;
} MoveState;

// Bishop attack API
Bitboard mask_bishop_blockers(int sq);
Bitboard compute_bishop_attacks(int sq, Bitboard blockers);
Bitboard bishop_attacks(int sq, Bitboard occupancy, const MagicData* magic);

// Rook attack API
Bitboard mask_rook_blockers(int sq);
Bitboard compute_rook_attacks(int sq, Bitboard blockers);
Bitboard rook_attacks(int sq, Bitboard occupancy, const MagicData* magic);

// Queen attack API
Bitboard queen_attacks(int sq, Bitboard occupancy, const MagicData* magic);

// Individual piece move generation functions
static inline void generate_pawn_moves(const Position* pos, MoveList* list, int side);
Bitboard knight_attacks(int sq);
static inline void generate_knight_moves(const Position* pos, MoveList* list, int side);
static inline void generate_bishop_moves(const Position* pos, MoveList* list, int side, const MagicData* magic);
static inline void generate_rook_moves(const Position* pos, MoveList* list, int side, const MagicData* magic);
static inline void generate_queen_moves(const Position* pos, MoveList* list, int side, const MagicData* magic);
static inline void generate_king_moves(const Position* pos, MoveList* list, int side, const MagicData* magic);
Bitboard king_attacks(int sq);

// Legal move generation and attack functions
static inline int is_square_attacked(const Position* pos, int sq, int side, const MagicData* magic);
int is_legal_move(Position* pos, int move, int side, const MagicData* magic);
void generate_pseudo_legal_moves(const Position* pos, MoveList* list, int side, const MagicData* magic);
void generate_legal_moves(const Position* pos, MoveList* list, int side, const MagicData* magic);

// Check, checkmate, and stalemate detection
int is_in_check(const Position* pos, int side, const MagicData* magic);
int is_in_checkmate(const Position* pos, int side, const MagicData* magic);
int is_in_stalemate(const Position* pos, int side, const MagicData* magic);

// Make and unmake move
int make_move(Position* pos, MoveState* state, int move);
int unmake_move(Position* pos, const MoveState* state);

// General helper functions
static inline int get_piece_on_square(const Position* pos, int sq);
void print_moves(const Position* pos, const MoveList* list, const MagicData* magic);

#endif