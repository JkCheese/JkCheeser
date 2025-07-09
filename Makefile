CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinclude -O3 -march=native -pg

SRC = \
	src/board.c \
	src/engine.c \
	src/moveformat.c \
	src/movegen.c \
	src/magic.c \
	src/main.c \
	src/test.c \
	src/uci.c

OBJ = $(SRC:.c=.o)
BIN = engine2

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

run:
	./$(BIN)

clean:
	rm -f src/*.o $(BIN)

rebuild:
	$(MAKE) clean
	$(MAKE)
	$(MAKE) run