#include "engine.h"
#include "magic.h"
#include "movegen.h"
#include "zobrist.h"
#include <stdio.h>
#include <stdlib.h>

void init_engine(MagicData* magic, ZobristKeys* keys) {
    srand(0);
    init_magic(magic);
    printf("Magic initialized.\n");
    init_zobrist(keys);
    printf("Zobrist initialized.\n");
}