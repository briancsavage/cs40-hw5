/* Last Modified: March 5th, 2020
 * Brian Savage and Vichka Fonarev
 * bsavag01         vfonar01
 * 
 * Comp40 - HW4 - Arith
 * bitpack.h
 * Does: interface for bitpacking
 * Possible Errors: Handles all unit testing and (de)compression properly
 */

#ifndef BITPACK_INCLUDED
#define BITPACK_INCLUDED
#include <stdbool.h>
#include <stdint.h>
#include "except.h"

extern Except_T Bitpack_Overflow;

bool Bitpack_fitsu(uint64_t n, unsigned width);
bool Bitpack_fitss( int64_t n, unsigned width);

uint64_t Bitpack_getu(uint64_t word, unsigned width, unsigned lsb);
int64_t Bitpack_gets(uint64_t word, unsigned width, unsigned lsb);

uint64_t Bitpack_newu(uint64_t word, unsigned width, unsigned lsb, 
		      uint64_t value); 
uint64_t Bitpack_news(uint64_t word, unsigned width, unsigned lsb, 
		      int64_t value);

#endif
