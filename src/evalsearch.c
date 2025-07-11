#include "board.h"
#include "evalsearch.h"
#include "movegen.h"
#include "tt.h"
#include "zobrist.h"
#include <stdio.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))

int killer_moves[MAX_PLY][2];

int history_table[64][64];

int piece_values[] = {
    100, 300, 300, 500, 900, 10000
};

const int futility_margin[] = {
    0, 100, 250, 400
};

#define PHASE_MAX 16

const int phase_values[6] = {
    0, // Pawn
    1, // Knight
    1, // Bishop
    2, // Rook
    4, // Queen
    0  // King
};

const int mg_value[6] = {
    82, 337, 365, 477, 1025, 0
}; // Middlegame

const int eg_value[6] = {
    94, 281, 297, 512, 936, 0
}; // Endgame

const int pawn_pst_mg[64] = {
    0,   0,   0,   0,   0,   0,  0,   0,
    -35,  -1, -20, -23, -15,  24, 38, -22,
    -26,  -4,  -4, -10,   3,   3, 33, -12,
    -27,  -2,  -5,  12,  17,   6, 10, -25,
    -14,  13,   6,  21,  23,  12, 17, -23,
    -6,   7,  26,  31,  65,  56, 25, -20,
    98, 134,  61,  95,  68, 126, 34, -11,
      0,   0,   0,   0,   0,   0,  0,   0
};

const int pawn_pst_eg[64] = {
    0,   0,   0,   0,   0,   0,   0,   0,
    13,   8,   8,  10,  13,   0,   2,  -7,
    4,   7,  -6,   1,   0,  -5,  -1,  -8,
    13,   9,  -3,  -7,  -7,  -8,   3,  -1,
    32,  24,  13,   5,  -2,   4,  17,  17,
    94, 100,  85,  67,  56,  53,  82,  84,
    178, 173, 158, 134, 147, 132, 165, 187,
      0,   0,   0,   0,   0,   0,   0,   0
};

const int knight_pst_mg[64] = {
    -105, -21, -58, -33, -17, -28, -19,  -23,
    -29, -53, -12,  -3,  -1,  18, -14,  -19,
    -23,  -9,  12,  10,  19,  17,  25,  -16,
    -13,   4,  16,  13,  28,  19,  21,   -8,
    -9,  17,  19,  53,  37,  69,  18,   22,
    -47,  60,  37,  65,  84, 129,  73,   44,
    -73, -41,  72,  36,  23,  62,   7,  -17,
    -167, -89, -34, -49,  61, -97, -15, -107
};

const int knight_pst_eg[64] = {
    -29, -51, -23, -15, -22, -18, -50, -64,
    -42, -20, -10,  -5,  -2, -20, -23, -44,
    -23,  -3,  -1,  15,  10,  -3, -20, -22,
    -18,  -6,  16,  25,  16,  17,   4, -18,
    -17,   3,  22,  22,  22,  11,   8, -18,
    -24, -20,  10,   9,  -1,  -9, -19, -41,
    -25,  -8, -25,  -2,  -9, -25, -24, -52,
    -58, -38, -13, -28, -31, -27, -63, -99
};

const int bishop_pst_mg[64] = {
    -33,  -3, -14, -21, -13, -12, -39, -21,
    4,  15,  16,   0,   7,  21,  33,   1,
    0,  15,  15,  15,  14,  27,  18,  10,
    -6,  13,  13,  26,  34,  12,  10,   4,
    -4,   5,  19,  50,  37,  37,   7,  -2,
    -16,  37,  43,  40,  35,  50,  37,  -2,
    -26,  16, -18, -13,  30,  59,  18, -47,
    -29,   4, -82, -37, -25, -42,   7,  -8
};

const int bishop_pst_eg[64] = {
    -23,  -9, -23,  -5, -9, -16,  -5, -17,
    -14, -18,  -7,  -1,  4,  -9, -15, -27,
    -12,  -3,   8,  10, 13,   3,  -7, -15,
    -6,   3,  13,  19,  7,  10,  -3,  -9,
    -3,   9,  12,   9, 14,  10,   3,   2,
    2,  -8,   0,  -1, -2,   6,   0,   4,
    -8,  -4,   7, -12, -3, -13,  -4, -14,
    -14, -21, -11,  -8, -7,  -9, -17, -24
};

const int rook_pst_mg[64] = {
    -19, -13,   1,  17, 16,  7, -37, -26,
    -44, -16, -20,  -9, -1, 11,  -6, -71,
    -45, -25, -16, -17,  3,  0,  -5, -33,
    -36, -26, -12,  -1,  9, -7,   6, -23,
    -24, -11,   7,  26, 24, 35,  -8, -20,
    -5,  19,  26,  36, 17, 45,  61,  16,
    27,  32,  58,  62, 80, 67,  26,  44,
    32,  42,  32,  51, 63,  9,  31,  43
};

const int rook_pst_eg[64] = {
    -9,  2,  3, -1, -5, -13,   4, -20,
    -6, -6,  0,  2, -9,  -9, -11,  -3,
    -4,  0, -5, -1, -7, -12,  -8, -16,
    3,  5,  8,  4, -5,  -6,  -8, -11,
    4,  3, 13,  1,  2,   1,  -1,   2,
    7,  7,  7,  5,  4,  -3,  -5,  -3,
    11, 13, 13, 11, -3,   3,   8,   3,
    13, 10, 18, 15, 12,  12,   8,   5
};

const int queen_pst_mg[64] = {
    -1, -18,  -9,  10, -15, -25, -31, -50,
    -35,  -8,  11,   2,   8,  15,  -3,   1,
    -14,   2, -11,  -2,  -5,   2,  14,   5,
    -9, -26,  -9, -10,  -2,  -4,   3,  -3,
    -27, -27, -16, -16,  -1,  17,  -2,   1,
    -13, -17,   7,   8,  29,  56,  47,  57,
    -24, -39,  -5,   1, -16,  57,  28,  54,
    -28,   0,  29,  12,  59,  44,  43,  45
};

const int queen_pst_eg[64] = {
    -33, -28, -22, -43,  -5, -32, -20, -41,
    -22, -23, -30, -16, -16, -23, -36, -32,
    -16, -27,  15,   6,   9,  17,  10,   5,
    -18,  28,  19,  47,  31,  34,  39,  23,
    3,  22,  24,  45,  57,  40,  57,  36,
    -20,   6,   9,  49,  47,  35,  19,   9,
    -17,  20,  32,  41,  58,  25,  30,   0,
    -9,  22,  22,  27,  27,  19,  10,  20
};

const int king_pst_mg[64] = {
    -15,  36,  12, -54,   8, -28,  24,  14,
    1,   7,  -8, -64, -43, -16,   9,   8,
    -14, -14, -22, -46, -44, -30, -15, -27,
    -49,  -1, -27, -39, -46, -44, -33, -51,
    -17, -20, -12, -27, -30, -25, -14, -36,
    -9,  24,   2, -16, -20,   6,  22, -22,
    29,  -1, -20,  -7,  -8,  -4, -38, -29,
    -65,  23,  16, -15, -56, -34,   2,  13
};

const int king_pst_eg[64] = {
    -53, -34, -21, -11, -28, -14, -24, -43,
    -27, -11,   4,  13,  14,   4,  -5, -17,
    -19,  -3,  11,  21,  23,  16,   7,  -9,
    -18,  -4,  21,  24,  27,  23,   9, -11,
    -8,  22,  24,  27,  26,  33,  26,   3,
    10,  17,  23,  15,  20,  45,  44,  13,
    -12,  17,  14,  17,  17,  38,  23,  11,
    -74, -35, -18, -18, -11,  15,   4, -17
};

#define MAX_REP_HISTORY 1024
uint64_t repetition_table[MAX_REP_HISTORY];
int repetition_index = 0;

bool is_threefold_repetition(uint64_t hash) {
    int count = 0;
    for (int i = 0; i < repetition_index; i++) {
        if (repetition_table[i] == hash) {
            if (++count >= 2) return true;  // third occurrence
        }
    }
    return false;
}

int compute_phase(const Position* pos) {
    int phase = PHASE_MAX;

    for (int sq = 0; sq < 64; sq++) {
        int piece = get_piece_on_square(pos, sq);
        if (piece == -1) continue;

        int type = piece % 6;
        if (type == P || type == K) continue;

        phase -= phase_values[type];
    }

    if (phase < 0) phase = 0;
    if (phase > PHASE_MAX) phase = PHASE_MAX;
    return phase;
}

void make_null_move(Position* pos, ZobristKeys* keys) {
    pos->zobrist_hash ^= keys->zobrist_side;
    pos->side_to_move ^= 1;
    pos->en_passant = -1;  // Null move cancels en passant
}

void unmake_null_move(Position* pos, ZobristKeys* keys) {
    pos->side_to_move ^= 1;
    pos->zobrist_hash ^= keys->zobrist_side;
}

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
    int mg_score = 0;
    int eg_score = 0;

    int phase = compute_phase(pos) * 256 / PHASE_MAX;

    for (int sq = 0; sq < 64; sq++) {
        int piece = get_piece_on_square(pos, sq);
        if (piece == -1) continue;

        int color = (piece < 6) ? WHITE : BLACK;
        int type = piece % 6;
        int sq_mirrored = (color == WHITE) ? sq : ((sq ^ 56)); // flip vertically

        // Material + PST
        int mg = mg_value[type];
        int eg = eg_value[type];

        switch (type) {
            case P:
                mg += pawn_pst_mg[sq_mirrored];
                eg += pawn_pst_eg[sq_mirrored];
                break;
            case N:
                mg += knight_pst_mg[sq_mirrored];
                eg += knight_pst_eg[sq_mirrored];
                break;
            case B:
                mg += bishop_pst_mg[sq_mirrored];
                eg += bishop_pst_eg[sq_mirrored];
                break;
            case R:
                mg += rook_pst_mg[sq_mirrored];
                eg += rook_pst_eg[sq_mirrored];
                break;
            case Q:
                mg += queen_pst_mg[sq_mirrored];
                eg += queen_pst_eg[sq_mirrored];
                break;
            case K:
                mg += king_pst_mg[sq_mirrored];
                eg += king_pst_eg[sq_mirrored];
        }

        // Add to score with sign depending on color
        if (color == pos->side_to_move) {
            mg_score += mg;
            eg_score += eg;
        } else {
            mg_score -= mg;
            eg_score -= eg;
        }
    }

    // Interpolate final score
    return (mg_score * phase + eg_score * (256 - phase)) >> 8;
}

int see(const Position* pos, int move, const MagicData* magic) {
    int from = MOVE_FROM(move);
    int to = MOVE_TO(move);
    int side = pos->side_to_move;
    int opp = side ^ 1;

    int captured_piece = get_piece_on_square(pos, to);
    if (captured_piece < 0 || captured_piece >= 12) return 0;

    int gain[32] = {0};
    int depth = 0;

    Bitboard occ = pos->occupied[WHITE] | pos->occupied[BLACK];
    Bitboard attackers = get_attackers_to(pos, to, occ, magic);

    // Remove moving piece
    attackers &= ~(1ULL << from);
    occ &= ~(1ULL << from);

    gain[depth++] = piece_values[captured_piece % 6];

    int stm = opp;

    while (true) {
        int least_val_piece = -1;
        int attacker_sq = -1;

        for (int pt = P; pt <= K; pt++) {
            Bitboard pieces = pos->pieces[stm * 6 + pt] & attackers;
            if (pieces) {
                attacker_sq = __builtin_ctzll(pieces);
                least_val_piece = pt;
                break;
            }
        }

        if (attacker_sq == -1 || least_val_piece == -1) break;

        if (depth >= 31) break; // prevent out-of-bounds
        if (least_val_piece < 0 || least_val_piece > 5) break;

        gain[depth] = piece_values[least_val_piece] - gain[depth - 1];

        if (gain[depth] < 0)
            break;

        depth++;
        occ &= ~(1ULL << attacker_sq);
        attackers = get_attackers_to(pos, to, occ, magic);
        stm ^= 1;
    }

    while (--depth > 0) {
        gain[depth - 1] = -MAX(-gain[depth - 1], gain[depth]);
    }

    return gain[0];
}

int quiescence(Position* pos, int alpha, int beta, const MagicData* magic, ZobristKeys* keys) {

    // if (is_threefold_repetition(pos->zobrist_hash)) {
    //     return 0;
    // }
    // int old_index = repetition_index;
    // repetition_table[repetition_index++] = pos->zobrist_hash;

    int stand_pat = evaluation(pos);

    if (stand_pat >= beta)
        return beta;  // fail-hard beta cutoff
    if (stand_pat > alpha)
        alpha = stand_pat;

    MoveList captures;
    generate_legal_moves(pos, &captures, pos->side_to_move, magic, keys);

    // Filter to only captures (and optionally promotions)
    int count = 0;
    for (int i = 0; i < captures.count; i++) {
        int flag = MOVE_FLAG(captures.moves[i]);
        if (flag == CAPTURE ||
            (flag >= PROMOTE_N_CAPTURE && flag <= PROMOTE_Q_CAPTURE)) {
            captures.moves[count++] = captures.moves[i];  // keep
        }
    }
    captures.count = count;

    // Order captures using MVV-LVA
    for (int i = 0; i < captures.count - 1; i++) {
        for (int j = i + 1; j < captures.count; j++) {
            int m1 = captures.moves[i], m2 = captures.moves[j];

            int from1 = MOVE_FROM(m1), to1 = MOVE_TO(m1);
            int from2 = MOVE_FROM(m2), to2 = MOVE_TO(m2);
            int att1 = get_piece_on_square(pos, from1), cap1 = get_piece_on_square(pos, to1);
            int att2 = get_piece_on_square(pos, from2), cap2 = get_piece_on_square(pos, to2);

            int score1 = piece_values[cap1 % 6] * 10 - piece_values[att1 % 6];
            int score2 = piece_values[cap2 % 6] * 10 - piece_values[att2 % 6];

            if (score2 > score1) {
                int tmp = captures.moves[i];
                captures.moves[i] = captures.moves[j];
                captures.moves[j] = tmp;
            }
        }
    }

    MoveState state;
    for (int i = 0; i < captures.count; i++) {
        int move = captures.moves[i];

        // Delta pruning
        int to = MOVE_TO(move);
        int captured = get_piece_on_square(pos, to);
        if (stand_pat + piece_values[captured % 6] + 100 < alpha) {
            continue;
        }

        // SEE filtering: skip losing captures
        if (see(pos, move, magic) < 0) {
            continue;
        }

        if (!make_move(pos, &state, move, keys))
            continue;

        int score = -quiescence(pos, -beta, -alpha, magic, keys);

        unmake_move(pos, &state, keys);

        if (score >= beta)
            return beta;
        if (score > alpha)
            alpha = score;
    }

    // repetition_index = old_index;

    return alpha;
}

int search(Position* pos, int depth, int ply, int alpha, int beta, const MagicData* magic, ZobristKeys* keys) {
    int original_alpha = alpha;
    int best_move = 0;
    int stand_pat = 0;

    // Threefold repetition check
    if (is_threefold_repetition(pos->zobrist_hash)) {
        int eval = evaluation(pos);
        return eval > 0 ? -50 : 0; // Draw score
    }

    // Push zobrist hash to repetition stack
    int old_index = repetition_index;
    repetition_table[repetition_index++] = pos->zobrist_hash;

    // TT PROBE
    int tt_score;
    if (tt_probe(pos->zobrist_hash, depth, alpha, beta, &tt_score, &best_move)) {
        repetition_index = old_index;  // Undo stack push
        return tt_score;
    }

    // Leaf node → Quiescence
    if (depth == 0) {
        repetition_index = old_index;
        return quiescence(pos, alpha, beta, magic, keys);
    }

    // Null Move Pruning
    if (depth >= 3 && !is_in_check(pos, pos->side_to_move, magic)) {
        make_null_move(pos, keys);
        int score = -search(pos, depth - 3, ply + 1, -beta, -beta + 1, magic, keys); // null reduction = 2
        unmake_null_move(pos, keys);
        if (score >= beta) {
            repetition_index = old_index;
            return beta;
        }
    }

    // Extended futility pruning setup
    int can_futility_prune = 0;
    if (depth <= 3 && !is_in_check(pos, pos->side_to_move, magic)) {
        stand_pat = evaluation(pos);
        can_futility_prune = 1;
    }

    // Generate and order moves
    MoveList list;
    generate_legal_moves(pos, &list, pos->side_to_move, magic, keys);

    // Prioritize TT best move
    if (best_move != 0) {
        for (int i = 0; i < list.count; i++) {
            if (list.moves[i] == best_move) {
                int temp = list.moves[0];
                list.moves[0] = best_move;
                list.moves[i] = temp;
                break;
            }
        }
    }

    sort_moves(pos, &list, ply);

    // Check for mate/stalemate
    if (list.count == 0) {
        repetition_index = old_index;
        return is_in_check(pos, pos->side_to_move, magic) ? -MATE_SCORE + ply : DRAW_SCORE;
    }

    int best_score = -MATE_SCORE;
    MoveState state;

    for (int i = 0; i < list.count; i++) {
        int move = list.moves[i];
        int flag = MOVE_FLAG(move);
        int is_capture = (flag == CAPTURE ||
                          flag == PROMOTE_N_CAPTURE || flag == PROMOTE_B_CAPTURE ||
                          flag == PROMOTE_R_CAPTURE || flag == PROMOTE_Q_CAPTURE);

        // Extended Futility Pruning
        if (can_futility_prune && !is_capture &&
            stand_pat + futility_margin[depth] <= alpha)
        {
            continue;
        }

        if (!make_move(pos, &state, move, keys)) continue;

        int score;
        if (depth >= 3 && i >= 3 && !is_capture && !is_in_check(pos, pos->side_to_move ^ 1, magic)) {
            // Late Move Reduction (LMR)
            score = -search(pos, depth - 2, ply + 1, -alpha - 1, -alpha, magic, keys);
            if (score > alpha) {
                score = -search(pos, depth - 1, ply + 1, -beta, -alpha, magic, keys);
            }
        } else {
            score = -search(pos, depth - 1, ply + 1, -beta, -alpha, magic, keys);
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
            // History and Killer Heuristics
            int from = MOVE_FROM(move);
            int to   = MOVE_TO(move);

            if (!is_capture) {
                history_table[from][to] += depth * depth;

                if (killer_moves[ply][0] != move) {
                    killer_moves[ply][1] = killer_moves[ply][0];
                    killer_moves[ply][0] = move;
                }
            }

            break; // Beta cutoff
        }
    }

    // Store in TT
    TTFlag flag = (best_score <= original_alpha) ? TT_ALPHA :
                  (best_score >= beta)           ? TT_BETA :
                                                    TT_EXACT;
    tt_store(pos->zobrist_hash, depth, best_score, best_move, flag);

    // Undo repetition stack
    repetition_index = old_index;

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