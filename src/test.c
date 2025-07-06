#include "board.h"
#include "magic.h"
#include "moveformat.h"
#include "movegen.h"
#include "test.h"
#include <stdbool.h>
#include <stdio.h>

// Helper: Check if position is valid (e.g., king exists on board)
bool is_position_valid(const Position* pos) {
    if (pos->king_from[WHITE] < 0 || pos->king_from[WHITE] > 63) return false;
    if (pos->king_from[BLACK] < 0 || pos->king_from[BLACK] > 63) return false;
    // Add more sanity checks if you want
    return true;
}

uint64_t perft_debug(Position* pos, int depth, const MagicData* magic) {
    if (depth == 0) return 1;

    // printf("[DEBUG] Before move - Occupied bitboard:\n");
    // print_bitboard(pos->occupied[ALL]);

    MoveList list;
    generate_legal_moves(pos, &list, pos->side_to_move, magic);

    uint64_t nodes = 0;

    for (int i = 0; i < list.count; ++i) {
        int move = list.moves[i];
        MoveState state;

        // Before making move, save copy of position for comparison later
        Position pos_before = *pos;

        if (!make_move(pos, &state, move)) {
            printf("[DEBUG] make_move failed for move %d\n", move);
            continue;
        }

        // printf("[DEBUG] After move %d - Occupied bitboard:\n", i + 1);
        // print_bitboard(pos->occupied[ALL]);
        // print_bitboard(pos->pieces[BP]);
        // print_bitboard(pos->pieces[BN]);
        // print_bitboard(pos->pieces[BB]);
        // print_bitboard(pos->pieces[BR]);
        // print_bitboard(pos->pieces[BQ]);
        // printf("[DEBUG] After move - WQ rook position:\n");
        // Bitboard rook_to_WQ = 1ULL << pos->rook_from[0];
        // print_bitboard(rook_to_WQ);
        // printf("[DEBUG] After move - WK rook position:\n");
        // Bitboard rook_to_WK = 1ULL << pos->rook_from[1];
        // print_bitboard(rook_to_WK);
        // printf("[DEBUG] After move - BQ rook position:\n");
        // Bitboard rook_to_BQ = 1ULL << pos->rook_from[2];
        // print_bitboard(rook_to_BQ);
        // printf("[DEBUG] After move - BK rook position:\n");
        // Bitboard rook_to_BK = 1ULL << pos->rook_from[3];
        // print_bitboard(rook_to_BK);

        // Immediately check if king is in check for side that just moved (illegal if so)
        int moved_side = state.side_to_move;
        int opponent = moved_side ^ 1;
        if (is_in_check(pos, moved_side, magic)) {
            char san[16];
            move_to_san(&pos_before, move, san, magic);
            printf("[ILLEGAL MOVE DETECTED] Move %s leaves king in check at depth %d\n", san, depth);
            unmake_move(pos, &state);
            continue;
        }

        // Check if position still valid (optional)
        if (!is_position_valid(pos)) {
            char san[16];
            move_to_san(&pos_before, move, san, magic);
            printf("[INVALID POSITION] after move %s at depth %d\n", san, depth);
            unmake_move(pos, &state);
            continue;
        }

        nodes += perft_debug(pos, depth - 1, magic);
        
        // Undo move
        if (!unmake_move(pos, &state)) {
            printf("[DEBUG] unmake_move failed after move %d\n", move);
            // Could be serious bug - stop or break here if you want
        }

        // After undo, check if position restored exactly (optional deep check)
        // You can compare pos and pos_before here by memcmp or custom function
    }

    return nodes;
}

void perft_divide(Position* pos, int depth, const MagicData* magic) {
    MoveList list;
    generate_legal_moves(pos, &list, pos->side_to_move, magic);

    uint64_t total = 0;

    for (int i = 0; i < list.count; ++i) {
        int move = list.moves[i];
        int flag = (move >> 12) & 0xF;

        MoveState state;
        if (!make_move(pos, &state, move))
            continue;

        char from_str[3], to_str[3], uci_move[6];  // +1 for null terminator
        square_to_coords((move >> 6) & 0x3F, from_str);
        square_to_coords(move & 0x3F, to_str);
        from_str[2] = to_str[2] = 0;

        // Determine promotion character if applicable
        char promo = 0;
        if ((flag >= PROMOTE_N && flag <= PROMOTE_Q) ||
            (flag >= PROMOTE_N_CAPTURE && flag <= PROMOTE_Q_CAPTURE)) {
            int index = (flag >= PROMOTE_N_CAPTURE)
                        ? flag - PROMOTE_N_CAPTURE
                        : flag - PROMOTE_N;

            static const char promo_chars[] = {'n', 'b', 'r', 'q'};
            promo = promo_chars[index];
        }

        if (promo) {
            snprintf(uci_move, sizeof(uci_move), "%s%s%c", from_str, to_str, promo);
        } else {
            snprintf(uci_move, sizeof(uci_move), "%s%s", from_str, to_str);
        }

        uint64_t count = perft_debug(pos, depth - 1, magic);
        total += count;

        printf("%s: %llu\n", uci_move, count);

        unmake_move(pos, &state);
    }

    printf("Total: %llu\n", total);
}