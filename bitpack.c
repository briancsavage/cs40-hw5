/* Last Modified: March 5th, 2020
 * Brian Savage and Vichka Fonarev
 * bsavag01         vfonar01
 * 
 * Comp40 - HW4 - Arith
 * bitpack.c
 * Does: implementation for bitpacking
 * Possible Errors: Handles all unit testing and (de)compression properly
 */

#include <stdlib.h>
#include <stdio.h>
#include "assert.h"
#include <stdbool.h>
#include "except.h"
#include "bitpack.h"

#define UNSIGNED uint64_t
#define SIGNED int64_t
#define SIZE 64


/* Except_T Bitpack_Overflow
 *     This exception is raised if a given value can not fit 
 *     in a block of the given width, preventing out of bounds
 *     overwriting within the word
 */
Except_T Bitpack_Overflow = { "Overflow packing bits" };

/* static inline UNSIGNED u_right_shift(UNSIGNED word, unsigned width)
 * Parameters: UNSIGNED word - the word to be shifted
 *             unsigned width - the width of the shift to be performed
 *    Returns: UNSIGNED type, the shifted word
 *       Does: shifts the given would by width to the right
 *   if Error: an assertion will be raised if width > SIZE
 */
static inline UNSIGNED u_left_shift(UNSIGNED word, unsigned width)
{
    assert(width <= SIZE);
    if (width == SIZE) {
        return 0;
    }
    return word << width;
}

/* static inline UNSIGNED u_right_shift(UNSIGNED word, unsigned width)
 * Parameters: UNSIGNED word - the word to be shifted
 *             unsigned width - the width of the shift to be performed
 *    Returns: UNSIGNED type, the shifted word
 *       Does: shifts the given would by width to the right
 *   if Error: an assertion will be raised if width > SIZE
 */
static inline UNSIGNED u_right_shift(UNSIGNED word, unsigned width)
{
    assert(width <= SIZE);
    if (width == SIZE) {
        return 0;
    }
    return word >> width;
}

/* static inline SIGNED s_right_shift(SIGNED word, unsigned width)
 * Parameters: SIGNED word - the word that will be shifted
 *             unsigned width - the width of the shift to be performed
 *    Returns: SIGNED type, the shifted word
 *       Does: shifts a signed word by the given width
 *   if Error: an assertion will be raised if width > SIZE
 */
static inline SIGNED s_right_shift(SIGNED word, unsigned width)
{
    assert(width <= SIZE);
    if (width == SIZE) {
        return 0;
    }
    return word >> width;
}

/* static inline UNSIGNED with_new_bits(UNSIGNED word, unsigned width, 
 *                                      unsigned lsb, SIGNED value)
 * Parameters: UNSIGNED word - the word that value will in inserted into
 *             unsigned width - the width of the value that will be inserted
 *             unsigned lsb - the least significant bit of value within word
 *             SIGNED value - the signed value that will be inserted into the 
 *             word
 *    Returns: UNSIGNED type, with the value inserted into the given word
 *       Does: inserts a signed value at lsb to lsb + width within the given 
 *             word
 *   if Error: from the shift helper functions, an assertion will be raised 
 *             if width > SIZE
 */
static inline UNSIGNED with_new_bits(UNSIGNED word, unsigned width, 
                                     unsigned lsb, SIGNED value)
{
    UNSIGNED mask = ~0; /* 111...111 */
    mask = u_left_shift(mask, (SIZE - width));
    mask = u_right_shift(mask, (SIZE - (width + lsb)));
    /* ^ the mask will now be all 0s except in range from lsb to lsb + width
     * which will be all 1s */

    value = u_left_shift(value, lsb); /* shift value over to lsb */

    /* apply mask over value to remove leading 1s in front of width + lsb */
    value &= mask; 
    return word | value; 
}

/* bool Bitpack_fitsu(UNSIGNED n, unsigned width)
 * Parameters: UNSIGNED n - the unsigned word to be checked
 *             unsigned width - the length of the used space in the word
 *    Returns: True -> unsigned word (n) can hold a value of length width
 *             False -> unsigned word (n) can not hold a value of length width
 *       Does: Validates that the given unsigned value can be contained within
 *             the given size
 *   if Error: assertion will be raised if width > SIZE
 */
bool Bitpack_fitsu(UNSIGNED n, unsigned width)
{
    assert(width <= SIZE);
    if (width == 0) {
      return false;
    }
    if (n == 0) {
        return true;
    }
    /* compute the max value of the given width check that
     * n is less than or equal to this max value */
    UNSIGNED t = 1;
    t = u_left_shift(t, width) - 1;
    if (t >= n || width == SIZE) {
      return true;
    }

    return false;
}

/* bool Bitpack_fitss(SIGNED n, unsigned width)
 * Parameters: SIGNED n - the signed word to be checked
 *             unsigned width - the length of the used space in the word
 *    Returns: True -> signed word (n) can hold a value of length width
 *             False -> signed word (n) can not hold a value of length width
 *       Does: Validates that the given signed word (n) has room to hold,
 *             a new insertion of size width into the signed word
 *   if Error: assertion will be raised if width > SIZE
 */
bool Bitpack_fitss(SIGNED n, unsigned width)
{
    assert(width <= SIZE);
    if (width == 0) {
        return false;
    }
    if (n == 0) {
        return true;
    }
    /* if the value is negative, check if the complement (positive value)
     * can be held in width - 1 to preserve the negative in the original */
    if (n < 0) {
      return Bitpack_fitsu((UNSIGNED) ~n, width - 1);
    }
    return Bitpack_fitsu((UNSIGNED) n, width - 1);
}

/* UNSIGNED Bitpack_getu(UNSIGNED word, unsigned width, unsigned lsb)
 * Parameters: UNSIGNED word - the unsigned word to be operated on
 *             unsigned width - the length of the used space in the word
 *             unsigned lsb - the index of the least significant bit in word
 *    Returns: UNSIGNED type, containing the range of bits gotten
 *       Does: gets the bits starting at lsb to lsb + width for UNSIGNED type
 *   if Error: assertion will be raised if width + lsb > SIZE
 */
UNSIGNED Bitpack_getu(UNSIGNED word, unsigned width, unsigned lsb)
{
    assert(width + lsb <= SIZE);
    if (width == 0 || word == 0) {
        return 0;
    }
    UNSIGNED mask = ~0; 
    mask = u_left_shift(mask, (SIZE - width));
    mask = u_right_shift(mask, (SIZE - (width + lsb)));

    /* apply mask over the given word to make everything outside the
     * lsb to lsb + width range equal to 0, then shift the gotten value
     * right by lsb to have the value lsb be 0 */
    return u_right_shift(word & mask, lsb);
}

/* SIGNED Bitpack_gets(UNSIGNED word, unsigned width, unsigned lsb)
 * Parameters: UNSIGNED word - the unsigned word to be operated on
 *             unsigned width - the length of the used space in the word
 *             unsigned lsb - the index of the least significant bit in word
 *    Returns: SIGNED type, containing the range of bits gotten
 *       Does: gets the bits starting at lsb to lsb + width for a SIGNED type
 *   if Error: assertion will be raised if width + lsb > SIZE
 */
SIGNED Bitpack_gets(UNSIGNED word, unsigned width, unsigned lsb)
{
    assert(width + lsb <= SIZE);
    if (width == 0 || word == 0) {
        return 0;
    }
    UNSIGNED mask = ~0;
    mask = u_left_shift(mask, (SIZE - width));
    mask = u_right_shift(mask, (SIZE - (width + lsb)));

    /* apply the mask over the given word, perform a left shift then
     * a signed right shift to preserve the sign of the value within
     * lsb to lsb + width  */
    UNSIGNED copy = word & mask;
    copy = u_left_shift(copy, (SIZE - (width + lsb)));
    return s_right_shift((SIGNED) copy, (SIZE - width));
}

/* UNSIGNED Bitpack_newu(UNSIGNED word, unsigned width, unsigned lsb, 
                         UNSIGNED value)
 * Parameters: UNSIGNED word - the original UNSIGNED word that value will be 
 *             inserted into
 *             unsigned width - the bit length of the value to be inserted
 *             unsigned lsb - the index for the least significant bit
 *             UNSIGNED value - the UNSIGNED value to be inserted into the word
 *    Returns: UNSIGNED type, with the passed value inserted into the word
 *       Does: inserts an UNSIGNED value into an UNSIGNED word starting at the 
 *             lsb to lsb + width
 *   if Error: assertion will be raised if width + lsb > SIZE
 *             raises Bitpack_Overflow if given width will cause an overflow
 */
UNSIGNED Bitpack_newu(UNSIGNED word, unsigned width, unsigned lsb, 
                      UNSIGNED value)
{
    assert(width + lsb <= SIZE);
    if (Bitpack_fitsu(value, width) == false) {
        /* if the value can not be contained within the given width */
        RAISE(Bitpack_Overflow);
    }
    return with_new_bits(word, width, lsb, (SIGNED) value);
}

/* UNSIGNED Bitpack_news(UNSIGNED word, unsigned width, unsigned lsb, 
                         SIGNED value)
 * Parameters: UNSIGNED word - the original UNSIGNED word that value will be 
 *             inserted into
 *             unsigned width - the bit length of the value to be inserted
 *             unsigned lsb - the index for the least significant bit
 *             SIGNED value - the SIGNED value to be inserted into the word
 *    Returns: UNSIGNED type, with the passed value inserted into the passed 
 *             word
 *       Does: inserts a SIGNED value into an UNSIGNED word starting at the 
 *             lsb to lsb + width
 *   if Error: assertion will be raised if width + lsb > SIZE
 *             raises Bitpack_Overflow if given width will cause an overflow
 */
UNSIGNED Bitpack_news(UNSIGNED word, unsigned width, unsigned lsb, 
                      SIGNED value)
{
    assert(width + lsb <= SIZE);
    if (Bitpack_fitss(value, width) == false) {
        /* if the value can not be contained within the given width */
        RAISE(Bitpack_Overflow);
    }
    return with_new_bits(word, width, lsb, value);
}
