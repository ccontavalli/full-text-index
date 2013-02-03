

// Implements the LZtrie data structure

#ifndef LZTRIEINCLUDED
#define LZTRIEINCLUDED

#include "basics.h"
#include "parentheses.h"


typedef uint trieNode; // a node of lztrie

#define NULLT ((trieNode)(~0)) // a null node
#define ROOT ((trieNode)(0)) // the root node
#define PARAMETER_T_IDS 2

typedef struct slztrie
 { 
    uint *data;         // bitmap data
    parentheses pdata;  // parentheses structure
    uint n;             // # of parentheses
    byte *letters;      // letters of the trie
    uint *ids;          // ids of the trie
    uint nbits;         // log n 
    trieNode *boost;    // direct pointers to first children;
    uint *R;            // pointer to the R mapping
 } *lztrie;

	// creates a lztrie structure from a parentheses bitstring,
	// a letter array in preorder, and an id array in preorder,
        // all of which except the latter become owned
        // n is the total number of nodes (n letters/ids, 2n parentheses)
lztrie createLZTrie(uint *string, byte *letters, uint *id, uint n);
	// frees LZTrie structure, including the owned data
void destroyLZTrie(lztrie T);
        // stores lztrie T on file f
void saveLZTrie(lztrie T, FILE *f);
        // loads lztrie T from file f
lztrie loadLZTrie(FILE *f);
	// letter by which node i descends
byte letterLZTrie(lztrie T, trieNode i);
	// go down by letter c, if possible
trieNode childLZTrie(lztrie T, trieNode i, byte c);
	// go up, if possible
trieNode parentLZTrie(lztrie T, trieNode i);
	// subtree size
uint subtreesizeLZTrie(lztrie T, trieNode i);
	// smallest rank in subtree
uint leftrankLZTrie(lztrie T, trieNode i);
	// largest rank in subtree
uint rightrankLZTrie(lztrie T, trieNode i);
	// id of node
uint idLZTrie(lztrie T, trieNode i);
	// rth of position
uint rthLZTrie(lztrie T, uint pos);
        // is node i ancestor of node j?
bool ancestorLZTrie(lztrie T, trieNode i, trieNode j);
	// next node from i, in preorder, adds/decrements depth accordingly
	// assumes it *can* go on!
trieNode nextLZTrie(lztrie T, trieNode i, uint *depth);
        // size in bytes of LZTrie T
uint sizeofLZTrie(lztrie T);
uint maxdepthLZTrie(lztrie T);
#endif
