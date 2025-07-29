#include "evaluation.h"
#include "movegen.h"
#include "operations.h"

void evaluate_passed_pawns(const Position* pos, FeatureCounts* counts, const EvalParams* params, const EvalParamsDouble* dparams, int side, int* mg, int* eg, double* dmg, double* deg) {
    counts->passed_pawn_bonus = 0;
    
    Bitboard pawns = pos->pieces[side == WHITE ? WP : BP];
    Bitboard enemy_pawns = pos->pieces[side == WHITE ? BP : WP];

    int sign = (side == pos->side_to_move) ? +1 : -1;

    while (pawns) {
        int sq = pop_lsb(&pawns);
        Bitboard file_mask = FILE_X(FILE(sq));
        if (FILE(sq) > 0) file_mask |= FILE_X(FILE(sq - 1));
        if (FILE(sq) < 7) file_mask |= FILE_X(FILE(sq + 1));

        Bitboard front_mask = SQUARES_AHEAD(sq, side);

        if (enemy_pawns & file_mask & front_mask) {
            continue;
        }

        counts->passed_pawn_bonus++;

        if (params) {
            *mg += sign * params->passed_pawn_bonus_mg;
            *eg += sign * params->passed_pawn_bonus_eg;
        }
        if (dparams) {
            *dmg += (double)sign * dparams->passed_pawn_bonus_mg;
            *deg += (double)sign * dparams->passed_pawn_bonus_eg;
        }
    }
}

void evaluate_knight_outposts(const Position* pos, FeatureCounts* counts, const EvalParams* params, const EvalParamsDouble* dparams, int side, int* mg, int* eg, double* dmg, double* deg) {
    counts->knight_outpost_bonus = 0;

    Bitboard knights = pos->pieces[side == WHITE ? WN : BN];
    Bitboard own_pawns = pos->pieces[side == WHITE ? WP : BP];
    Bitboard enemy_pawns = pos->pieces[side == WHITE ? BP : WP];

    int sign = (side == pos->side_to_move) ? +1 : -1;

    while (knights) {
        int sq = pop_lsb(&knights);
        Bitboard file_mask = 0;
        if (FILE(sq) > 0) file_mask |= FILE_X(FILE(sq - 1));
        if (FILE(sq) < 7) file_mask |= FILE_X(FILE(sq + 1));

        Bitboard front_mask = SQUARES_AHEAD(sq, side);

        Bitboard defend_mask = 0;
        if (side == WHITE) {
            if (FILE(sq) > 0) defend_mask |= 1ULL << (sq - 9);
            if (FILE(sq) < 7) defend_mask |= 1ULL << (sq - 7);
        } else {
            if (FILE(sq) > 0) defend_mask |= 1ULL << (sq + 7);
            if (FILE(sq) < 7) defend_mask |= 1ULL << (sq + 9);
        }

        if (enemy_pawns & file_mask & front_mask || !(own_pawns & defend_mask)) {
            continue;
        }

        counts->knight_outpost_bonus++;

        if (params) {
            *mg += sign * params->knight_outpost_bonus_mg;
            *eg += sign * params->knight_outpost_bonus_eg;
        }
        if (dparams) {
            *dmg += (double)sign * dparams->knight_outpost_bonus_mg;
            *deg += (double)sign * dparams->knight_outpost_bonus_eg;
        }
    }
}

void evaluate_rook_activity(const Position* pos, FeatureCounts* counts, const EvalParams* params, const EvalParamsDouble* dparams, int side, int* mg, int* eg, double* dmg, double* deg) {
    counts->rook_semi_open_file_bonus = 0;
    counts->rook_semi_open_file_bonus = 0;
    counts->rook_open_file_bonus = 0;

    Bitboard rooks = pos->pieces[side == WHITE ? WR : BR];
    Bitboard own_pawns = pos->pieces[side == WHITE ? WP : BP];
    Bitboard enemy_pawns = pos->pieces[side == WHITE ? BP : WP];

    int sign = (side == pos->side_to_move) ? +1 : -1;

    while (rooks) {
        int sq = pop_lsb(&rooks);

        if ((side == WHITE && RANK(sq) == 6) || (side == BLACK && RANK(sq) == 1)) {
            counts->blind_swine_rooks_bonus++;
            if (params) {
                *mg += sign * params->blind_swine_rooks_bonus_mg;
                *eg += sign * params->blind_swine_rooks_bonus_eg;
            }
            if (dparams) {
                *dmg += (double)sign * dparams->blind_swine_rooks_bonus_mg;
                *deg += (double)sign * dparams->blind_swine_rooks_bonus_eg;
            }
        }
        if (FILE(sq) & own_pawns) {
            continue;
        }
        else if (FILE(sq) & enemy_pawns) {
            counts->rook_semi_open_file_bonus++;
            if (params) {
                *mg += sign * params->rook_semi_open_file_bonus_mg;
                *eg += sign * params->rook_semi_open_file_bonus_eg;
            }
            if (dparams) {
                *dmg += (double)sign * dparams->rook_semi_open_file_bonus_mg;
                *deg += (double)sign * dparams->rook_semi_open_file_bonus_eg;
            }
        }
        else {
            counts->rook_open_file_bonus++;
            if (params) {
                *mg += sign * params->rook_open_file_bonus_mg;
                *eg += sign * params->rook_open_file_bonus_eg;
            }
            if (dparams) {
                *dmg += (double)sign * dparams->rook_open_file_bonus_mg;
                *deg += (double)sign * dparams->rook_open_file_bonus_eg;
            }
        }
    }
}

void evaluate_tropism(const Position* pos, FeatureCounts* counts, const EvalParams* params, const EvalParamsDouble* dparams, int side, int* mg, int* eg, double* dmg, double* deg) {
    memset(counts->tropism, 0, sizeof(counts->tropism));
    int enemy_king_sq = pos->king_from[side ^ 1];  // Get enemy king location
    int sign = (side == pos->side_to_move) ? +1 : -1;

    for (PieceType piece_type = N; piece_type <= Q; piece_type++) {
        Bitboard bb = pos->pieces[side == WHITE ? piece_type : piece_type + 6];
        while (bb) {
            int sq = pop_lsb(&bb);
            int dist = manhattan(sq, enemy_king_sq);
            if (dist > 7) dist = 7;

            counts->tropism[piece_type][dist]++;
            if (params) {
                *mg += sign * params->tropism_mg[piece_type][dist];
                *eg += sign * params->tropism_eg[piece_type][dist];
            }
            if (dparams) {
                *dmg += (double)sign * dparams->tropism_mg[piece_type][dist];
                *deg += (double)sign * dparams->tropism_eg[piece_type][dist];
            }
        }
    }
}

int evaluation(const Position* pos, const EvalParams* params) {
    FeatureCounts counts;
    int mg = 0, eg = 0;
    int phase = 0;

    for (int sq = 0; sq < 64; sq++) {
        int piece = get_piece_on_square(pos, sq);
        if (piece == -1) continue;

        int side = (piece < 6) ? WHITE : BLACK;
        int type = piece % 6;
        int mirrored_sq = (side == WHITE) ? sq : MIRROR(sq);

        // Use PSTs and values from params
        int mg_val = params->mg_value[type];
        int eg_val = params->eg_value[type];

        int mg_pst = 0, eg_pst = 0;
        switch (type) {
            case P:
                mg_pst = params->pawn_pst_mg[mirrored_sq];
                eg_pst = params->pawn_pst_eg[mirrored_sq];
                phase += 0;
                break;
            case N:
                mg_pst = params->knight_pst_mg[mirrored_sq];
                eg_pst = params->knight_pst_eg[mirrored_sq];
                phase += 1;
                break;
            case B:
                mg_pst = params->bishop_pst_mg[mirrored_sq];
                eg_pst = params->bishop_pst_eg[mirrored_sq];
                phase += 1;
                break;
            case R:
                mg_pst = params->rook_pst_mg[mirrored_sq];
                eg_pst = params->rook_pst_eg[mirrored_sq];
                phase += 2;
                break;
            case Q:
                mg_pst = params->queen_pst_mg[mirrored_sq];
                eg_pst = params->queen_pst_eg[mirrored_sq];
                phase += 4;
                break;
            case K:
                mg_pst = params->king_pst_mg[mirrored_sq];
                eg_pst = params->king_pst_eg[mirrored_sq];
                break;
        }

        int mg_score = mg_val + mg_pst;
        int eg_score = eg_val + eg_pst;

        mg += (side == pos->side_to_move) ? mg_score : -mg_score;
        eg += (side == pos->side_to_move) ? eg_score : -eg_score;
    }

    evaluate_passed_pawns(pos, &counts, params, NULL, WHITE, &mg, &eg, NULL, NULL);
    evaluate_passed_pawns(pos, &counts, params, NULL, BLACK, &mg, &eg, NULL, NULL);

    evaluate_knight_outposts(pos, &counts, params, NULL, WHITE, &mg, &eg, NULL, NULL);
    evaluate_knight_outposts(pos, &counts, params, NULL, BLACK, &mg, &eg, NULL, NULL);

    evaluate_tropism(pos, &counts, params, NULL, WHITE, &mg, &eg, NULL, NULL);
    evaluate_tropism(pos, &counts, params, NULL, BLACK, &mg, &eg, NULL, NULL);

    // Cap the phase to 24 (max material)
    if (phase > 24) phase = 24;

    // Interpolate between middlegame and endgame scores
    int score = (mg * phase + eg * (24 - phase)) / 24;
    return score;
}