#include "board.h"
#include "evalsearch.h"
#include "movegen.h"
#include "tt.h"
#include "zobrist.h"
#include <stdio.h>

int killer_moves[MAX_PLY][2];

int history_table[64][64];

int piece_values[] = {
    100, 300, 300, 500, 900, 10000
};

const int pawn_pst_mg[64] = {
    0,  0,  0,  0,  0,  0,  0,  0,
    50, 50, 50, 50, 50, 50, 50, 50,
    10, 10, 20, 30, 30, 20, 10, 10,
    5,  5, 10, 25, 25, 10,  5,  5,
    0,  0,  0, 20, 20,  0,  0,  0,
    5, -5,-10,  0,  0,-10, -5,  5,
    5, 10, 10,-20,-20, 10, 10,  5,
    0,  0,  0,  0,  0,  0,  0,  0
};

const int pawn_pst_eg[64] = {
    0,  0,  0,  0,  0,  0,  0,  0,
    10, 10, 10, 10, 10, 10, 10, 10,
    10, 20, 20, 30, 30, 20, 20, 10,
    20, 20, 30, 40, 40, 30, 20, 20,
    30, 30, 40, 50, 50, 40, 30, 30,
    40, 40, 50, 60, 60, 50, 40, 40,
    50, 50, 60, 70, 70, 60, 50, 50,
    0,  0,  0,  0,  0,  0,  0,  0
};

const int knight_pst_mg[64] = {
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50
};

const int knight_pst_eg[64] = {
    -40,-30,-20,-20,-20,-20,-30,-40,
    -30,-10, 10, 15, 15, 10,-10,-30,
    -20, 10, 20, 25, 25, 20, 10,-20,
    -20, 15, 25, 30, 30, 25, 15,-20,
    -20, 15, 25, 30, 30, 25, 15,-20,
    -20, 10, 20, 25, 25, 20, 10,-20,
    -30,-10, 10, 15, 15, 10,-10,-30,
    -40,-30,-20,-20,-20,-20,-30,-40
};

const int bishop_pst_mg[64] = {
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -20,-10,-10,-10,-10,-10,-10,-20
};

const int bishop_pst_eg[64] = {
    -10,-5,-5,-5,-5,-5,-5,-10,
    -5,10,10,10,10,10,10,-5,
    -5,10,15,15,15,15,10,-5,
    -5,10,15,20,20,15,10,-5,
    -5,10,15,20,20,15,10,-5,
    -5,10,15,15,15,15,10,-5,
    -5,10,10,10,10,10,10,-5,
    -10,-5,-5,-5,-5,-5,-5,-10
};

const int rook_pst_mg[64] = {
    0,  0,  0,  0,  0,  0,  0,  0,
    5, 10, 10, 10, 10, 10, 10,  5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    0,  0,  0,  5,  5,  0,  0,  0
};

const int rook_pst_eg[64] = {
    0,  0,  5, 10, 10,  5,  0,  0,
    0,  0, 10, 15, 15, 10,  0,  0,
    0,  0, 15, 20, 20, 15,  0,  0,
    0,  0, 15, 20, 20, 15,  0,  0,
    0,  0, 15, 20, 20, 15,  0,  0,
    0,  0, 10, 15, 15, 10,  0,  0,
    0,  0,  5, 10, 10,  5,  0,  0,
    0,  0,  0,  5,  5,  0,  0,  0
};

const int queen_pst_mg[64] = {
    -20,-10,-10, -5, -5,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5,  5,  5,  5,  0,-10,
    -5,  0,  5,  5,  5,  5,  0, -5,
    0,  0,  5,  5,  5,  5,  0, -5,
    -10,  5,  5,  5,  5,  5,  0,-10,
    -10,  0,  5,  0,  0,  0,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20
};

const int queen_pst_eg[64] = {
    -20,-10,-10, -5, -5,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5,  5,  5,  5,  0,-10,
    -5,  0,  5,  5,  5,  5,  0, -5,
    0,  0,  5,  5,  5,  5,  0, -5,
    -10,  5,  5,  5,  5,  5,  0,-10,
    -10,  0,  5,  0,  0,  0,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20
};

const int king_pst_mg[64] = {
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -10,-20,-20,-20,-20,-20,-20,-10,
    20, 20,  0,  0,  0,  0, 20, 20,
    20, 30, 10,  0,  0, 10, 30, 20
};

const int king_pst_eg[64] = {
    -50,-40,-30,-20,-20,-30,-40,-50,
    -30,-20,-10,  0,  0,-10,-20,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-30,  0,  0,  0,  0,-30,-30,
    -50,-30,-30,-30,-30,-30,-30,-50
};

#define MATE_SCORE 100000
#define DRAW_SCORE 0

void sort_moves(Position* pos, MoveList* list, int ply) {
    for (int i = 0; i < list->count - 1; i++) {
        for (int j = i + 1; j < list->count; j++) {
            if (move_order_heuristic(pos, list->moves[j], ply) > move_order_heuristic(pos, list->moves[i], ply)) {
                int tmp = list->moves[i];
                list->moves[i] = list->moves[j];
                list->moves[j] = tmp;
            }
        }
    }
}

int move_order_heuristic(const Position* pos, int move, int ply) {
    int flag = MOVE_FLAG(move);

    if (flag == CASTLE_KINGSIDE || flag == CASTLE_QUEENSIDE) {
        return 850000;  // Just below killer moves but above captures
    }

    // Killer moves
    if (move == killer_moves[ply][0]) return 900000;
    if (move == killer_moves[ply][1]) return 800000;

    if (flag == CAPTURE || flag == PROMOTE_N_CAPTURE || flag == PROMOTE_B_CAPTURE || flag == PROMOTE_R_CAPTURE || flag == PROMOTE_Q_CAPTURE) {
        int from = MOVE_FROM(move);
        int to = MOVE_TO(move);
        int captured_piece = get_piece_on_square(pos, to);
        int attacker_piece = get_piece_on_square(pos, from);
        return 10 * piece_values[captured_piece] - piece_values[attacker_piece] + MATE_SCORE;
    }

    // Quiet move: history heuristic
    int from = MOVE_FROM(move);
    int to = MOVE_TO(move);
    return history_table[from][to];
}

int evaluation(const Position* pos) {
    int eval_score = 0;

    for (int sq = 0; sq < 64; sq++) {
        int piece = get_piece_on_square(pos, sq);
        if (piece == -1) continue;

        int side = (piece < 6) ? WHITE : BLACK;
        int type = piece % 6;
        int value = piece_values[type];

        // Add PSQT score
        int pst_score = 0;
        int mirrored_sq = (side == WHITE) ? sq : (56 ^ (sq & 56)) | (sq & 7);  // vertical flip

        switch (type) {
            case P: pst_score = pawn_pst_mg[mirrored_sq]; break;
            case N: pst_score = knight_pst_mg[mirrored_sq]; break;
            case B: pst_score = bishop_pst_mg[mirrored_sq]; break;
            case R: pst_score = rook_pst_mg[mirrored_sq]; break;
            case Q: pst_score = queen_pst_mg[mirrored_sq]; break;
            case K: pst_score = king_pst_mg[mirrored_sq]; break;
        }

        int score = value + pst_score;
        eval_score += (side == pos->side_to_move) ? score : -score;
    }

    return eval_score;
}

int search(Position* pos, int depth, int ply, int alpha, int beta, const MagicData* magic, ZobristKeys* keys) {
    int original_alpha = alpha;
    int best_move = 0;

    // TT PROBE
    // int tt_score;
    // if (tt_probe(pos->zobrist_hash, depth, alpha, beta, &tt_score, &best_move))
    //     return tt_score;

    if (depth == 0) {
        return evaluation(pos);  // No magic needed here yet
    }

    MoveList list;
    generate_legal_moves(pos, &list, pos->side_to_move, magic, keys);  // if your movegen needs magic

    // Move TT best move to the front if present
    // if (best_move != 0) {
    //     for (int i = 0; i < list.count; i++) {
    //         if (list.moves[i] == best_move) {
    //             // Swap with first move
    //             int temp = list.moves[0];
    //             list.moves[0] = best_move;
    //             list.moves[i] = temp;
    //             break;
    //         }
    //     }
    // }

    sort_moves(pos, &list, ply);

    if (list.count == 0) {
        if (is_in_check(pos, pos->side_to_move, magic)) {
            return -MATE_SCORE + ply;  // Distance to mate (maximize delay)
        } else {
            return DRAW_SCORE;  // Stalemate
        }
    }

    int best_score = -MATE_SCORE;
    MoveState state;

    for (int i = 0; i < list.count; i++) {
        int move = list.moves[i];

        if (!make_move(pos, &state, move, keys)) continue;

        int score = -search(pos, depth - 1, ply + 1, -beta, -alpha, magic, keys);

        if (state.has_castled) {
            score += 200;  // e.g., 40
        }

        unmake_move(pos, &state, keys);

        if (score > best_score) {
            best_score = score;
            best_move = move;
        }

        if (score > alpha) {
            alpha = score;
        }

        if (alpha >= beta) {
            // Extract from and to squares from move
            int from = MOVE_FROM(move);
            int to = MOVE_TO(move);
            int flag = MOVE_FLAG(move);

            if (flag != CAPTURE && flag != PROMOTE_N_CAPTURE && flag != PROMOTE_B_CAPTURE && flag != PROMOTE_R_CAPTURE && flag != PROMOTE_Q_CAPTURE) {
                history_table[from][to] += depth * depth;  // quiet move history update

                // Killer moves
                if (killer_moves[ply][0] != move) {
                    killer_moves[ply][1] = killer_moves[ply][0];
                    killer_moves[ply][0] = move;
                }
            }

            break;
        }
    }

    // Store in TT
    TTFlag flag = (best_score <= original_alpha) ? TT_ALPHA :
                  (best_score >= beta)           ? TT_BETA :
                                                   TT_EXACT;
    tt_store(pos->zobrist_hash, depth, best_score, best_move, flag);

    return best_score;
}

int find_best_move(Position* pos, int depth, const MagicData* magic, ZobristKeys* keys) {
    for (int i = 0; i < MAX_PLY; i++) {
        killer_moves[i][0] = 0;
        killer_moves[i][1] = 0;
    }
    for (int from = 0; from < 64; from++) {
        for (int to = 0; to < 64; to++) {
            history_table[from][to] = 0;
        }
    }

    MoveList list;
    generate_legal_moves(pos, &list, pos->side_to_move, magic, keys);

    if (list.count == 0) {
        // No legal moves -> checkmate or stalemate
        if (is_in_check(pos, pos->side_to_move, magic)) {
            printf("Checkmate detected for side %d\n", pos->side_to_move);
            // Return NO_MOVE or handle end-of-game logic here
        } else {
            printf("Stalemate detected for side %d\n", pos->side_to_move);
            // Handle draw/stalemate similarly
        }
        return -1;  // Indicate no moves available
    }

    int best_score = -MATE_SCORE;
    int best_move = 0;
    MoveState state;

    for (int i = 0; i < list.count; i++) {
        int move = list.moves[i];
        if (!make_move(pos, &state, move, keys)) continue;

        int score = -search(pos, depth - 1, 1, -MATE_SCORE, MATE_SCORE, magic, keys);
        unmake_move(pos, &state, keys);

        if (score > best_score) {
            best_score = score;
            best_move = move;
        }
    }

    return best_move;
}