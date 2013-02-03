#ifndef __FM_COMMON
#define __FM_COMMON
#define TESTINFO (0)

#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <stdlib.h>

#ifndef DATATYPE 
typedef unsigned char uchar;
#ifndef __USE_MISC
typedef unsigned long uint32_t;
#endif
#define DATATYPE 1
#endif

/* Some useful macro */
#define EOF_shift(n) (n < index->bwt_eof_pos) ? n+1 :  n
#define MIN(a, b) ((a)<=(b) ? (a) : (b))
#define MAX(a, b) ((a)<=(b) ? (a) : (b))

/* Ritorna il tempo */
int int_log2(int); 		/* calcola log base 2 */
int int_pow2(int); 		/* calcola potenza di 2 */
#endif
