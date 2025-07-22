#include "board.h"
#include "evalsearch.h"
#include "evaltuner.h"
#include "engine.h"
#include "magic.h"
#include "moveformat.h"
#include "movegen.h"
#include "operations.h"
#include "test.h"
#include "uci.h"
#include <stdio.h>
#include <stdlib.h>

// Starting position: rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
// Example FEN: 2n1nkn1/1NBPpppP/8/pP1QN1Pp/1P6/2b5/3N4/R3K2R w KQ a6 0 2
// Example FEN (Black POV): r2k3r/4n3/5B2/6p1/Pp1nq1pP/8/pPPPpbn1/1NKN1N2 b kq h3 0 2
// Example FEN (checkmate test): 2n1nkn1/1NBPpQpP/8/pP2N1Pp/1P6/2b1r3/3N4/R3K2R b KQ - 0 2
// Example 960 FEN (for castling test): 1k5r/rpp4p/p1np4/8/3B4/2NQ2P1/PPP2P1P/RK3B1R b KQk - 15 21

#define STARTPOS_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

int depth = 6;

int main(int argc, char** argv) {
    MagicData* magic = malloc(sizeof(MagicData));
    if (!magic) {
        fprintf(stderr, "Failed to allocate MagicData\n");
        return 1;
    }

    ZobristKeys* keys = malloc(sizeof(ZobristKeys));
    if (!keys) {
        fprintf(stderr, "Failed to allocate ZobristKeys\n");
        return 1;
    }

    init_engine(magic, keys);
    Position pos;
    MoveState state;
    MoveList list;
    // const char* start_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    // init_position(&pos, start_fen);

    // EvalParamsDouble params;
    // init_double_params(&params);

    // EvalResult result = evaluate_with_features(&pos, &params, magic);

    // printf("Evaluation score: %.2f centipawns\n", result.score);

    // printf("Detected %d features:\n", result.num_features);
    // int zero_weight_features = 0;
    // for (int i = 0; i < result.num_features; i++) {
    //     int idx = result.features[i].index;
    //     double weight = result.features[i].weight;
    //     if (weight == 0)  {
    //         zero_weight_features++;
    //         continue;
    //     } 
    //     printf("  Feature index %4d | weight %+7.4f\n", idx, weight);
    // }
    // printf("Detected %d non-zero weighted features\n", result.num_features - zero_weight_features);

    uci_loop(&pos, &list, &state, depth, magic, keys);
    // if (argc >= 3) {
    //     run_tuner_main(argv[1], argv[2]);
    // } else {
    //     fprintf(stderr, "Usage: %s <dataset.txt> <output_prefix>\n", argv[0]);
    // }
    free(magic);
    free(keys);
    return 0;
}