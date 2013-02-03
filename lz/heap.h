
  // Heap of fixed size objects, fast to alloc and especially to totally free

#ifndef HEAPINCLUDED
#define HEAPINCLUDED

#include "basics.h"

typedef struct sheap
   { uint siz;      // element size
     uint cap;      // block capacity in # of elems
     void **blocks; // list of blocks, next is block[0]
     void **free;   // list of free positions, each contains next
     uint first;    // first free position in current block
     uint totsize;  // total memory allocated here
   } *heap;

	// Creates a heap of elements of size siz
heap createHeap (uint siz);
	// Gets a new element from H
void *mallocHeap (heap H);
	// Frees ptr from heap H
void freeHeap (heap H, void *ptr);
	// Frees everything in heap H
void destroyHeap (heap H);

#endif
