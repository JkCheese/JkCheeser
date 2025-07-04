#ifndef OPERATIONS_H
#define OPERATIONS_H

#define set_bit(bb, sq) ((bb) |= (1ULL << (sq))) // also used to generate moves
#define get_bit(bb, sq) ((bb) & (1ULL << sq))
#define clear_bit(bb, sq) ((bb) &= ~(1ULL << (sq)))
#define get_lsb(bb) (__builtin_ctzll(bb)) // Get the least significant bit set to 1
#define pop_lsb(bb) ((bb) &= (bb - 1)) // Pop the least significant bit set to 1

#endif