
// Implements the Map data structure, which maps block ids to lztrie nodes

#include "nodemap.h"

	// creates a nodemap structure from a mapping array, not owning it
	// n is number of blocks
	// max is the number of trie nodes

nodemap createNodemap (uint *map, uint n, uint max)

   { nodemap M;
     uint i;
     M = malloc (sizeof(struct snodemap));
     M->nbits = bits(2*max-1);
     M->map = malloc (((n*M->nbits+W-1)/W)*sizeof(uint));
     for (i=0;i<n;i++)
        bitput (M->map,i*M->nbits,M->nbits,map[i]);
     return M;

   }

	// frees revtrie structure, including the owned data

void destroyNodemap (nodemap M)

   { free (M->map);
     free (M);
   }

	// mapping

trieNode mapto (nodemap M, uint id)

   { return bitget (M->map,id*M->nbits,M->nbits);
   }
