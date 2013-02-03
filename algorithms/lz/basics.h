
// Basics

#ifndef BASICSINCLUDED
#define BASICSINCLUDED


  // Includes 

#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <unistd.h>

  // Memory management

#define malloc(n) Malloc(n)
#define free(p) Free(p)
#define realloc(p,n) Realloc(p,n)

void *Malloc (int n);
void Free (void *p);
void *Realloc (void *p, int n);

  // Data types

typedef unsigned char byte;
// typedef unsigned int uint;

typedef int bool;
#define true 1
#define false 0

#define max(x,y) ((x)>(y)?(x):(y))
#define min(x,y) ((x)<(y)?(x):(y))

  // Bitstream management

#define W (8*sizeof(uint))
#define bitsW 5 // OJO

	// bits needed to represent a number between 0 and n
uint bits (uint n);
        // returns e[p..p+len-1], assuming len <= W
uint bitget (uint *e, uint p, uint len);
        // writes e[p..p+len-1] = s, assuming len <= W
void bitput (uint *e, uint p, uint len, uint s);
	// reads bit p from e
#define bitget1(e,p) ((e)[(p)/W] & (1<<((p)%W)))
	// sets bit p in e
#define bitset(e,p) ((e)[(p)/W] |= (1<<((p)%W)))
	// cleans bit p in e
#define bitclean(e,p) ((e)[(p)/W] &= ~(1<<((p)%W)))

  
#endif
