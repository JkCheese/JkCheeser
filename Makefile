CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinclude -O3 -march=native

SRC = \
	src/board.c \
	src/engine.c \
	src/evaluation.c \
	src/evalparams.c \
	src/evalsearch.c \
	src/evaltuner.c \
	src/moveformat.c \
	src/movegen.c \
	src/magic.c \
	src/main.c \
	src/test.c \
	src/tt.c \
	src/uci.c \
	src/zobrist.c

OBJ = $(SRC:.c=.o)
BIN = v9-tune_testing_5

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

run: $(BIN)
	./$(BIN) lichess-big3-resolved.book tuned_params

clean:
	rm -f src/*.o $(BIN)

rebuild:
	$(MAKE) clean
	$(MAKE)
	$(MAKE) run