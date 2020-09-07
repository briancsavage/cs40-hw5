/* Last Modified: March 5th, 2020
 * Brian Savage and Vichka Fonarev
 * bsavag01         vfonar01
 * 
 * Comp40 - HW4 - Arith
 * testBit.c
 * Does: unit testing for bitpacking
 */

#include "bitpack.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#define SIGNED int64_t
#define UNSIGNED uint64_t
#define SIZE 64

int main() 
{       
        /*      TESTS FOR Bitpack_fitsu()  */
                UNSIGNED a = 2;
                assert(!Bitpack_fitsu(a, 0));
                assert(!Bitpack_fitsu(a, 1));
                assert(Bitpack_fitsu(a, 2));
                assert(Bitpack_fitsu(a, 3));
                assert(Bitpack_fitsu(a, SIZE - 1));
                assert(Bitpack_fitsu(a, SIZE - 2));
                assert(Bitpack_fitsu(a, SIZE)); 

                a = 11;
                assert(!Bitpack_fitsu(a, 0));
                assert(!Bitpack_fitsu(a,1)); 
                assert(!Bitpack_fitsu(a, 3));
                assert(Bitpack_fitsu(a, 4));
                assert(Bitpack_fitsu(a, 5));
                assert(Bitpack_fitsu(a, SIZE - 3));
                assert(Bitpack_fitsu(a, SIZE - 4));
                assert(Bitpack_fitsu(a, SIZE));

                assert(Bitpack_fitsu(0, 1));

        /*  TESTS FOR Bitpack_fitss()  */
                SIGNED b = 5;
                assert(!Bitpack_fitss(b,0)); 
                assert(!Bitpack_fitss(b,1)); 
                assert(!Bitpack_fitss(b,3)); 
                assert(Bitpack_fitss(b,4));
                assert(Bitpack_fitss(b,5));
                assert(Bitpack_fitss(b,SIZE));

                b = -199; /* open SIZE - 9 */
                assert(!Bitpack_fitss(b,0)); 
                assert(!Bitpack_fitss(b,1)); 
                assert(!Bitpack_fitss(b,3)); 
                assert(!Bitpack_fitss(b,8));
                assert(Bitpack_fitss(b, 9));
                assert(Bitpack_fitss(b, SIZE));
                assert(Bitpack_fitss(b, SIZE - 1));
                assert(Bitpack_fitss(b, SIZE - 8));

                assert(Bitpack_fitss(0, 1));

        /*  TESTS FOR Bitpack_getu()   */
                UNSIGNED value = 123; /* ...111 1011 */
                assert(Bitpack_getu(value, 2, 0) == 3);
                assert(Bitpack_getu(value, 3, 0) == 3);
                assert(Bitpack_getu(value, 4, 0) == 11);
                assert(Bitpack_getu(value, 2, 2) == 2);
                assert(Bitpack_getu(value, 20, 6) == 1);
                assert(Bitpack_getu(value, 1, 2) == 0);
                assert(Bitpack_getu(value, 1, 3) == 1);
                assert(Bitpack_getu(value, 10, 2) == 30);
                assert(Bitpack_getu(value, SIZE - 2, 2) == 30);
                assert(Bitpack_getu(value, SIZE, 0) == 123);
                assert(Bitpack_getu(value, SIZE - 7, 7) == 0);

        /*  TESTS FOR Bitpack_gets()   */
                SIGNED word = 123; /* ...111 1011 */
                assert(Bitpack_gets(word, 2, 0) == -1);
                assert(Bitpack_gets(word, 3, 0) == 3);
                assert(Bitpack_gets(word, 4, 2) == -2);
                assert(Bitpack_gets(word, 8, 0) == 123);
                assert(Bitpack_gets(word, 8, 6) == 1);
                assert(Bitpack_gets(word, 8, 7) == 0); 
                assert(Bitpack_gets(word, 0, 0) == 0);
                assert(Bitpack_gets(word, SIZE, 0) == 123);
                assert(Bitpack_gets(word, SIZE - 2, 2) == 30);

                word = -999; /* ..100 0001 1001 */
                assert(Bitpack_gets(word, 2, 0) == 1);
                assert(Bitpack_gets(word, 2, 2) == -2);
                assert(Bitpack_gets(word, 4, 2) == 6);
                assert(Bitpack_gets(word, 8, 0) == 25);
                assert(Bitpack_gets(word, 8, 6) == -16);
                assert(Bitpack_gets(word, 8, 7) == -8); 
                assert(Bitpack_gets(word, 0, 0) == 0);
                assert(Bitpack_gets(word, SIZE, 0) == -999);
                assert(Bitpack_gets(word, SIZE - 2, 2) == -250);


        /*  TESTS FOR Bitpack_newu()   */
                UNSIGNED c = 0; 
                c = Bitpack_newu(c, 1, 0, (UNSIGNED) 1);
                assert( Bitpack_getu(c, 1, 0) == 1);
                c = Bitpack_newu(c, 3, 1, (UNSIGNED) 7);
                assert( Bitpack_getu(c, 3, 1) == 7);
                c = Bitpack_newu(c, 4, 4, (UNSIGNED) 5);
                assert( Bitpack_getu(c, 4, 4) == 5);
                c = Bitpack_newu(c, 7, 7, (UNSIGNED) 87);
                assert( Bitpack_getu(c, 7, 7) == 87);
                c = Bitpack_newu(c, 1, SIZE - 1, (UNSIGNED) 1);
                assert( Bitpack_getu(c, 1, SIZE - 1) == 1);
                c = Bitpack_newu(c, 3, SIZE - 4, (UNSIGNED) 3);
                assert( Bitpack_getu(c, 3, SIZE - 4) == 3);
                c = Bitpack_newu(c, 5, SIZE - 9, (UNSIGNED) 10);
                assert( Bitpack_getu(c, 5, SIZE - 9) == 10);

        /*  TESTS FOR Bitpack_news()   */
                UNSIGNED d = 0;
                d = Bitpack_news(d, 2, 0, (SIGNED) 1);
                assert(Bitpack_gets(d, 2, 0) == 1);
                d = Bitpack_news(d, 5, 2, (SIGNED) -7);
                assert(Bitpack_gets(d, 5, 2) == -7);
                d = Bitpack_news(d, 4, 7, (SIGNED) 5);
                assert(Bitpack_gets(d, 4, 7) == 5);
                d = Bitpack_news(d, 9, 11, (SIGNED) -87);
                assert(Bitpack_gets(d, 9, 11) == -87);
                d = Bitpack_news(d, 2, SIZE - 2, (SIGNED) 1);
                assert(Bitpack_gets(d, 2, SIZE - 2) == 1);
                d = Bitpack_news(d, 4, SIZE - 6, (SIGNED) -3);
                assert(Bitpack_gets(d, 4, SIZE - 6) == -3);
                d = Bitpack_news(d, 5, SIZE - 13, (SIGNED) 10);
                assert(Bitpack_gets(d, 5, SIZE - 13) == 10);

    return EXIT_SUCCESS;
}
