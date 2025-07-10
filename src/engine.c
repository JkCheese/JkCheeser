#include "engine.h"
#include "magic.h"
#include "movegen.h"
#include <stdio.h>
#include <stdlib.h>

void init_engine(MagicData* magic) {
    srand(0);
    init_magic(magic);
    printf("Magic initialized.\n");
}