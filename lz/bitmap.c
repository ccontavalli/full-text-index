

// Implements operations over a bitmap

#include "bitmap.h"

  // In theory, we should have superblocks of size s=log^2 n divided into
  // blocks of size b=(log n)/2. This takes 
  // O(n log n / log^2 n + n log log n / log n + log n sqrt n log log n) bits
  // In practice, we can have any s and b, and the needed amount of bits is
  // (n/s) log n + (n/b) log s + b 2^b log b bits
  // Optimizing it turns out that s should be exactly s = b log n
  // Optimizing b is more difficult but could be done numerically.
  // However, the exponential table does no more than popcounting, so why not
  // setting up a popcount algorithm tailored to the computer register size,
  // defining that size as b, and proceeding.

const unsigned char popc[] =
 {
    0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
    1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
    1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
    2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
    1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
    2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
    2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
    3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8,
 };

uint popcount (register uint x)
 { 
    return popc[x&0xFF] + popc[(x>>8)&0xFF] + popc[(x>>16)&0xFF] + popc[x>>24];
 }

uint popcount8(register int x)
 {
    return popc[x & 0xff];
 }   
   
   
	// creates a bitmap structure from a bitstring, which is shared

bitmap createBitmap (uint *string, uint n, bool isfullbmap)
 { 
    bitmap B;
    uint i,j,pop,bpop,pos;
    uint nb,ns,words;
    B = malloc (sizeof(struct sbitmap));
    B->data = string;
    B->n = n; words = (n+W-1)/W;
    ns = (n+256-1)/256; nb = 256/W; // adjustments
    B->bdata = malloc (ns*nb*sizeof(byte));
    B->sdata = malloc (ns*sizeof(int));
#ifdef INDEXREPORT
    printf ("     Bitmap over %i bits took %i bits\n", n,n+ns*nb*8+ns*32);
#endif
    pop = 0; pos = 0;
    for (i=0;i<ns;i++) { 
       bpop = 0;
       B->sdata[i] = pop;
       for (j=0;j<nb;j++) { 
          if (pos == words) break;
          B->bdata[pos++] = bpop;
          bpop += popcount(*string++);
       }
       pop += bpop;
    }
    if (isfullbmap) {// creates the data structure to solve select_0() queries
       B->bdata_0 = malloc(ns*nb*sizeof(byte));
       B->sdata_0 = malloc(ns*sizeof(int));    
       string = B->data;
       pop = 0; pos = 0;
       for (i = 0; i < ns; i++) { 
          bpop = 0;
          B->sdata_0[i] = pop;
          for (j = 0; j < nb; j++) { 
             if (pos == words) break;
             B->bdata_0[pos++] = bpop;
             bpop += popcount(~(*string++));
          }
          pop += bpop;
       }
    }
    else { B->bdata_0 = NULL; B->sdata_0 = NULL;}
    return B;
 }

	// rank(i): how many 1's are there before position i, not included

uint rank (bitmap B, uint i)
 { 
    return B->sdata[i>>8] + B->bdata[i>>5] +
           popcount (B->data[i>>5] & ((1<<(i&0x1F))-1));
 }


        // select_1(x): returns the position i of the bitmap B such that
        // the number of ones up to position i is x.

uint select_1(bitmap B, uint x)
 {
    uint s = 256;
    uint l = 0, n = B->n, r = n/s,left;
    uint mid = (l+r)/2;
    uint ones, j;
    uint rankmid = B->sdata[mid];
    // first, binary search on the superblock array
    while (l <= r) {
       if (rankmid < x)
          l = mid+1;
       else
          r = mid-1;
       mid = (l+r)/2;
       rankmid = B->sdata[mid];
    }
    // sequential search using popcount over an int
    left = mid*8;
    x -= rankmid;
    j = B->data[left];
    ones = popcount(j);
    while (ones < x) {
       x -= ones;
       left++;
       if (left > (n+W-1)/W) return n;
       j = B->data[left];
       ones = popcount(j);
    }
    // sequential search using popcount over a char
    left = left*32;
    rankmid = popcount8(j);
    if (rankmid < x) {
       j = j>>8;
       x -= rankmid;
       left += 8;
       rankmid = popcount8(j);
       if (rankmid < x) {
          j = j>>8;
          x -= rankmid;
          left += 8;
          rankmid = popcount8(j);
          if (rankmid < x) {
             j = j>>8;
             x -= rankmid;
             left += 8;
          }
       }
    }
  // finally sequential search bit per bit
    while (x > 0) {
       if  (j&1) x--;
       j = j>>1;
       left++;
    }
    return left-1;
}


        // select_0(x): returns the position i of the bitmap B such that
        // the number of zeros up to position i is x.

uint select_0(bitmap B, uint x)
 {
    uint s = 256;
    uint l = 0, n = B->n, r = n/s, left;
    uint mid = (l+r)/2;
    uint ones, j;
    uint rankmid = B->sdata_0[mid];
    // first, binary search on the superblock array
    while (l <= r) {
       if (rankmid < x)
          l = mid+1;
       else
          r = mid-1;
       mid = (l+r)/2;
       rankmid = B->sdata_0[mid];
    }
    // sequential search using popcount over an int
    left = mid*8;
    x -= rankmid;
    j = B->data[left];
    ones = popcount(~j);
    while (ones < x) {
       x -= ones;
       left++;
       if (left > (n+W-1)/W) return n;
       j = B->data[left];
       ones = popcount(~j);
    }
    // sequential search using popcount over a char
    left = left*32;
    rankmid = popcount8(~j);
    if (rankmid < x) {
       j = j>>8;
       x -= rankmid;
       left += 8;
       rankmid = popcount8(~j);
       if (rankmid < x) {
          j = j>>8;
          x -= rankmid;
          left += 8;
          rankmid = popcount8(~j);
          if (rankmid < x) {
             j = j>>8;
             x -= rankmid;
             left += 8;
          }
       }
    }
  // finally sequential search bit per bit
    while (x > 0) {
       if  ((j&1)==0) x--;
       j = j>>1;
       left++;
    }
    return left-1; 
 }


    
	
	// destroys the bitmap, freeing the original bitstream

void destroyBitmap(bitmap B)
 { 
    free(B->data);
    free(B->bdata);
    free(B->sdata);
    free(B);
 }


uint sizeofBitmap(bitmap B)
 {
    return sizeof(struct sbitmap) +
           ((B->n+W-1)/W)*sizeof(uint) + // data
           ((B->n+256-1)/256)*sizeof(int) + // sdata
           ((B->n+256-1)/256)*(256/W)*sizeof(byte)+ // bdata
           ((B->sdata_0)?((B->n+256-1)/256)*sizeof(int):0) +
           ((B->bdata_0)?((B->n+256-1)/256)*(256/W)*sizeof(byte):0);
 }
