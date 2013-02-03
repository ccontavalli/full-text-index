

// Implements the revtrie data structure

#ifndef REVTRIEINCLUDED
#define REVTRIEINCLUDED

#include "basics.h"
#include "parentheses.h"
#include "lztrie.h"
#include "nodemap.h"

typedef struct srevtrie
 { 
    uint *data;        // bitmap data
    parentheses pdata; // parentheses structure
    bitmap B;          // bitmap indicating which nodes are empty and wich are non-empty
    uint *rids_1;      // rids^{-1}  
    uint n;            // # of nodes
    uint nbits;        // log n
    lztrie trie;       // associated lztrie
    uint *R;            // pointer to R mapping  
 } *revtrie;

 
	// creates a revtrie structure from a parentheses bitstring,
	// a corresponding lztrie, the mapping from block id to lztrie,
	// and an id array in preorder, which are not owned
        // n is the total number of nodes (n ids, 2n parentheses)
revtrie createRevTrie(uint *string, uint *id, lztrie trie, uint * emptybmap, uint n);
	// frees revtrie structure, including the owned data
void destroyRevTrie(revtrie T);
        // stores the revtrie on file f
void saveRevTrie(revtrie T, FILE *f);
        // loads revtrie from file f
revtrie loadRevTrie(FILE *f, lztrie trie);
        // give children by (a string starting in) letter c, if possible.
        // leave in lzl,lzr the corresponding lztrie nodes (leftmost and
        // rightmost) below the node returned, if it is empty, otherwise
        // lzl=lztrie node corresponding to the node returned,lzr=null. 
        // depth is the length of the string represented by node i
trieNode childRevTrie (revtrie T, trieNode i, uint depth, byte c,
                        trieNode *lzl, trieNode *lzr);
	// subtree size
uint subtreesizeRevTrie (revtrie T, trieNode i);
	// smallest rank in subtree
uint leftrankRevTrie (revtrie T, trieNode i);
	// largest rank in subtree
uint rightrankRevTrie (revtrie T, trieNode i);
	// id of node
uint idRevTrie (revtrie T, trieNode i);
	// rth of position
uint rthRevTrie (revtrie T, uint pos);
        // is node i ancestor of node j?
bool ancestorRevTrie (revtrie T, trieNode i, trieNode j);
        // next node from i, in preorder
        // assumes it *can* go on!
trieNode nextRevTrie (revtrie T, trieNode i);

uint getposRevTrie(revtrie T, uint i);
trieNode getnodeRevTrie(revtrie T, uint pos);       
       // is the node i of RevTrie an empty node?
bool isemptyRevTrie(revtrie T, trieNode i);

uint sizeofRevTrie(revtrie T, uint n);

#endif
