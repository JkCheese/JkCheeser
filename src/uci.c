// --- Modified uci.c with InstantMate support ---
#include "evalparams.h"
#include "evalsearch.h"
#include "test.h"
#include "uci.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define STARTPOS_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

static int forced_mate_line[32];
static int forced_mate_index = 0;
static int forced_mate_length = 0;
static int instant_mate_mode = 0;  // InstantMate option flag

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

    if (flag >= 6 && flag <= 13) {
        char promo = (flag <= 9) ? "nbrq"[flag - 6] : "nbrq"[flag - 10];
        out[4] = promo;
        out[5] = '\0';
    } else {
        out[4] = '\0';
    }
}

int parse_move(const Position* pos, const char* uci_str, const MagicData* magic, ZobristKeys* keys) {
    MoveList list;
    generate_legal_moves(pos, &list, pos->side_to_move, magic, keys);

    for (int i = 0; i < list.count; ++i) {
        char move_str[6];
        move_to_uci(list.moves[i], move_str);
        if (strncmp(move_str, uci_str, 5) == 0) {
            return list.moves[i];
        }
    }
    return 0;
}

void uci_loop(Position* pos, MoveList* list, MoveState* state, int depth, const MagicData* magic, ZobristKeys* keys) {
    char line[32767];
    printf("id name JkCheeserChess\n");
    printf("id author JkCheese\n");
    printf("option name UCI_Chess960 type check default false\n");
    printf("option name InstantMate type check default false\n");
    fflush(stdout);

    while (fgets(line, sizeof(line), stdin)) {
        line[strcspn(line, "\n")] = '\0';

        if (strncmp(line, "uci", 3) == 0) {
            printf("uciok\n");
            fflush(stdout);

        } else if (strncmp(line, "isready", 7) == 0) {
            printf("readyok\n");
            fflush(stdout);

        } else if (strncmp(line, "setoption", 9) == 0) {
            if (strstr(line, "name InstantMate")) {
                if (strstr(line, "value true")) instant_mate_mode = 1;
                else instant_mate_mode = 0;
            }

        } else if (strncmp(line, "ucinewgame", 10) == 0) {
            forced_mate_index = 0;
            forced_mate_length = 0;

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
                pos->zobrist_hash = compute_zobrist_hash(pos, keys);
                ptr += strlen(fen);
            }

            char* moves = strstr(line, "moves");
            if (moves) {
                moves += 6;
                char move_str[8];
                while (sscanf(moves, "%7s", move_str) == 1) {
                    int move = parse_move(pos, move_str, magic, keys);
                    if (move) {
                        if (!make_move(pos, state, move, keys)) {
                            fprintf(stderr, "Illegal move: %s\n", move_str);
                            break;
                        }
                    }
                    moves += strlen(move_str);
                    while (*moves == ' ') moves++;
                }
            }

        } else if (strncmp(line, "go", 2) == 0) {
            if (instant_mate_mode && forced_mate_index < forced_mate_length) {
                int move = forced_mate_line[forced_mate_index++];
                char move_str[6];
                move_to_uci(move, move_str);
                printf("bestmove %s\n", move_str);
                make_move(pos, state, move, keys);
                fflush(stdout);
                continue;
            }

            generate_legal_moves(pos, list, pos->side_to_move, magic, keys);
            EvalParams params;
            set_default_evalparams(&params);

            if (list->count > 0) {
                int mate_line[32] = {0};
                int mate_len = 0;
                int result = find_best_move(pos, depth, &params, magic, keys, mate_line, &mate_len);

                if (result == 2 && mate_len > 0) {
                    memcpy(forced_mate_line, mate_line, sizeof(int) * mate_len);
                    forced_mate_index = 1;
                    forced_mate_length = mate_len;
                    char move_str[6];
                    move_to_uci(mate_line[0], move_str);
                    printf("info string Forced mate detected\n");
                    printf("bestmove %s\n", move_str);
                    make_move(pos, state, mate_line[0], keys);
                } else if (result == 0) {
                    printf("bestmove 0000\n");
                } else {
                    char move_str[6];
                    move_to_uci(result, move_str);
                    printf("bestmove %s\n", move_str);
                    make_move(pos, state, result, keys);
                }
            } else {
                printf("bestmove 0000\n");
            }
            fflush(stdout);

        } else if (strncmp(line, "quit", 4) == 0) {
            break;
        }
    }
}