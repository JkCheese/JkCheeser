#include "board.h"
#include "magic.h"
#include "operations.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Magic numbers from https://github.com/maksimKorzh/chess_programming/blob/master/src/magics/magics.txt
const Bitboard rook_magics_const[64] = {
    0x8a80104000800020ULL,
    0x140002000100040ULL,
    0x2801880a0017001ULL,
    0x100081001000420ULL,
    0x200020010080420ULL,
    0x3001c0002010008ULL,
    0x8480008002000100ULL,
    0x2080088004402900ULL,
    0x800098204000ULL,
    0x2024401000200040ULL,
    0x100802000801000ULL,
    0x120800800801000ULL,
    0x208808088000400ULL,
    0x2802200800400ULL,
    0x2200800100020080ULL,
    0x801000060821100ULL,
    0x80044006422000ULL,
    0x100808020004000ULL,
    0x12108a0010204200ULL,
    0x140848010000802ULL,
    0x481828014002800ULL,
    0x8094004002004100ULL,
    0x4010040010010802ULL,
    0x20008806104ULL,
    0x100400080208000ULL,
    0x2040002120081000ULL,
    0x21200680100081ULL,
    0x20100080080080ULL,
    0x2000a00200410ULL,
    0x20080800400ULL,
    0x80088400100102ULL,
    0x80004600042881ULL,
    0x4040008040800020ULL,
    0x440003000200801ULL,
    0x4200011004500ULL,
    0x188020010100100ULL,
    0x14800401802800ULL,
    0x2080040080800200ULL,
    0x124080204001001ULL,
    0x200046502000484ULL,
    0x480400080088020ULL,
    0x1000422010034000ULL,
    0x30200100110040ULL,
    0x100021010009ULL,
    0x2002080100110004ULL,
    0x202008004008002ULL,
    0x20020004010100ULL,
    0x2048440040820001ULL,
    0x101002200408200ULL,
    0x40802000401080ULL,
    0x4008142004410100ULL,
    0x2060820c0120200ULL,
    0x1001004080100ULL,
    0x20c020080040080ULL,
    0x2935610830022400ULL,
    0x44440041009200ULL,
    0x280001040802101ULL,
    0x2100190040002085ULL,
    0x80c0084100102001ULL,
    0x4024081001000421ULL,
    0x20030a0244872ULL,
    0x12001008414402ULL,
    0x2006104900a0804ULL,
    0x1004081002402ULL
};

const Bitboard bishop_magics_const[64] = {
    0x40040844404084ULL,
    0x2004208a004208ULL,
    0x10190041080202ULL,
    0x108060845042010ULL,
    0x581104180800210ULL,
    0x2112080446200010ULL,
    0x1080820820060210ULL,
    0x3c0808410220200ULL,
    0x4050404440404ULL,
    0x21001420088ULL,
    0x24d0080801082102ULL,
    0x1020a0a020400ULL,
    0x40308200402ULL,
    0x4011002100800ULL,
    0x401484104104005ULL,
    0x801010402020200ULL,
    0x400210c3880100ULL,
    0x404022024108200ULL,
    0x810018200204102ULL,
    0x4002801a02003ULL,
    0x85040820080400ULL,
    0x810102c808880400ULL,
    0xe900410884800ULL,
    0x8002020480840102ULL,
    0x220200865090201ULL,
    0x2010100a02021202ULL,
    0x152048408022401ULL,
    0x20080002081110ULL,
    0x4001001021004000ULL,
    0x800040400a011002ULL,
    0xe4004081011002ULL,
    0x1c004001012080ULL,
    0x8004200962a00220ULL,
    0x8422100208500202ULL,
    0x2000402200300c08ULL,
    0x8646020080080080ULL,
    0x80020a0200100808ULL,
    0x2010004880111000ULL,
    0x623000a080011400ULL,
    0x42008c0340209202ULL,
    0x209188240001000ULL,
    0x400408a884001800ULL,
    0x110400a6080400ULL,
    0x1840060a44020800ULL,
    0x90080104000041ULL,
    0x201011000808101ULL,
    0x1a2208080504f080ULL,
    0x8012020600211212ULL,
    0x500861011240000ULL,
    0x180806108200800ULL,
    0x4000020e01040044ULL,
    0x300000261044000aULL,
    0x802241102020002ULL,
    0x20906061210001ULL,
    0x5a84841004010310ULL,
    0x4010801011c04ULL,
    0xa010109502200ULL,
    0x4a02012000ULL,
    0x500201010098b028ULL,
    0x8040002811040900ULL,
    0x28000010020204ULL,
    0x6000020202d0240ULL,
    0x8918844842082200ULL,
    0x4010011029020020ULL
};

const int rook_shifts_const[64] = {
    12, 11, 11, 11, 11, 11, 11, 12,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    12, 11, 11, 11, 11, 11, 11, 12
};

const int bishop_shifts_const[64] = {
    6, 5, 5, 5, 5, 5, 5, 6,
    5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5,
    6, 5, 5, 5, 5, 5, 5, 6
};

static const int rook_deltas[4]   = {8, -8, 1, -1};
static const int bishop_deltas[4] = {9, -9, 7, -7};

// Mask between-board edges to prevent wrap-around
static inline int is_valid_rook(int from, int to) {
    return (RANK(from) == RANK(to)) || (FILE(from) == FILE(to));
}

static inline int is_valid_bishop(int from, int to) {
    return abs(RANK(from) - RANK(to)) == abs(FILE(from) - FILE(to));
}

// Compute blocker mask for bishops
Bitboard mask_bishop_blockers(int sq) {
    Bitboard mask = 0ULL;
    for (int d = 0; d < 4; d++) {
        int delta = bishop_deltas[d];
        int s = sq + delta;
        while (ON_BOARD(s) && is_valid_bishop(sq, s)) {
            if (RANK(s) == 0 || RANK(s) == 7 || FILE(s) == 0 || FILE(s) == 7) break;
            mask |= (1ULL << s);
            s += delta;
        }
    }
    // printf("Bishop mask for sq %d: ", sq);
    // for (int i = 0; i < 64; i++) {
    //     if (mask & (1ULL << i)) printf("%d ", i);
    // }
    // printf("\n");
    
    return mask;
}

Bitboard compute_bishop_attacks(int sq, Bitboard blockers) {
    Bitboard attacks = 0ULL;
    for (int d = 0; d < 4; d++) {
        int delta = bishop_deltas[d];
        int s = sq + delta;
        while (ON_BOARD(s) && is_valid_bishop(sq, s)) {
            attacks |= (1ULL << s);
            if (blockers & (1ULL << s)) break;
            s += delta;
        }
    }
    return attacks;
}

// Compute blocker mask for rooks
Bitboard mask_rook_blockers(int sq) {
    Bitboard mask = 0ULL;
    int rank = sq / 8;
    int file = sq % 8;

    // Up
    for (int r = rank + 1; r <= 6; r++) mask |= (1ULL << (r * 8 + file));
    // Down
    for (int r = rank - 1; r >= 1; r--) mask |= (1ULL << (r * 8 + file));
    // Right
    for (int f = file + 1; f <= 6; f++) mask |= (1ULL << (rank * 8 + f));
    // Left
    for (int f = file - 1; f >= 1; f--) mask |= (1ULL << (rank * 8 + f));

    return mask;
}

// Compute attacks given a square and blockers
Bitboard compute_rook_attacks(int sq, Bitboard blockers) {
    Bitboard attacks = 0ULL;
    for (int d = 0; d < 4; d++) {
        int delta = rook_deltas[d];
        int s = sq + delta;
        while (ON_BOARD(s) && is_valid_rook(sq, s)) {
            attacks |= (1ULL << s);
            if (blockers & (1ULL << s)) break;
            s += delta;
        }
    }
    return attacks;
}

// Count bits set
int count_bits(Bitboard b) {
    int count = 0;
    while (b) {
        count++;
        b &= b - 1;
    }
    return count;
}

// Given index and mask, return blocker permutation
Bitboard set_blockers_from_index(uint64_t index, Bitboard mask) {
    Bitboard blockers = 0ULL;
    int bits = count_bits(mask);
    int i = 0;
    for (int sq = 0; sq < 64; sq++) {
        if (mask & (1ULL << sq)) {
            if (index & (1ULL << i)) blockers |= (1ULL << sq);
            i++;
        }
    }
    return blockers;
}

void init_magic(MagicData* magic) {

    if (load_magic_tables(magic, "magictable.bin")) {
        // printf("Loaded magic tables from disk.\n");
        return;
    }
    
    // printf("init_magic() called\n");
    // memset(magic, 0, sizeof(MagicData)); // or memset(magic->bishop_shifts, 0, sizeof(magic->bishop_shifts));
    memcpy(magic->rook_magics, rook_magics_const, sizeof(rook_magics_const));
    memcpy(magic->bishop_magics, bishop_magics_const, sizeof(bishop_magics_const));
    memcpy(magic->rook_shifts, rook_shifts_const, sizeof(rook_shifts_const));
    memcpy(magic->bishop_shifts, bishop_shifts_const, sizeof(bishop_shifts_const));
    // printf("Global constants defined\n");

    for (int sq = 0; sq < 64; sq++) {
        magic->rook_masks[sq] = mask_rook_blockers(sq);
        magic->bishop_masks[sq] = mask_bishop_blockers(sq);
        // printf("sq=%2d  bishop_shift=%2d  rook_shift=%2d\n", sq, magic->bishop_shifts[sq], magic->rook_shifts[sq]);
    }

    // printf("Blocker masks initialized\n");

    for (int sq = 0; sq < 64; sq++) {
        int rook_bits = count_bits(magic->rook_masks[sq]);
        // printf("rook bits counted\n");
        uint64_t rook_limit = 1ULL << rook_bits;
        // printf("Square %d rook combos: %llu\n", sq, rook_limit);
        // fflush(stdout);
        for (uint64_t index = 0; index < rook_limit; index++) {
            Bitboard blockers = set_blockers_from_index(index, magic->rook_masks[sq]);
            uint64_t magic_index = (blockers * magic->rook_magics[sq]) >> (64 - magic->rook_shifts[sq]);
            magic->rook_attack_table[sq][magic_index] = compute_rook_attacks(sq, blockers);
        }

        int bishop_bits = count_bits(magic->bishop_masks[sq]);
        // printf("bishop bits counted\n");
        uint64_t bishop_limit = 1ULL << bishop_bits;
        // printf("Square %d bishop combos: %llu\n", sq, bishop_limit);
        // fflush(stdout);
        for (uint64_t index = 0; index < bishop_limit; index++) {
            Bitboard blockers = set_blockers_from_index(index, magic->bishop_masks[sq]);
            uint64_t magic_index = (blockers * magic->bishop_magics[sq]) >> (64 - magic->bishop_shifts[sq]);
            magic->bishop_attack_table[sq][magic_index] = compute_bishop_attacks(sq, blockers);
        }
        // printf("init_magic success\n");
    }

    save_magic_tables(magic, "magictable.bin");
    // printf("Saved magic tables to disk.\n");
}

int save_magic_tables(const MagicData* magic, const char* path) {
    FILE* f = fopen(path, "wb");
    if (!f) return 0;

    size_t written = fwrite(magic, sizeof(MagicData), 1, f);
    fclose(f);
    return written == 1;
}

int load_magic_tables(MagicData* magic, const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) return 0;

    size_t read = fread(magic, sizeof(MagicData), 1, f);
    fclose(f);
    return read == 1;
}