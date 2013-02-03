

// Implements the Node and RNode data structures, 
// which map block ids to LZTrie nodes and
// block ids to RevTrie nodes, respectively

#ifndef NODEMAPINCLUDED
#define NODEMAPINCLUDED

#include "basics.h"
#include "lztrie.h"

typedef struct snodemap
 { 
    uint nbits;        // bits per cell
    uint *map;         // mapping
 } *nodemap;

 
        // creates a nodemap structure from a mapping array, not owning it
        // n is number of blocks
        // max is the number of trie nodes
nodemap createNodemap(uint *map, uint n, uint max);
	// frees revtrie structure, including the owned data
void destroyNodemap(nodemap M);
	// mapping
trieNode mapto(nodemap M, uint id);

#endif
