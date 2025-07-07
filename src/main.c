#include "board.h"
#include "eval.h"
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

int depth = 1;

int main() {
    char line[512];
    MagicData* magic = malloc(sizeof(MagicData));
    if (!magic) {
        fprintf(stderr, "Failed to allocate MagicData\n");
        return 1;
    }

    init_engine(magic);
    Position pos;
    MoveState state;
    MoveList list;

    printf("id name JkCheeserChess\n");
    printf("id author YourName\n");
    fflush(stdout);

    while (fgets(line, sizeof(line), stdin)) {
        line[strcspn(line, "\n")] = '\0';

        if (strncmp(line, "uci", 3) == 0) {
            printf("uciok\n");
            fflush(stdout);
        } else if (strncmp(line, "isready", 7) == 0) {
            printf("readyok\n");
            fflush(stdout);
        } else if (strncmp(line, "ucinewgame", 10) == 0) {
            // Reset state if needed
        } else if (strncmp(line, "position", 8) == 0) {
            const char* ptr = line + 9;
            if (strncmp(ptr, "startpos", 8) == 0) {
                init_position(&pos, STARTPOS_FEN);
                memset(&state, 0, sizeof(MoveState));
                ptr += 8;
            } else if (strncmp(ptr, "fen", 3) == 0) {
                ptr += 4;
                char fen[256] = {0};
                sscanf(ptr, "%255[^\n]", fen);
                init_position(&pos, fen);
                ptr += strlen(fen);
            }

            // Handle moves
            char* moves = strstr(line, "moves");
            if (moves) {
                moves += 6;
                char move_str[8];
                while (sscanf(moves, "%7s", move_str) == 1) {
                    int move = parse_move(&pos, move_str, magic);
                    if (move) {
                        if (!make_move(&pos, &state, move)) {
                            fprintf(stderr, "Illegal move in move list: %s\n", move_str);
                            break;
                        }
                    }
                    moves += strlen(move_str);
                    while (*moves == ' ') moves++;
                }
            }
        } else if (strncmp(line, "go", 2) == 0) {
            generate_legal_moves(&pos, &list, pos.side_to_move, magic);
            if (list.count > 0) {
                int move = list.moves[0];
                char bestmove[6];
                move_to_uci(move, bestmove);
                printf("bestmove %s\n", bestmove);
            }
            fflush(stdout);
        } else if (strncmp(line, "quit", 4) == 0) {
            break;
        }
    }

    free(magic);
    return 0;
}