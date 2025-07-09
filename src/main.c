#include "board.h"
#include "engine.h"
#include "magic.h"
#include "moveformat.h"
#include "movegen.h"
#include "operations.h"
#include "test.h"
#include <stdio.h>
#include <stdlib.h>

// Starting position: rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
// Example FEN: 2n1nkn1/1NBPpppP/8/pP1QN1Pp/1P6/2b5/3N4/R3K2R w KQ a6 0 2
// Example FEN (Black POV): r2k3r/4n3/5B2/6p1/Pp1nq1pP/8/pPPPpbn1/1NKN1N2 b kq h3 0 2
// Example FEN (checkmate test): 2n1nkn1/1NBPpQpP/8/pP2N1Pp/1P6/2b1r3/3N4/R3K2R b KQ - 0 2
// Example 960 FEN (for castling test): 1k5r/rpp4p/p1np4/8/3B4/2NQ2P1/PPP2P1P/RK3B1R b KQk - 15 21

int depth = 1;

int main() {
    printf("Program started.\n");
    printf("Initializing variables...\n");
    Position pos;
    MoveList list;
    MagicData* magic = malloc(sizeof(MagicData)); // Allocate on heap
    if (!magic) {
        printf("Failed to allocate MagicData\n");
        return 1;
    }
    const char* fen = "r3k2r/p2p1p1p/P2P1P1P/8/8/p2p1p1p/P2P1P1P/R3K2R w KQkq - 0 1";

    printf("Variables initialized.\n");
    
    printf("Initializing engine...");
    init_engine(magic);
    printf("Engine initialized.\n");
    
    printf("Parsing FEN...\n");
    init_position(&pos, fen);

    printf("rook_from[%d] = %d, rook_to[%d] = %d\n",
        WHITE_QUEENSIDE_ROOK, pos.rook_from[WHITE_QUEENSIDE_ROOK],
        WHITE_QUEENSIDE_ROOK, pos.rook_to[WHITE_QUEENSIDE_ROOK]);

    printf("rook_from[%d] = %d, rook_to[%d] = %d\n",
        WHITE_KINGSIDE_ROOK, pos.rook_from[WHITE_KINGSIDE_ROOK],
        WHITE_KINGSIDE_ROOK, pos.rook_to[WHITE_KINGSIDE_ROOK]);
        
    printf("rook_from[%d] = %d, rook_to[%d] = %d\n",
        BLACK_QUEENSIDE_ROOK, pos.rook_from[BLACK_QUEENSIDE_ROOK],
        BLACK_QUEENSIDE_ROOK, pos.rook_to[BLACK_QUEENSIDE_ROOK]);
                        
    printf("rook_from[%d] = %d, rook_to[%d] = %d\n",
        BLACK_KINGSIDE_ROOK, pos.rook_from[BLACK_KINGSIDE_ROOK],
        BLACK_KINGSIDE_ROOK, pos.rook_to[BLACK_KINGSIDE_ROOK]);
    
    printf("Printing board...\n");
    print_board(&pos);

    printf("Generating moves...\n");
    generate_legal_moves(&pos, &list, pos.side_to_move, magic);
    print_moves(&pos, &list, magic);
    printf("Move generation done.\n");

    printf("Running perft divide (depth = %d)...\n", depth);
    perft_divide(&pos, depth, magic);

    printf("Running perft total (depth = %d)...\n", depth);
    uint64_t total_nodes = perft_debug(&pos, depth, magic);
    printf("Total nodes: %llu\n", total_nodes);

    free(magic);
    return 0;
}