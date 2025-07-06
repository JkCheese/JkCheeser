#include "board.h"
#include "magic.h"
#include "moveformat.h"
#include "movegen.h"
#include "operations.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

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

// Is the given side in check?
int is_in_check(const Position* pos, int side, const MagicData* magic) {
    int king_sq = pos->king_from[side];
    // printf("King square: %d\n", king_sq);
    // printf("[DEBUG] is_in_check: side=%s, king_sq=%d\n",
           // side == WHITE ? "WHITE" : "BLACK", king_sq);

    // printf("[is_in_check] King on square: %d, Side: %s\n", king_sq, side == WHITE ? "White" : "Black");

    int attacked = is_square_attacked(pos, king_sq, side ^ 1, magic);
    // printf("[DEBUG] square %d attacked by %s? %s\n",
           // king_sq, side ^ 1 ? "BLACK" : "WHITE", attacked ? "YES" : "NO");

    // printf("[is_in_check] Result: %s\n", attacked ? "CHECK" : "SAFE");

    return attacked;
}

// Is the given side in checkmate?
int is_in_checkmate(const Position* pos, int side, const MagicData* magic) {

    // Check if the given side is in check: if not, they can't be in checkmate either
    if (!is_in_check(pos, side, magic)) return 0;

    // Generate all legal moves and return whether there are none
    MoveList legal;
    generate_legal_moves(pos, &legal, side, magic);
    return legal.count == 0;
}

// Is the given side in stalemate?
int is_in_stalemate(const Position* pos, int side, const MagicData* magic) {

    // Check if the given side is in check: if yes, they can't be in stalemate
    if (is_in_check(pos, side, magic)) return 0;

    // Generate all legal moves and return whether there are none
    MoveList legal;
    generate_legal_moves(pos, &legal, side, magic);
    return legal.count == 0;
}

// Function to make a move
int make_move(Position* pos, MoveState* state, int move) {

    // If either the position or state pointers point to nothing, or the encoded move has no value, don't make the move
    if (!pos || !state || move == 0) {
        printf("[make_move] Invalid input or zero move\n");
        return 0;
    }

    // Decode the move
    int from = (move >> 6) & 0x3f;
    int to = move & 0x3f;
    int flag = (move >> 12) & 0xf;

    // Initialize bitboards with a bit set at the from and to square, respectively 
    Bitboard from_bb = 1ULL << from;
    Bitboard to_bb = 1ULL << to;

    // Grab the side to move
    int side = pos->side_to_move;
    // Set the piece that will be moved 
    int moved_piece = get_piece_on_square(pos, from);
    // If there is no piece, don't make the move
    if (moved_piece == -1) {
        printf("[make_move] No piece on from-square %d\n", from);
        return 0;
    }
    // if (side == BLACK) printf("Moved piece: %d\n", moved_piece);
    // Set the piece that will be captured, checking if it's an en passant capture
    int captured_piece = (flag == EN_PASSANT)
        ? get_piece_on_square(pos, (side == WHITE) ? (to - 8) : (to + 8))
        : get_piece_on_square(pos, to);

    // Set the piece that will be promoted
    int promoted_piece = -1;

    // Save this information in a MoveState structure to restore the position later with unmake_move()
    *state = (MoveState){
        .from = from,
        .to = to,
        .moved_piece = moved_piece,
        .captured_piece = captured_piece,
        .flag = flag,
        .side_to_move = side,
        .en_passant = pos->en_passant,
        .castling_rights = pos->castling_rights,
        .halfmove_clock = pos->halfmove_clock,
        .fullmove_number = pos->fullmove_number,
        .promoted_piece = -1,
        .king_sq[WHITE] = pos->king_from[WHITE],
        .king_sq[BLACK] = pos->king_from[BLACK]
    };

    memcpy(state->rook_from_before, pos->rook_from, sizeof(pos->rook_from));

    // Reset en passant square
    pos->en_passant = -1;

    // For 50-move-rule: if a piece was captured or a pawn was moved, reset the halfmove clock
    if (captured_piece != -1 || (moved_piece & 7) == P)
        pos->halfmove_clock = 0;
    // Otherwise, increment it
    else
        pos->halfmove_clock++;

    // If both sides have completed a move, increment the full move number
    if (side == BLACK)
        pos->fullmove_number++;

    // Remove the moved piece
    pos->pieces[moved_piece] &= ~from_bb;
    pos->occupied[side] &= ~from_bb;

    // Handle capture
    if (captured_piece == WK || captured_piece == BK) {
        printf("ERROR: King illegally captured at %d\n", to);
        return 0;
    } else if (flag == EN_PASSANT && captured_piece != -1) {
        int cap_sq = (side == WHITE) ? to - 8 : to + 8;
        Bitboard cap_bb = 1ULL << cap_sq;
        pos->pieces[captured_piece] &= ~cap_bb;
        pos->occupied[!side] &= ~cap_bb;
    } else if (captured_piece != -1) {
        pos->pieces[captured_piece] &= ~to_bb;
        pos->occupied[!side] &= ~to_bb;
    }

    // Handle promotion
    if ((flag >= PROMOTE_N && flag <= PROMOTE_Q) ||
    (flag >= PROMOTE_N_CAPTURE && flag <= PROMOTE_Q_CAPTURE)) {

        int promote_index = (flag >= PROMOTE_N_CAPTURE)
                            ? flag - PROMOTE_N_CAPTURE
                            : flag - PROMOTE_N;

        promoted_piece = (side == WHITE ? WN : BN) + promote_index;
        pos->pieces[promoted_piece] |= to_bb;
        pos->occupied[side] |= to_bb;
        state->promoted_piece = promoted_piece;

    } else {
        pos->pieces[moved_piece] |= to_bb;
        pos->occupied[side] |= to_bb;
    }

    // Handle castling
    if (flag == CASTLE_KINGSIDE || flag == CASTLE_QUEENSIDE) {
        int rook = (side == WHITE) ? WR : BR;
        int index = (side == WHITE ? 0 : 2) + (flag == CASTLE_KINGSIDE);
        int rook_from = pos->rook_from[index];
        int rook_to = pos->rook_to[index];
        Bitboard rf_bb = 1ULL << rook_from;
        Bitboard rt_bb = 1ULL << rook_to;

        pos->pieces[rook] &= ~rf_bb;
        pos->pieces[rook] |= rt_bb;
        pos->occupied[side] &= ~rf_bb;
        pos->occupied[side] |= rt_bb;
        // Update rook_from[] for castling to reflect that the rook has moved
        pos->rook_from[index] = rook_to;
    }

    // Clear castling rights if rook moves or is captured
    if (moved_piece == WR) {
        if (from == pos->rook_from[WHITE_QUEENSIDE_ROOK]) pos->castling_rights &= ~WHITE_QUEENSIDE;
        else if (from == pos->rook_from[WHITE_KINGSIDE_ROOK]) pos->castling_rights &= ~WHITE_KINGSIDE;
    }
    if (captured_piece == WR) {
        if (to == pos->rook_from[WHITE_QUEENSIDE_ROOK]) pos->castling_rights &= ~WHITE_QUEENSIDE;
        else if (to == pos->rook_from[WHITE_KINGSIDE_ROOK]) pos->castling_rights &= ~WHITE_KINGSIDE;
    }
    if (moved_piece == BR) {
        if (from == pos->rook_from[BLACK_QUEENSIDE_ROOK]) pos->castling_rights &= ~BLACK_QUEENSIDE;
        else if (from == pos->rook_from[BLACK_KINGSIDE_ROOK]) pos->castling_rights &= ~BLACK_KINGSIDE;
    }
    if (captured_piece == BR) {
        if (to == pos->rook_from[BLACK_QUEENSIDE_ROOK]) pos->castling_rights &= ~BLACK_QUEENSIDE;
        else if (to == pos->rook_from[BLACK_KINGSIDE_ROOK]) pos->castling_rights &= ~BLACK_KINGSIDE;
    }

    // Update king square and disable castling rights
    if (moved_piece == WK) {
        pos->king_from[WHITE] = to;
        pos->castling_rights &= ~(WHITE_KINGSIDE | WHITE_QUEENSIDE);
    }
    if (moved_piece == BK) {
        pos->king_from[BLACK] = to;
        pos->castling_rights &= ~(BLACK_KINGSIDE | BLACK_QUEENSIDE);
    }

    // Update occupied and side
    pos->occupied[ALL] = pos->occupied[WHITE] | pos->occupied[BLACK];
    pos->side_to_move = side ^ 1;

    // Double pawn push: set en passant square
    if (flag == DOUBLE_PUSH)
        pos->en_passant = (side == WHITE) ? to - 8 : to + 8;

    return 1;
}

int unmake_move(Position* pos, const MoveState* state) {
    if (!pos || !state) return 0;

    int from = state->from;
    int to = state->to;
    int flag = state->flag;
    int side = state->side_to_move;

    Bitboard from_bb = 1ULL << from;
    Bitboard to_bb = 1ULL << to;

    // printf("[unmake_move] BEFORE undo:\n");
    // print_bitboard(pos->pieces[WK]);
    // print_bitboard(pos->occupied[WHITE]);
    // print_bitboard(pos->occupied[BLACK]);

    int moved_piece = state->moved_piece;
    int captured_piece = state->captured_piece;
    int promoted_piece = state->promoted_piece;

    pos->side_to_move = side;
    pos->en_passant = state->en_passant;
    pos->castling_rights = state->castling_rights;
    pos->halfmove_clock = state->halfmove_clock;
    pos->fullmove_number = state->fullmove_number;

    // Remove piece from destination
    if (promoted_piece != -1) {
        pos->pieces[promoted_piece] &= ~to_bb;
    } else {
        pos->pieces[moved_piece] &= ~to_bb;
    }
    pos->occupied[side] &= ~to_bb;

    // Restore piece to source
    pos->pieces[moved_piece] |= from_bb;
    pos->occupied[side] |= from_bb;

    // Restore king square
    pos->king_from[WHITE] = state->king_sq[WHITE];
    pos->king_from[BLACK] = state->king_sq[BLACK];

    // Restore captured piece
    // Fix en passant: restore captured pawn
    if (flag == EN_PASSANT && captured_piece != -1) {
        int cap_sq = (side == WHITE) ? to - 8 : to + 8;
        Bitboard cap_bb = 1ULL << cap_sq;
        pos->pieces[captured_piece] |= cap_bb;
        pos->occupied[!side] |= cap_bb;
    } else if (captured_piece != -1) {
        pos->pieces[captured_piece] |= to_bb;
        pos->occupied[!side] |= to_bb;
    }

    // Undo castling
    if (flag == CASTLE_KINGSIDE || flag == CASTLE_QUEENSIDE) {
        int rook = (side == WHITE) ? WR : BR;
        int index = (side == WHITE ? 0 : 2) + (flag == CASTLE_KINGSIDE);

        // Restore rook position from saved state
        int rook_from = state->rook_from_before[index];  // Original rook square
        int rook_to   = pos->rook_to[index];             // Where rook was moved to

        Bitboard rf_bb = 1ULL << rook_from;
        Bitboard rt_bb = 1ULL << rook_to;

        // Move rook back
        pos->pieces[rook] &= ~rt_bb;
        pos->pieces[rook] |= rf_bb;
        pos->occupied[side] &= ~rt_bb;
        pos->occupied[side] |= rf_bb;

        // Restore rook_from[] now
        memcpy(pos->rook_from, state->rook_from_before, sizeof(pos->rook_from));
    }

    // Recompute full occupancy
    pos->occupied[ALL] = pos->occupied[WHITE] | pos->occupied[BLACK];

    // printf("[unmake_move] AFTER undo:\n");
    // print_bitboard(pos->pieces[WK]);
    // print_bitboard(pos->occupied[WHITE]);
    // print_bitboard(pos->occupied[BLACK]);

    return 1;
}

int is_legal_move(Position* pos, int move, int side, const MagicData* magic) {
    if (!pos || move == 0) return 0;

    // If the king is the piece moving, we need to track its new square
    int from = (move >> 6) & 0x3F;
    int to = move & 0x3F;
    int moved_piece = get_piece_on_square(pos, from);

    // Create a shallow copy of the Position struct
    Position temp = *pos;
    MoveState state;
    memcpy(&temp, pos, sizeof(Position));
    if (!make_move(&temp, &state, move)) {
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

void generate_legal_moves(const Position* pos, MoveList* list, int side, const MagicData* magic) {
    if (!pos || !list) return;

    MoveList pseudo;
    generate_pseudo_legal_moves(pos, &pseudo, side, magic);
    // printf("Pseudo moves count: %d\n", pseudo.count);

    list->count = 0;

    for (int i = 0; i < pseudo.count; ++i) {
        int move = pseudo.moves[i];
        int from = (move >> 6) & 0x3F;
        int to = move & 0x3F;

        // printf("Checking move from %d to %d\n", from, to);

        if (is_legal_move(pos, move, side, magic)) {
            list->moves[list->count++] = move;
            // printf("[generate_legal_moves] Legal move: %d\n", move);
            // printf("Legal move added\n");
        } // else printf("Illegal move skipped\n");
    }

    // printf("[generate_legal_moves] Total legal moves: %d\n", list->count);
}

void generate_pseudo_legal_moves(const Position* pos, MoveList* list, int side, const MagicData* magic) {
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

// Function to generate moves for all pawns on the bitboard
static inline void generate_pawn_moves(const Position* pos, MoveList* list, int side) {
    if (!pos || !list) return;
    // Get the pawn bitboard for the current side
    Bitboard pawns = pos->pieces[side == WHITE ? WP : BP];
    // Get the occupied squares for the current side
    Bitboard own = pos->occupied[side];
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
        non_promo_single_pushes = pop_lsb(non_promo_single_pushes);
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
        double_pushes = pop_lsb(double_pushes);
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
        non_promo_left_captures = pop_lsb(non_promo_left_captures);
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
        non_promo_right_captures = pop_lsb(non_promo_right_captures);
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
        push_promotions = pop_lsb(push_promotions);
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
        left_cap_promotions = pop_lsb(left_cap_promotions);
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
        right_cap_promotions = pop_lsb(right_cap_promotions);
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

// Function to generate moves for any given knight on any given square on the bitboard
Bitboard knight_attacks(int sq) {

    // Initialize the knight bitboard
    Bitboard knight = 0;
    // Set the knight to be on the given square
    knight = set_bit(knight, sq);
    
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
        // Initialize the target square
        int to = 0;
        // Skip if the square is out of bounds
        if (!ON_BOARD(from)) {
            knights = pop_lsb(knights);
            continue;
        }

        // Get all knight moves from the current square, removing moves to own pieces and the enemy king
        Bitboard all_knight_moves = knight_attacks(from) & ~own & ~enemy_king;
        // For each generated knight move on the bitboard
        while (all_knight_moves) {
            // Get the target square from the knight moves
            to = get_lsb(all_knight_moves);
            // Remove the move if out of bounds
            if (!ON_BOARD(to)) {
                all_knight_moves = pop_lsb(all_knight_moves);
                continue;
            }
            // Determine if the move is a capture or a quiet move
            int flag = (opp & (1ULL << to)) ? CAPTURE : QUIET;
            // Add the move to the list 
            SAFE_ADD_MOVE(list, from, to, flag); 
            // Remove the move from the bitboard
            all_knight_moves = pop_lsb(all_knight_moves); 
        }
        // Remove the knight from the bitboard to iterate to the next knight
        knights = pop_lsb(knights); 
    }
}

Bitboard bishop_attacks(int sq, Bitboard occupancy, const MagicData* magic) {
    Bitboard blockers = occupancy & magic->bishop_masks[sq];
    uint64_t index = (blockers * magic->bishop_magics[sq]) >> (64 - magic->bishop_shifts[sq]);
    return magic->bishop_attack_table[sq][index];
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
        // Initialize the target square
        int to = 0;
        // Skip if the square is out of bounds
        if (!ON_BOARD(from)) {
            bishops = pop_lsb(bishops);
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
                all_bishop_moves = pop_lsb(all_bishop_moves);
                continue;
            }
            // Determine if the move is a capture or a quiet move
            int flag = (opp & (1ULL << to)) ? CAPTURE : QUIET;
            // Add the move to the list
            SAFE_ADD_MOVE(list, from, to, flag);
            // Remove the move from the bitboard
            all_bishop_moves = pop_lsb(all_bishop_moves);
        }
        // Remove the bishop from the bitboard to iterate to the next bishop
        bishops = pop_lsb(bishops);
    }
}

// Runtime rook attack lookup
Bitboard rook_attacks(int sq, Bitboard occupancy, const MagicData* magic) {
    Bitboard blockers = occupancy & magic->rook_masks[sq];
    uint64_t index = (blockers * magic->rook_magics[sq]) >> (64 - magic->rook_shifts[sq]);
    return magic->rook_attack_table[sq][index];
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
        // Initialize the target square
        int to = 0;
        // Skip if the square is out of bounds
        if (!ON_BOARD(from)) {
            rooks = pop_lsb(rooks);
            continue;
        }

        // Get all rook moves from the current square, removing moves to own pieces and the enemy king                    
        Bitboard all_rook_moves = rook_attacks(from, all, magic) & ~own & ~enemy_king; 
        
        // For each generated rook move on the bitboard
        while (all_rook_moves) {
            // Get the target square from the rook moves
            to = get_lsb(all_rook_moves);
            // Remove the move if out of bounds
            if (!ON_BOARD(to)) {
                all_rook_moves = pop_lsb(all_rook_moves);
                continue;
            }
            // Determine if the move is a capture or a quiet move
            int flag = (opp & (1ULL << to)) ? CAPTURE : QUIET;
            // Add the move to the list
            SAFE_ADD_MOVE(list, from, to, flag);
            // Remove the move from the bitboard
            all_rook_moves = pop_lsb(all_rook_moves);
        }
        // Remove the rook from the bitboard to iterate to the next rook
        rooks = pop_lsb(rooks);
    }
}

Bitboard queen_attacks(int sq, Bitboard occupancy, const MagicData* magic) {
    return rook_attacks(sq, occupancy, magic) | bishop_attacks(sq, occupancy, magic);
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
        // Initialize the target square
        int to = 0;
        // Skip if the square is out of bounds
        if (!ON_BOARD(from)) {
            queens = pop_lsb(queens);
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
                all_queen_moves = pop_lsb(all_queen_moves);
                continue;
            }
            // Determine if the move is a capture or a quiet move
            int flag = (opp & (1ULL << to)) ? CAPTURE : QUIET;
            // Add the move to the list
            SAFE_ADD_MOVE(list, from, to, flag);
            // Remove the move from the bitboard
            all_queen_moves = pop_lsb(all_queen_moves);
        }
        // Remove the queen from the bitboard to iterate to the next queen
        queens = pop_lsb(queens);
    }
}

// Function to generate moves for the king
Bitboard king_attacks(int sq) {

    // Initialize the king bitboard
    Bitboard king = 0;
    // Set the king to be on the given square
    king = set_bit(king, sq);

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
            moves = pop_lsb(moves);
        }

        /* ---------- Castling ---------- */

        int king_from = from;
        Bitboard occupied = pos->occupied[ALL];
        if (!(pos->pieces[side == WHITE ? WK : BK] & (1ULL << king_from))) {
            king_bb = pop_lsb(king_bb);
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
            int rook_to = pos->rook_to[rook_index];

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
                path = pop_lsb(path);
            }

            if (!safe) continue;

            // Passed all checks – add the castling move
            int flag = (i == 0) ? CASTLE_QUEENSIDE : CASTLE_KINGSIDE;
            SAFE_ADD_MOVE(list, king_from, king_to, flag);
        }

        king_bb = pop_lsb(king_bb);
    }
}