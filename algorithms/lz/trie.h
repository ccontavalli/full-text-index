
 // LZ78 trie data structure

#ifndef TRIEINCLUDED
#define TRIEINCLUDED

#include "basics.h"
#include "heap.h"

typedef struct striebody
   { uint id;	// node id
     short nchildren;
     struct schild
       { byte car;
         struct striebody *trie;
       } *children;
   } triebody;

typedef struct strie
   { triebody trie;	// trie
     heap heaps[256];	// heaps
     uint nid;    	// nr of nodes
   } *trie;

	// creates trie
trie createTrie (void);
        // inserts word[0...] into pTrie and returns new text ptr
        // insertion proceeds until we get a new trie node
byte *insertTrie (trie pTrie, byte *word/*, ulong *pos*/);
        // inserts word[0..len-1] into pTrie, with id = id
        // assumes that no two equal strings are ever inserted
void insertstringTrie (trie pTrie, byte *word, uint len, uint id);
        // frees the trie
void destroyTrie (trie pTrie);
	// represents pTrie with parentheses, letters and ids
void representTrie (trie pTrie, uint *parent, byte *letters, uint *ids,
                    uint *emptybmap, uint nbits);

#endif
