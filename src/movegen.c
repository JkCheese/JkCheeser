#include "board.h"
#include "magic.h"
#include "moveformat.h"
#include "movegen.h"
#include "operations.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

// Is a given square attacked by a given side?
int is_square_attacked(const Position* pos, int sq, int attacking_side, const MagicData* magic) {

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
    int from = MOVE_FROM(move);
    int to = MOVE_TO(move);
    int flag = MOVE_FLAG(move);

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
    if (pos->en_passant != -1)
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
        pos->king_from[side] = to;
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
        // Update castled state
        state->has_castled = true;
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
    if (flag == DOUBLE_PUSH) {
        pos->en_passant = (side == WHITE) ? to - 8 : to + 8;
    }
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

void generate_legal_moves(const Position* pos, MoveList* list, int side, const MagicData* magic) {
    if (!pos || !list) return;
    memset(list->moves, 0, MAX_MOVES * sizeof(int));
    list->count = 0; // Reset move count

    MoveList pseudo = {0};
    // Generate moves for each piece type
    generate_pawn_moves(pos, &pseudo, side);
    generate_knight_moves(pos, &pseudo, side);
    generate_bishop_moves(pos, &pseudo, side, magic);
    generate_rook_moves(pos, &pseudo, side, magic);
    generate_queen_moves(pos, &pseudo, side, magic);
    generate_king_moves(pos, &pseudo, side, magic);
    // printf("Pseudo moves count: %d\n", pseudo.count);

    for (int i = 0; i < pseudo.count; ++i) {
        int move = pseudo.moves[i];

        // printf("Checking move from %d to %d\n", from, to);

        if (is_legal_move(pos, move, side, magic)) {
            list->moves[list->count++] = move;
            // printf("[generate_legal_moves] Legal move: %d\n", move);
            // printf("Legal move added\n");
        } // else printf("Illegal move skipped\n");
    }

    // printf("[generate_legal_moves] Total legal moves: %d\n", list->count);
}