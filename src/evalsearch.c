#include "board.h"
#include "evaluation.h"
#include "evalsearch.h"
#include "evalparams.h"
#include "movegen.h"
#include "tt.h"
#include "zobrist.h"
#include <stdio.h>
#include <time.h>

#define DRAW_SCORE 0
#define DRAW_PENALTY -20

#define MAX(a, b) ((a) > (b) ? (a) : (b))

int killer_moves[MAX_PLY][2];
int history_table[64][64];
int counter_moves[64][64]; // New: counter-move heuristic

int piece_values[] = {
    100, 300, 300, 500, 900, 10000
};

const int futility_margin[] = {
    0, 100, 250, 400
};

// New: Additional pruning margins
const int razor_margin[] = {0, 300, 300, 300};
const int reverse_futility_margin[] = {0, 200, 300, 500};

#define PHASE_MAX 24

const int phase_values[6] = {
    0, // Pawn
    1, // Knight
    1, // Bishop
    2, // Rook
    4, // Queen
    0  // King
};

#define MAX_REP_HISTORY 1024
uint64_t repetition_table[MAX_REP_HISTORY];
int repetition_index = 0;

// New: Move scoring constants
#define SCORE_TT_MOVE        1000000
#define SCORE_GOOD_CAPTURE    900000
#define SCORE_KILLER_1        800000
#define SCORE_KILLER_2        700000
#define SCORE_CASTLE          600000
#define SCORE_BAD_CAPTURE     100000
#define SCORE_QUIET              0

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

// Enhanced move scoring function
int score_move(const Position* pos, int move, int ply, int tt_move, const MagicData* magic) {
    if (move == tt_move) return SCORE_TT_MOVE;
    
    int flag = MOVE_FLAG(move);
    
    // Captures
    if (flag == CAPTURE || flag == PROMOTE_N_CAPTURE || flag == PROMOTE_B_CAPTURE || 
        flag == PROMOTE_R_CAPTURE || flag == PROMOTE_Q_CAPTURE) {
        
        int see_score = see(pos, move, magic);
        if (see_score >= 0) {
            int from = MOVE_FROM(move);
            int to = MOVE_TO(move);
            int captured = get_piece_on_square(pos, to);
            int attacker = get_piece_on_square(pos, from);
            return SCORE_GOOD_CAPTURE + 10 * piece_values[captured % 6] - piece_values[attacker % 6];
        } else {
            return SCORE_BAD_CAPTURE + see_score; // Bad captures sorted by SEE
        }
    }
    
    // Killer moves
    if (move == killer_moves[ply][0]) return SCORE_KILLER_1;
    if (move == killer_moves[ply][1]) return SCORE_KILLER_2;
    
    // Castling
    if (flag == CASTLE_KINGSIDE || flag == CASTLE_QUEENSIDE) {
        return SCORE_CASTLE;
    }
    
    // Promotions (non-capture)
    if (flag >= PROMOTE_N && flag <= PROMOTE_Q) {
        return SCORE_CASTLE + (flag - PROMOTE_N) * 100;
    }
    
    // History heuristic for quiet moves
    int from = MOVE_FROM(move);
    int to = MOVE_TO(move);
    return SCORE_QUIET + history_table[from][to];
}

// Enhanced move sorting (replaces sort_moves)
void sort_moves(Position* pos, MoveList* list, int ply, int tt_move, const MagicData* magic) {
    // Simple selection sort with enhanced scoring
    for (int i = 0; i < list->count - 1; i++) {
        int best_idx = i;
        int best_score = score_move(pos, list->moves[i], ply, tt_move, magic);
        
        for (int j = i + 1; j < list->count; j++) {
            int score = score_move(pos, list->moves[j], ply, tt_move, magic);
            if (score > best_score) {
                best_score = score;
                best_idx = j;
            }
        }
        
        if (best_idx != i) {
            int tmp = list->moves[i];
            list->moves[i] = list->moves[best_idx];
            list->moves[best_idx] = tmp;
        }
    }
}

// Keep your original move_order_heuristic for compatibility
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

// New: LMR reduction calculation
int get_lmr_reduction(int depth, int move_count, int is_pv, int is_capture, int gives_check) {
    if (depth < 3 || move_count < 4) return 0;
    if (is_capture || gives_check) return 0;
    
    int reduction = 1;
    
    // More reduction for later moves
    if (move_count > 6) reduction++;
    if (move_count > 15) reduction++;
    
    // Less reduction in PV nodes
    if (is_pv) reduction = (reduction + 1) / 2;
    
    // Ensure we don't reduce too much
    if (reduction >= depth) reduction = depth - 1;
    
    return reduction;
}

int quiescence(Position* pos, int alpha, int beta, const EvalParams* params, const MagicData* magic, ZobristKeys* keys) {
    int stand_pat = evaluation(pos, params, magic);

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

        int score = -quiescence(pos, -beta, -alpha, params, magic, keys);

        unmake_move(pos, &state, keys);

        if (score >= beta)
            return beta;
        if (score > alpha)
            alpha = score;
    }

    return alpha;
}

// Enhanced search function with PVS and improved pruning
int search(Position* pos, int depth, int ply, int alpha, int beta, int is_pv_node, const EvalParams* params, const MagicData* magic, ZobristKeys* keys) {
    int original_alpha = alpha;
    int best_move = 0;
    int stand_pat = 0;

    // Push zobrist hash to repetition stack
    int old_index = repetition_index;
    repetition_table[repetition_index++] = pos->zobrist_hash;

    // Repetition draw check
    if (is_threefold_repetition(pos->zobrist_hash)) {
        repetition_index = old_index;  // Undo Zobrist push before returning
        return DRAW_PENALTY;  // Avoid repetition in winning positions
    }

    // TT PROBE
    int tt_score;
    if (tt_probe(pos->zobrist_hash, depth, alpha, beta, &tt_score, &best_move)) {
        repetition_index = old_index;  // Undo stack push
        return tt_score;
    }

    // Leaf node → Quiescence
    if (depth == 0) {
        repetition_index = old_index;
        return quiescence(pos, alpha, beta, params, magic, keys);
    }

    // Check extension
    int in_check = is_in_check(pos, pos->side_to_move, magic);
    if (in_check) {
        depth++; // Extend search when in check
    }

    // Razoring - if we're way behind even after capturing something, drop to qsearch
    if (!is_pv_node && depth <= 3 && !in_check) {
        int eval = evaluation(pos, params, magic);
        if (eval + razor_margin[depth] <= alpha) {
            int razor_score = quiescence(pos, alpha, beta, params, magic, keys);
            if (razor_score <= alpha) {
                repetition_index = old_index;
                return razor_score;
            }
        }
    }

    // Reverse Futility Pruning (Static Null Move Pruning)
    if (!is_pv_node && depth <= 3 && !in_check) {
        int eval = evaluation(pos, params, magic);
        if (eval - reverse_futility_margin[depth] >= beta) {
            repetition_index = old_index;
            return eval; // Fail soft
        }
    }

    // Null Move Pruning
    if (!is_pv_node && depth >= 3 && !in_check) {
        make_null_move(pos, keys);
        int score = -search(pos, depth - 3, ply + 1, -beta, -beta + 1, 0, params, magic, keys); // null reduction = 2
        unmake_null_move(pos, keys);
        if (score >= beta) {
            repetition_index = old_index;
            return beta;
        }
    }

    // Extended futility pruning setup
    int can_futility_prune = 0;
    if (!is_pv_node && depth <= 3 && !in_check) {
        stand_pat = evaluation(pos, params, magic);
        can_futility_prune = 1;
    }

    // Generate and order moves
    MoveList list;
    generate_legal_moves(pos, &list, pos->side_to_move, magic, keys);

    // Use enhanced move ordering
    sort_moves(pos, &list, ply, best_move, magic);

    // Check for mate/stalemate
    if (list.count == 0) {
        repetition_index = old_index;
        return in_check ? -MATE_SCORE + ply : DRAW_SCORE;
    }

    int best_score = -MATE_SCORE;
    MoveState state;
    int found_pv = 0;

    for (int i = 0; i < list.count; i++) {
        int move = list.moves[i];
        int flag = MOVE_FLAG(move);
        int is_capture = (flag == CAPTURE ||
                          flag == PROMOTE_N_CAPTURE || flag == PROMOTE_B_CAPTURE ||
                          flag == PROMOTE_R_CAPTURE || flag == PROMOTE_Q_CAPTURE);

        // Extended Futility Pruning
        if (!is_pv_node && can_futility_prune && !is_capture &&
            stand_pat + futility_margin[depth] <= alpha)
        {
            continue;
        }

        if (!make_move(pos, &state, move, keys)) continue;

        int score;
        int child_is_pv = is_pv_node && (i == 0);
        int gives_check = is_in_check(pos, pos->side_to_move, magic);

        if (found_pv) {
            // PVS: Search with null window first
            score = -search(pos, depth - 1, ply + 1, -alpha - 1, -alpha, 0, params, magic, keys);
            
            // If it fails high, do a full re-search
            if (score > alpha && score < beta) {
                score = -search(pos, depth - 1, ply + 1, -beta, -alpha, child_is_pv, params, magic, keys);
            }
        } else {
            // First move or not yet found PV: full window search
            if (depth >= 3 && i >= 3 && !is_capture && !gives_check) {
                // Enhanced LMR
                int reduction = get_lmr_reduction(depth, i, is_pv_node, is_capture, gives_check);
                score = -search(pos, depth - 1 - reduction, ply + 1, -alpha - 1, -alpha, 0, params, magic, keys);
                if (score > alpha) {
                    score = -search(pos, depth - 1, ply + 1, -beta, -alpha, child_is_pv, params, magic, keys);
                }
            } else {
                score = -search(pos, depth - 1, ply + 1, -beta, -alpha, child_is_pv, params, magic, keys);
            }
        }

        unmake_move(pos, &state, keys);

        if (score > best_score) {
            best_score = score;
            best_move = move;
        }

        if (score > alpha) {
            alpha = score;
            found_pv = 1;
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

// Enhanced find_best_move with iterative deepening
int find_best_move(Position* pos, int max_depth, const EvalParams* params,
                   const MagicData* magic, ZobristKeys* keys,
                   int* mate_line, int* mate_length) {
    for (int i = 0; i < MAX_PLY; i++) {
        killer_moves[i][0] = 0;
        killer_moves[i][1] = 0;
    }
    for (int from = 0; from < 64; from++) {
        for (int to = 0; to < 64; to++) {
            history_table[from][to] = 0;
            counter_moves[from][to] = 0;
        }
    }

    MoveList list;
    generate_legal_moves(pos, &list, pos->side_to_move, magic, keys);

    if (list.count == 0) {
        return 0;
    }

    int best_move = 0;
    int best_score = -MATE_SCORE;

    for (int depth = 1; depth <= max_depth; depth++) {
        int current_best_move = 0;
        int current_best_score = -MATE_SCORE;
        MoveState state;

        int alpha = -MATE_SCORE;
        int beta = MATE_SCORE;

        if (depth > 2 && abs(best_score) < MATE_SCORE - 1000) {
            alpha = best_score - 50;
            beta = best_score + 50;
        }

        bool research = true;
        int research_count = 0;

        while (research && research_count < 3) {
            research = false;

            for (int i = 0; i < list.count; i++) {
                int move = list.moves[i];

                if (depth > 1 && move == best_move) {
                    int tmp = list.moves[0];
                    list.moves[0] = move;
                    list.moves[i] = tmp;
                }

                if (!make_move(pos, &state, move, keys)) continue;

                int score;
                if (i == 0) {
                    score = -search(pos, depth - 1, 1, -beta, -alpha, 1, params, magic, keys);
                } else {
                    score = -search(pos, depth - 1, 1, -alpha - 1, -alpha, 0, params, magic, keys);
                    if (score > alpha && score < beta) {
                        score = -search(pos, depth - 1, 1, -beta, -alpha, 1, params, magic, keys);
                    }
                }

                unmake_move(pos, &state, keys);

                if (score > current_best_score) {
                    current_best_score = score;
                    current_best_move = move;
                }

                if (score > alpha) alpha = score;

                if (score <= alpha - 50 || score >= beta + 50) {
                    research = true;
                    alpha = -MATE_SCORE;
                    beta = MATE_SCORE;
                    break;
                }
            }
            research_count++;
        }

        if (current_best_move != 0) {
            best_move = current_best_move;
            best_score = current_best_score;
        }

        printf("info depth %d score cp %d\n", depth, (pos->side_to_move == WHITE) ? best_score : -best_score);

        if (abs(best_score) > MATE_SCORE - 1000) {
            printf("info string Found mate in %d\n", (MATE_SCORE - abs(best_score) + 1) / 2);
            if (mate_line && mate_length) {
                mate_line[0] = best_move;
                *mate_length = 1;
            }
            return 2;  // Forced mate detected
        }
    }

    if (mate_line && mate_length) {
        mate_line[0] = best_move;
        *mate_length = 1;
    }

    return best_move;
}