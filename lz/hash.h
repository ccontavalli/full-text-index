
// Closed hash table that does not store the keys

// needs the maximum number of elements stored to be declared at creation time
// cannot remove elements
// does not store the key, rather, it reports all the collisioning values
// right value under collisions
// CANNOT STORE ZERO in the value

#ifndef HASHINCLUDED
#define HASHINCLUDED

#include "basics.h"

typedef struct shash
   { uint size;    // # of table entries
     uint bits;    // bits per entry
     uint *table;    // data
   } *hash;

typedef uint handle;

  // creates a table to store up to n values with guaranteed load factor.
  // vbits = # of bits per entry, ENTRIES CANNOT HAVE VALUE ZERO
hash createHash (uint n, uint vbits, float factor);
  // frees the structure
void destroyHash (hash H);
  // inserts an entry 
void insertHash (hash H, uint key, uint elem);
  // looks for a key, returns first value (zero => no values)
  // writes in pos a handle to get next values
uint searchHash (hash H, uint key, handle *h);
  // gets following values using handle *h, which is rewritten
  // returns next value (zero => no more values)
uint nextHash (hash H, handle *h);

        // two large primes found with etc/hash.c
#define PRIME1 ((uint)4294967279)
#define PRIME2 ((uint)4294967197)

#endif
