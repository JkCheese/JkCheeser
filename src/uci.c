#include "evalsearch.h"
#include "test.h"
#include "uci.h"
#include <stdio.h>

#define STARTPOS_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

void move_to_uci(int move, char out[6]) {
    int from = MOVE_FROM(move);
    int to = MOVE_TO(move);
    int flag = MOVE_FLAG(move);

    char files[] = "abcdefgh";
    char ranks[] = "12345678";

    out[0] = files[from % 8];
    out[1] = ranks[from / 8];
    out[2] = files[to % 8];
    out[3] = ranks[to / 8];

    // Promotion handling (e.g., q, r, b, n)
    if (flag >= 6 && flag <= 13) {
        char promo = (flag <= 9) ? "nbrq"[flag - 6] : "nbrq"[flag - 10];
        out[4] = promo;
        out[5] = '\0';
    } else {
        out[4] = '\0';
    }
}

int parse_move(const Position* pos, const char* uci_str, const MagicData* magic, const ZobristKeys* keys) {
    MoveList list;
    generate_legal_moves(pos, &list, pos->side_to_move, magic, keys);

    for (int i = 0; i < list.count; ++i) {
        char move_str[6];
        move_to_uci(list.moves[i], move_str);
        if (strncmp(move_str, uci_str, 5) == 0) { // allow match with or without promo letter
            return list.moves[i];
        }
    }
    return 0; // Illegal move
}

void uci_loop(Position* pos, MoveList* list, MoveState* state, int depth, EvalParams* params, const MagicData* magic, const ZobristKeys* keys) {
    char line[32767];
    printf("id name JkCheeserChess\n");
    printf("id author JkCheese\n");
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
                init_position(pos, STARTPOS_FEN);
                memset(state, 0, sizeof(MoveState));
                ptr += 8;
            } else if (strncmp(ptr, "fen", 3) == 0) {
                ptr += 4;
                char fen[256] = {0};
                sscanf(ptr, "%255[^\n]", fen);
                init_position(pos, fen);
                generate_legal_moves(pos, list, pos->side_to_move, magic, keys);
                // print_moves(pos, list, magic, keys);
                // perft_debug(pos, depth, magic, keys);
                // Initialize Zobrist hashing
                pos->zobrist_hash = compute_zobrist_hash(pos, keys);
                ptr += strlen(fen);
            }

            // Handle moves
            char* moves = strstr(line, "moves");
            if (moves) {
                moves += 6;
                char move_str[8];
                while (sscanf(moves, "%7s", move_str) == 1) {
                    int move = parse_move(pos, move_str, magic, keys);
                    if (move) {
                        if (!make_move(pos, state, move, keys)) {
                            fprintf(stderr, "Illegal move in move list: %s\n", move_str);
                            break;
                        }

                        repetition_table[repetition_index++] = pos->zobrist_hash;

                        int eval = evaluation(pos, params, magic);
                        printf("info string Eval after move %s: %d\n", move_str, eval);

                    }
                    moves += strlen(move_str);
                    while (*moves == ' ') moves++;
                }
            }
        } else if (strncmp(line, "go", 2) == 0) {
            generate_legal_moves(pos, list, pos->side_to_move, magic, keys);
            if (list->count > 0) {
                int move = find_best_move(pos, depth, params, magic, keys);  // or depth 4 if fast enough;
                if (move == 0) {
                    printf("bestmove 0000\n");  // Null move (no legal moves, e.g., checkmate or stalemate)
                } else {
                    char bestmove[6];
                    move_to_uci(move, bestmove);

                    int eval = evaluation(pos, params, magic);
                    printf("info string Eval before bestmove %s: %d\n", bestmove, eval);

                    printf("bestmove %s\n", bestmove);
                }
            }
            fflush(stdout);
        } else if (strncmp(line, "quit", 4) == 0) {
            break;
        }
    }
}