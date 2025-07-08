#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "board.h"
#include "magic.h"
#include "operations.h"
#include "zobrist.h"
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

/* ---------- General helper functions ---------- */

// Function to get the piece on a given square
static inline int get_piece_on_square(const Position* pos, int sq) {
    // Initialize a bitboard with a bit set at the given square to compare with all piece bitboards 
    Bitboard piece_bb = 1ULL << sq;
    // For every piece bitboard, check if there is a piece on the same square as the previously set bitboard and return the piece if one is found
    for (int piece = 0; piece < 12; piece++) {
        if (pos->pieces[piece] & piece_bb) {
            // printf("Piece: %d\n", piece);
            // print_bitboard(pos->pieces[piece]);
            return piece;
        }
    }

    // Return -1 if no piece is found
    return -1;
}
void print_moves(const Position* pos, const MoveList* list, const MagicData* magic, ZobristKeys* keys);

/* ---------- Attack lookup functions ---------- */

// Function to generate knight attacks
static inline Bitboard knight_attacks(int sq) {

    // Initialize the knight bitboard
    Bitboard knight = 0;
    // Set the knight to be on the given square
    set_bit(knight, sq);
    
    // Precompute knight moves with file masks to prevent wrap-around (overflow automatically handled by bitwise operations)
    Bitboard precomputed_knight_attacks = 0;

    precomputed_knight_attacks |= (knight & ~FILE_X(8)) << 17; // +1 file, +2 ranks
    precomputed_knight_attacks |= (knight & ~FILE_X(1)) << 15; // -1 file, +2 ranks
    precomputed_knight_attacks |= (knight & ~(FILE_X(8) | FILE_X(7))) << 10; // +2 files, +1 rank
    precomputed_knight_attacks |= (knight & ~(FILE_X(1) | FILE_X(2))) << 6; // -2 files, +1 rank
    precomputed_knight_attacks |= (knight & ~FILE_X(1)) >> 17; // -1 file, -2 ranks
    precomputed_knight_attacks |= (knight & ~FILE_X(8)) >> 15; // +1 file, -2 ranks
    precomputed_knight_attacks |= (knight & ~(FILE_X(1) | FILE_X(2))) >> 10; // -2 files, -1 rank
    precomputed_knight_attacks |= (knight & ~(FILE_X(8) | FILE_X(7))) >> 6; // +2 files, -1 rank

    // Return the precomputed knight moves
    return precomputed_knight_attacks;
}

// Function to generate bishop attacks
static inline Bitboard bishop_attacks(int sq, Bitboard occupancy, const MagicData* magic) {
    Bitboard blockers = occupancy & magic->bishop_masks[sq];
    uint64_t index = (blockers * magic->bishop_magics[sq]) >> (64 - magic->bishop_shifts[sq]);
    return magic->bishop_attack_table[sq][index];
}

// Function to generate rook attacks
static inline Bitboard rook_attacks(int sq, Bitboard occupancy, const MagicData* magic) {
    Bitboard blockers = occupancy & magic->rook_masks[sq];
    uint64_t index = (blockers * magic->rook_magics[sq]) >> (64 - magic->rook_shifts[sq]);
    return magic->rook_attack_table[sq][index];
}

// Function to generate queen attacks
static inline Bitboard queen_attacks(int sq, Bitboard occupancy, const MagicData* magic) {
    return rook_attacks(sq, occupancy, magic) | bishop_attacks(sq, occupancy, magic);
}

// Function to generate king attacks
static inline Bitboard king_attacks(int sq) {

    // Initialize the king bitboard
    Bitboard king = 0;
    // Set the king to be on the given square
    set_bit(king, sq);

    // Precompute king moves with file masks to prevent wrap-around (overflow automatically handled by bitwise operations)
    Bitboard precomputed_king_attacks = 0;

    precomputed_king_attacks |= (king & ~FILE_X(1)) << 7; // -1 file, +1 rank
    precomputed_king_attacks |= king << 8; // +1 rank
    precomputed_king_attacks |= (king & ~FILE_X(8)) << 9; // +1 file, +1 rank
    precomputed_king_attacks |= (king & ~FILE_X(8)) << 1; // +1 file
    precomputed_king_attacks |= (king & ~FILE_X(8)) >> 7; // +1 file, -1 rank
    precomputed_king_attacks |= king >> 8; // -1 rank
    precomputed_king_attacks |= (king & ~FILE_X(1)) >> 9; // -1 file, -1 rank
    precomputed_king_attacks |= (king & ~FILE_X(1)) >> 1; // -1 file
    
    // Return the precomputed king moves
    return precomputed_king_attacks;
}

// Is a given square attacked by a given side?
static inline int is_square_attacked(const Position* pos, int sq, int attacking_side, const MagicData* magic) {

    // If the pointer points to nothing, or if the square is out of bounds, return false
    if (!pos || sq < 0 || sq > 63) return 0;

    /* ---------- Pawn attacks ---------- */

    // printf("Checking if square %d (%s) is attacked by %s\n", 
    //        sq, (char[3]){('a' + (sq % 8)), ('1' + (sq / 8)), 0}, 
    //        side == WHITE ? "White" : "Black");

    // Initialize the pawn bitboard for the attacking side
    Bitboard pawns = pos->pieces[attacking_side == WHITE ? WP : BP];
    // Initialize a bitboard for all pawn attacks from the given square
    Bitboard pawn_attackers = (attacking_side == WHITE)
        ? (((1ULL << sq) & ~FILE_X(8)) >> 7) | (((1ULL << sq) & ~FILE_X(1)) >> 9)
        : (((1ULL << sq) & ~FILE_X(1)) << 7) | (((1ULL << sq) & ~FILE_X(8)) << 9);


    if (pawns & pawn_attackers) {
        // printf("[DEBUG] pawn attack detected on square %d\n", sq);
        return 1;
    }

    /* ---------- Knight attacks ---------- */

    // Initialize the knight bitboard for the attacking side
    Bitboard knights = pos->pieces[attacking_side == WHITE ? WN : BN];
    // Initialize a bitboard for all knight attacks from the given square
    Bitboard knight_moves = knight_attacks(sq);
    // If at least one knight on the bitboard intersects with at least one of the possible knight moves from the square, return true
    if (knights & knight_moves) {
        // printf("[DEBUG] knight attack detected on square %d\n", sq);
        // printf("  Knight attack detected on square %d\n", sq);
        // for (int i = 0; i < 64; i++) {
        //     if (knights & (1ULL << i)) {
        //         printf("    Knight on %d (%s) attacks %d\n", 
        //                i, (char[3]){('a' + (i % 8)), ('1' + (i / 8)), 0}, sq);
        //     }
        // }
        return 1;
    }

    /* ---------- Bishop/Queen diagonal attacks ---------- */
    
    // Initialize a bitboard for all bishops and queens for the attacking side
    Bitboard bishops_queens = pos->pieces[attacking_side == WHITE ? WB : BB] | pos->pieces[attacking_side == WHITE ? WQ : BQ];
    // Initialize a bitboard for all bishop-like attacks from the given square
    Bitboard bishop_moves = bishop_attacks(sq, pos->occupied[ALL], magic);
    // If at least one bishop or queen on the bitboard intersects with at least one of the possible bishop moves from the square, return true
    if (bishops_queens & bishop_moves) {
        // printf("[DEBUG] bishop/queen attack detected on square %d\n", sq);
        // printf("  Bishop/Queen attack detected on square %d\n", sq);
        // for (int i = 0; i < 64; i++) {
        //     if (bishops_queens & (1ULL << i)) {
        //         printf("    Bishop/Queen on %d (%s) attacks %d\n", 
        //                i, (char[3]){('a' + (i % 8)), ('1' + (i / 8)), 0}, sq);
        //     }
        // }
        return 1;
    }

    /* ---------- Rook/Queen horizontal attacks ---------- */

    // Initialize a bitboard for all rooks and queens for the attacking side
    Bitboard rooks_queens = pos->pieces[attacking_side == WHITE ? WR : BR] | pos->pieces[attacking_side == WHITE ? WQ : BQ];
    // Initialize a bitboard for all rook-like attacks from the given square
    Bitboard rook_moves = rook_attacks(sq, pos->occupied[ALL], magic);
    // If at least one rook or queen on the bitboard intersects with at least one of the possible rook moves from the square, return true
    if (rooks_queens & rook_moves) {
        // printf("[DEBUG] rook/queen attack detected on square %d\n", sq);
        // printf("  Rook/Queen attack detected on square %d\n", sq);
        // for (int i = 0; i < 64; i++) {
        //     if (rooks_queens & (1ULL << i)) {
        //         printf("    Rook/Queen on %d (%s) attacks %d\n", 
        //                i, (char[3]){('a' + (i % 8)), ('1' + (i / 8)), 0}, sq);
        //     }
        // }
        return 1;
    }

    /* ---------- King attacks ---------- */

    // Initialize the king bitboard for the attacking side
    Bitboard kings = pos->pieces[attacking_side == WHITE ? WK : BK];
    // Initialize a bitboard for all king attacks from the given square
    Bitboard king_moves = king_attacks(sq);
    // If the king's square on its bitboard intersects with at least one of the possible king moves from the square, return true
    if (kings & king_moves) {
        // printf("[DEBUG] king attack detected on square %d\n", sq);
        // printf("  King attack detected on square %d\n", sq);
        return 1;
    }

    // printf("  No attacks detected on square %d\n", sq);

    // Return false if no attacks were detected
    return 0;
}

/* ---------- Individual piece move generation functions ---------- */ 

// Function to generate moves for all pawns on the bitboard
static inline void generate_pawn_moves(const Position* pos, MoveList* list, int side) {
    if (!pos || !list) return;
    // Get the pawn bitboard for the current side
    Bitboard pawns = pos->pieces[side == WHITE ? WP : BP];
    // Get the occupied squares for the opponent
    Bitboard opp = pos->occupied[side ^ 1];
    // Get all empty squares
    Bitboard empty = ~pos->occupied[ALL];
    // Get the enemy king bitboard
    Bitboard enemy_king = pos->pieces[side ? WK : BK];
    // Set promotion rank
    Rank promotion_rank = (side == WHITE) ? (RANK_X(8)) : (RANK_X(1));
    // Set forward direction
    int forward = (side == WHITE) ? (8) : (-8);
    // Set left direction (for captures)
    int left = (side == WHITE) ? (7) : (-9);
    // Set right direction (for captures)
    int right = (side == WHITE) ? (9) : (-7);

    /* ---------- Single pushes (non-promoting) ---------- */
    
    // Single push every pawn on the bitboard if the square in front is empty and is not on the promotion rank
    Bitboard non_promo_single_pushes = (side == WHITE)
        ? (pawns << 8) & empty & ~promotion_rank
        : (pawns >> 8) & empty & ~promotion_rank;
    
    // For each generated signle push on the bitboard    
    while(non_promo_single_pushes) {
        // Get the first generated single push on the bitboard
        int to = get_lsb(non_promo_single_pushes);
        // Compute the square from which the pawn pushed from
        int from = to - forward;
        // Add the move to the list
        SAFE_ADD_MOVE(list, from, to, QUIET);
        // Remove the move from the bitboard
        pop_lsb(non_promo_single_pushes);
    }

    /* ---------- Double pushes (non-promoting) ---------- */

    // Double push every pawn on the bitboard if the pawn is on the 2nd (white) or 7th (black) rank and both of the two squares in front are empty
    Bitboard double_pushes = (side == WHITE)
        ? ((((pawns & RANK_X(2)) << 8) & empty) << 8) & empty
        : ((((pawns & RANK_X(7)) >> 8) & empty) >> 8) & empty;
    
    // For each generated double push on the bitboard
    while(double_pushes) {
        // Get the first generated double push on the bitboard
        int to = get_lsb(double_pushes);
        // Compute the square from which the pawn pushed from
        int from = to - forward * 2;
        // Add the move to the list
        SAFE_ADD_MOVE(list, from, to, DOUBLE_PUSH);
        // Remove the move from the bitboard
        pop_lsb(double_pushes);
    }

    /* ---------- Captures (left, non-promoting) ---------- */

    // Compute left captures for every pawn on the bitboard if the square diagonally left is occupied by an opponent's piece, is not the enemy king, and is not on the promotion rank
    Bitboard non_promo_left_captures = (side == WHITE)
        ? (pawns & ~FILE_X(1)) << 7 & opp & ~enemy_king & ~promotion_rank
        : (pawns & ~FILE_X(1)) >> 9 & opp & ~enemy_king & ~promotion_rank;
    
    // For each generated left capture on the bitboard
    while(non_promo_left_captures) {
        // Get the first generated left capture on the bitboard
        int to = get_lsb(non_promo_left_captures);
        // Compute the square from which the pawn captured from
        int from = to - left;
        // Add the move to the list
        SAFE_ADD_MOVE(list, from, to, CAPTURE);
        // Remove the move from the bitboard
        pop_lsb(non_promo_left_captures);
    }

    /* ---------- Captures (right, non-promoting) ---------- */

    // Compute right captures for every pawn on the bitboard if the square diagonally right is occupied by an opponent's piece, is not the enemy king, and is not on the promotion rank
    Bitboard non_promo_right_captures = (side == WHITE)
        ? (pawns & ~FILE_X(8)) << 9 & opp & ~enemy_king & ~promotion_rank
        : (pawns & ~FILE_X(8)) >> 7 & opp & ~enemy_king & ~promotion_rank;
    
    // For each generated right capture on the bitboard
    while(non_promo_right_captures) {
        // Get the first generated right capture on the bitboard
        int to = get_lsb(non_promo_right_captures);
        // Compute the square from which the pawn captured from
        int from = to - right;
        // Add the move to the list
        SAFE_ADD_MOVE(list, from, to, CAPTURE);
        // Remove the move from the bitboard
        pop_lsb(non_promo_right_captures);
    }

    /* ---------- Promotions (push) ---------- */

    // Push and promote every pawn on the bitboard if the square in front is empty and is on the promotion rank
    Bitboard push_promotions = (side == WHITE)
        ? (pawns << 8) & empty & promotion_rank
        : (pawns >> 8) & empty & promotion_rank;

    // For each generated push promotion on the bitboard
    while(push_promotions) {
        // Get the first generated push promotion on the bitboard
        int to = get_lsb(push_promotions);
        // Compute the square from which the pawn pushed from
        int from = to - forward;
        // Add the moves to the list
        SAFE_ADD_MOVE(list, from, to, PROMOTE_N);
        SAFE_ADD_MOVE(list, from, to, PROMOTE_B);
        SAFE_ADD_MOVE(list, from, to, PROMOTE_R);
        SAFE_ADD_MOVE(list, from, to, PROMOTE_Q);
        // Remove the move from the bitboard
        pop_lsb(push_promotions);
    }

    /* ---------- Promotions (left-capturing) ---------- */

    // Compute left capture promotions for every pawn on the bitboard if the square diagonally left is occupied by an opponent's piece, is not the enemy king, and is on the promotion rank
    Bitboard left_cap_promotions = (side == WHITE)
        ? (pawns & ~FILE_X(1)) << 7 & opp & ~enemy_king & promotion_rank
        : (pawns & ~FILE_X(1)) >> 9 & opp & ~enemy_king & promotion_rank;
    
    // For each generated left capture promotion on the bitboard
    while(left_cap_promotions) {
        // Get the first generated left capture promotion on the bitboard
        int to = get_lsb(left_cap_promotions);
        // Compute the square from which the pawn captured from
        int from = to - left;
        // Add the moves to the list
        SAFE_ADD_MOVE(list, from, to, PROMOTE_N_CAPTURE);
        SAFE_ADD_MOVE(list, from, to, PROMOTE_B_CAPTURE);
        SAFE_ADD_MOVE(list, from, to, PROMOTE_R_CAPTURE);
        SAFE_ADD_MOVE(list, from, to, PROMOTE_Q_CAPTURE);
        // Remove the move from the bitboard        
        pop_lsb(left_cap_promotions);
    }

    /* ---------- Promotions (right-capturing) ---------- */

    // Compute right capture promotions for every pawn on the bitboard if the square diagonally right is occupied by an opponent's piece, is not the enemy king, and is on the promotion rank
    Bitboard right_cap_promotions = (side == WHITE)
        ? (pawns & ~FILE_X(8)) << 9 & opp & ~enemy_king & promotion_rank
        : (pawns & ~FILE_X(8)) >> 7 & opp & ~enemy_king & promotion_rank;
    
    // For each generated right capture promotion on the bitboard
    while(right_cap_promotions) {
        // Get the first generated right capture promotion on the bitboard
        int to = get_lsb(right_cap_promotions);
        // Compute the square from which the pawn captured from
        int from = to - right;
        // Add the moves to the list
        SAFE_ADD_MOVE(list, from, to, PROMOTE_N_CAPTURE);
        SAFE_ADD_MOVE(list, from, to, PROMOTE_B_CAPTURE);
        SAFE_ADD_MOVE(list, from, to, PROMOTE_R_CAPTURE);
        SAFE_ADD_MOVE(list, from, to, PROMOTE_Q_CAPTURE);
        // Remove the move from the bitboard
        pop_lsb(right_cap_promotions);
    }

    /* ---------- En passant ---------- */

    // Check if en passant is available
    if (pos->en_passant != -1) {
        // Get the en passant square as a bitboard
        Bitboard ep_square = 1ULL << pos->en_passant;

        // Left-capturing en passant moves
        Bitboard ep_left = (side == WHITE)
            ? (pawns & ~FILE_X(1)) << 7
            : (pawns & ~FILE_X(1)) >> 9;
        
        // Check if the pawn that left-captured en passant occupies the en passant square
        if (ep_left & ep_square) {
            int to = pos->en_passant;
            int from = to - left;
            SAFE_ADD_MOVE(list, from, to, EN_PASSANT);
        }
        
        // Right-capturing en passant moves
        Bitboard ep_right = (side == WHITE)
            ? (pawns & ~FILE_X(8)) << 9
            : (pawns & ~FILE_X(8)) >> 7;

        // Check if the pawn that right-captured en passant occupies the en passant square
        if (ep_right & ep_square) {
            int to = pos->en_passant;
            int from = to - right;
            SAFE_ADD_MOVE(list, from, to, EN_PASSANT);
        }
    }
}

// Function to generate moves for all knights on the bitboard
static inline void generate_knight_moves(const Position* pos, MoveList* list, int side) {
    if (!pos || !list) return;
    // Get the knight bitboard for the current side
    Bitboard knights = pos->pieces[side == WHITE ? WN : BN];
    // Get the occupied squares for the current side
    Bitboard own = pos->occupied[side];
    // Get the occupied squares for the opponent
    Bitboard opp = pos->occupied[side ^ 1];
    // Get the enemy king bitboard
    Bitboard enemy_king = pos->pieces[side ? WK : BK];

    // For each knight on the bitboard
    while (knights) {
        // Get the square of the first knight on the bitboard (start checking set bits from a1)
        int from = get_lsb(knights);

        // Skip if the square is out of bounds
        if (!ON_BOARD(from)) {
            pop_lsb(knights);
            continue;
        }

        // Get all knight moves from the current square, removing moves to own pieces and the enemy king
        Bitboard all_knight_moves = knight_attacks(from) & ~own & ~enemy_king;
        // For each generated knight move on the bitboard
        while (all_knight_moves) {
            // Get the target square from the knight moves
            int to = get_lsb(all_knight_moves);
            // Remove the move if out of bounds
            if (!ON_BOARD(to)) {
                pop_lsb(all_knight_moves);
                continue;
            }
            // Determine if the move is a capture or a quiet move
            int flag = (opp & (1ULL << to)) ? CAPTURE : QUIET;
            // Add the move to the list 
            SAFE_ADD_MOVE(list, from, to, flag); 
            // Remove the move from the bitboard
            pop_lsb(all_knight_moves); 
        }
        // Remove the knight from the bitboard to iterate to the next knight
        pop_lsb(knights); 
    }
}

// Function to generate moves for all bishops on the bitboard
static inline void generate_bishop_moves(const Position* pos, MoveList* list, int side, const MagicData* magic) {
    if (!pos || !list) return;
   // Get the bishop bitboard for the current side
    Bitboard bishops = pos->pieces[side == WHITE ? WB : BB];
    // Get the occupied squares for the current side
    Bitboard own = pos->occupied[side];
    // Get the occupied squares for the opponent
    Bitboard opp = pos->occupied[side ^ 1];
    // Get all occupied squares
    Bitboard all = pos->occupied[ALL];
    // Get the enemy king bitboard
    Bitboard enemy_king = pos->pieces[side ? WK : BK];

    // For each bishop on the bitboard
    while (bishops) {
        // Get the square of the first bishop on the bitboard (start checking set bits from a1)
        int from = get_lsb(bishops);
        // Skip if the square is out of bounds
        if (!ON_BOARD(from)) {
            pop_lsb(bishops);
            continue;
        }
        
        // Get all bishop moves from the current square, removing moves to own pieces and the enemy king
        Bitboard all_bishop_moves = bishop_attacks(from, all, magic) & ~own & ~enemy_king;

        // For each generated bishop move on the bitboard
        while (all_bishop_moves) {
            // Get the target square from the bishop moves
            int to = get_lsb(all_bishop_moves);
            // Remove the move if out of bounds
            if (!ON_BOARD(to)) {
                pop_lsb(all_bishop_moves);
                continue;
            }
            // Determine if the move is a capture or a quiet move
            int flag = (opp & (1ULL << to)) ? CAPTURE : QUIET;
            // Add the move to the list
            SAFE_ADD_MOVE(list, from, to, flag);
            // Remove the move from the bitboard
            pop_lsb(all_bishop_moves);
        }
        // Remove the bishop from the bitboard to iterate to the next bishop
        pop_lsb(bishops);
    }
}

// Function to generate moves for all rooks on the bitboard
static inline void generate_rook_moves(const Position* pos, MoveList* list, int side, const MagicData* magic) {
    if (!pos || !list) return;
    // Get the rook bitboard for the current side
    Bitboard rooks = pos->pieces[side == WHITE ? WR : BR];
    // Get the occupied squares for the current side
    Bitboard own = pos->occupied[side];
    // Get the occupied squares for the opponent
    Bitboard opp = pos->occupied[side ^ 1];
    // Get all occupied squares
    Bitboard all = pos->occupied[ALL];
    // Get the enemy king bitboard
    Bitboard enemy_king = pos->pieces[side ? WK : BK];
    
    // For each rook on the bitboard
    while (rooks) {
        // Get the square of the first rook on the bitboard (start checking set bits from a1)
        int from = get_lsb(rooks);
        // Skip if the square is out of bounds
        if (!ON_BOARD(from)) {
            pop_lsb(rooks);
            continue;
        }

        // Get all rook moves from the current square, removing moves to own pieces and the enemy king                    
        Bitboard all_rook_moves = rook_attacks(from, all, magic) & ~own & ~enemy_king; 
        
        // For each generated rook move on the bitboard
        while (all_rook_moves) {
            // Get the target square from the rook moves
            int to = get_lsb(all_rook_moves);
            // Remove the move if out of bounds
            if (!ON_BOARD(to)) {
                pop_lsb(all_rook_moves);
                continue;
            }
            // Determine if the move is a capture or a quiet move
            int flag = (opp & (1ULL << to)) ? CAPTURE : QUIET;
            // Add the move to the list
            SAFE_ADD_MOVE(list, from, to, flag);
            // Remove the move from the bitboard
            pop_lsb(all_rook_moves);
        }
        // Remove the rook from the bitboard to iterate to the next rook
        pop_lsb(rooks);
    }
}

// Function to generate moves for all queens on the bitboard
static inline void generate_queen_moves(const Position* pos, MoveList* list, int side, const MagicData* magic) {
    if (!pos || !list) return;
    // Get the queen bitboard for the current side
    Bitboard queens = pos->pieces[side == WHITE ? WQ : BQ];
    // Get the occupied squares for the current side
    Bitboard own = pos->occupied[side];
    // Get the occupied squares for the opponent
    Bitboard opp = pos->occupied[side ^ 1];
    // Get all occupied squares
    Bitboard all = pos->occupied[ALL];
    // Get the enemy king bitboard
    Bitboard enemy_king = pos->pieces[side ? WK : BK];

    // For each queen on the bitboard
    while (queens) {
        // Get the square of the first queen on the bitboard (start checking set bits from a1)
        int from = get_lsb(queens);
        // Skip if the square is out of bounds
        if (!ON_BOARD(from)) {
            pop_lsb(queens);
            continue;
        }

        // Get all queen moves from the current square, removing moves to own pieces and the enemy king
        Bitboard all_queen_moves = queen_attacks(from, all, magic) & ~own & ~enemy_king;
        
        // For each generated queen move on the bitboard
        while (all_queen_moves) {
            // Get the target square from the queen moves
            int to = get_lsb(all_queen_moves);
            // Remove the move if out of bounds
            if (!ON_BOARD(to)) {
                pop_lsb(all_queen_moves);
                continue;
            }
            // Determine if the move is a capture or a quiet move
            int flag = (opp & (1ULL << to)) ? CAPTURE : QUIET;
            // Add the move to the list
            SAFE_ADD_MOVE(list, from, to, flag);
            // Remove the move from the bitboard
            pop_lsb(all_queen_moves);
        }
        // Remove the queen from the bitboard to iterate to the next queen
        pop_lsb(queens);
    }
}

// Function to generate moves for all kings on the bitboard
static inline void generate_king_moves(const Position* pos, MoveList* list, int side, const MagicData* magic) {
    if (!pos || !list) return;

    Bitboard king_bb = pos->pieces[side == WHITE ? WK : BK];
    Bitboard own = pos->occupied[side];
    Bitboard opp = pos->occupied[side ^ 1];
    Bitboard enemy_king = pos->pieces[side ? WK : BK];

    while (king_bb) {
        int from = get_lsb(king_bb);
        Bitboard moves = king_attacks(from) & ~own & ~enemy_king;

        while (moves) {
            int to = get_lsb(moves);
            int flag = (opp & (1ULL << to)) ? CAPTURE : QUIET;
            SAFE_ADD_MOVE(list, from, to, flag);
            pop_lsb(moves);
        }

        /* ---------- Castling ---------- */

        int king_from = from;
        Bitboard occupied = pos->occupied[ALL];
        if (!(pos->pieces[side == WHITE ? WK : BK] & (1ULL << king_from))) {
            pop_lsb(king_bb);
            continue;
        }

        // Loop over both castling sides (0 = K-side, 1 = Q-side)
        for (int i = 0; i < 2; i++) {
            int castling_rights = (side == WHITE)
                           ? (i == 0 ? WHITE_QUEENSIDE : WHITE_KINGSIDE)
                           : (i == 0 ? BLACK_QUEENSIDE : BLACK_KINGSIDE);
            int rook_index = (side == WHITE ? WHITE_QUEENSIDE_ROOK : BLACK_QUEENSIDE_ROOK) + i;
            int king_to = (side == WHITE)
                        ? (i == 0 ? C1 : G1)
                        : (i == 0 ? C8 : G8);

            if (!(pos->castling_rights & castling_rights)) continue;

            int rook_from = pos->rook_from[rook_index];

            Bitboard rook_bb = 1ULL << rook_from;
            Bitboard rook_piece_bb = pos->pieces[side == WHITE ? WR : BR];
            if (!(rook_piece_bb & rook_bb)) continue; // Rook missing

            // Step 1: Path between king and rook must be clear
            Bitboard between_kr = squares_between_exclusive(king_from, rook_from);
            if (occupied & between_kr) continue;

            // Step 2: King's path must be safe and clear
            int min_sq = (king_from < king_to) ? king_from : king_to;
            int max_sq = (king_from > king_to) ? king_from : king_to;
            Bitboard king_path = squares_between_exclusive(min_sq, max_sq) | (1ULL << king_to) | (1ULL << king_from);

            if (is_square_attacked(pos, king_from, side ^ 1, magic)) continue;

            bool safe = true;
            Bitboard path = king_path;
            while (path) {
                int sq = get_lsb(path);
                if (is_square_attacked(pos, sq, side ^ 1, magic)) {
                    safe = false;
                    break;
                }
                pop_lsb(path);
            }

            if (!safe) continue;

            // Passed all checks – add the castling move
            int flag = (i == 0) ? CASTLE_QUEENSIDE : CASTLE_KINGSIDE;
            SAFE_ADD_MOVE(list, king_from, king_to, flag);
        }

        pop_lsb(king_bb);
    }
}

// Check, checkmate, and stalemate detection
int is_in_check(const Position* pos, int side, const MagicData* magic);
int is_in_checkmate(const Position* pos, int side, const MagicData* magic, const ZobristKeys* keys);
int is_in_stalemate(const Position* pos, int side, const MagicData* magic, const ZobristKeys* keys);

// Make and unmake move
int make_move(Position* pos, MoveState* state, int move, ZobristKeys* keys);
int unmake_move(Position* pos, const MoveState* state, ZobristKeys* keys);

/* ---------- Move generation functions ---------- */

static inline int is_legal_move(const Position* pos, int move, const MagicData* magic, ZobristKeys* keys) {
    if (!pos || move == 0) return 0;

    // Create a shallow copy of the Position struct
    Position temp = *pos;
    MoveState state;
    memcpy(&temp, pos, sizeof(Position));
    if (!make_move(&temp, &state, move, keys)) {
        return 0; // illegal move due to malformed input
    }

    // printf("[DEBUG] After move %d:\n", move);
    // print_position(&temp);

    // After move, is king in check? make_move() only switches the side to move in the temp position, so we don't have to switch back
    int in_check = is_in_check(&temp, temp.side_to_move ^ 1, magic);
    // move_to_string(move);
    // printf("Still in check? %d\n", in_check);

    return !in_check;
}

static inline void generate_pseudo_legal_moves(const Position* pos, MoveList* list, int side, const MagicData* magic) {
    if (!pos || !list) return;
    memset(list->moves, 0, MAX_MOVES * sizeof(int));
    list->count = 0; // Reset move count

    // Generate moves for each piece type
    generate_pawn_moves(pos, list, side);
    generate_knight_moves(pos, list, side);
    generate_bishop_moves(pos, list, side, magic);
    generate_rook_moves(pos, list, side, magic);
    generate_queen_moves(pos, list, side, magic);
    generate_king_moves(pos, list, side, magic);
}

void generate_legal_moves(const Position* pos, MoveList* list, int side, const MagicData* magic, const ZobristKeys* keys);

#endif