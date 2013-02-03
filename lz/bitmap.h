

// Implements operations over a bitmap

#ifndef BITMAPINCLUDED
#define BITMAPINCLUDED

#include "basics.h"

typedef struct sbitmap
 { 
    uint *data;
    uint n;        // # of bits
    uint *sdata;   // superblock counters
    byte *bdata;   // block counters
    uint *sdata_0; // superblock counters for select_0() queries
    byte *bdata_0; // block counters for select_0() queries
 } *bitmap;
   
   
        // creates a bitmap structure from a bitstring, which gets owned
bitmap createBitmap (uint *string, uint n, bool isfullbmap);
        // rank(i): how many 1's are there before position i, not included
uint rank (bitmap B, uint i);
        // select_1(x): returns the position i of the bitmap B such that
        // the number of ones up to position i is x.
uint select_1(bitmap B, uint x);
        // select_0(x): returns the position i of the bitmap B such that
        // the number of zeros up to position i is x.
uint select_0(bitmap B, uint x);
        // destroys the bitmap, freeing the original bitstream
void destroyBitmap (bitmap B);
        // popcounts 1's in x
uint popcount (register uint x);

uint sizeofBitmap(bitmap B);
#endif

