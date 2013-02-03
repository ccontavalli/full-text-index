
 // Indexing module

#include "trie.h"
#include "lztrie.h"
#include "nodemap.h"
#include "revtrie.h"
#include "lzindex.h"
#include <math.h>

	// creates lztrie over a null-terminated text
	// it also creates *ids


extern uint nbits_aux;

#ifdef INDEXREPORT
struct tms time;
clock_t t1,t2;
uint ticks;
#endif

lztrie buildLZTrie(byte *text, uint **ids, byte s)
 { 
    trie T;
    uint n;
    uint *parent;
    byte *letters;
    lztrie LZT;
    unsigned long long aux;
    // first creates a full trie T
#ifdef INDEXREPORT
    ticks= sysconf(_SC_CLK_TCK);
    times(&time); t1 = time.tms_utime;
    printf("  Building LZTrie...\n"); fflush(stdout);
    printf("    Building normal trie...\n"); fflush(stdout);
#endif
    T = createTrie();
    do {
       text = insertTrie(T,text);
    }   
    while (text[-1] != s);
    // now compresses it
#ifdef INDEXREPORT
    times(&time); t2 = time.tms_utime;
    printf("    User time: %f secs\n",(t2-t1)/(float)ticks); fflush(stdout);
    t1 = t2;
    printf("    Representing with parentheses, letters and ids...\n"); fflush(stdout);
#endif
    n       = T->nid;
    aux     = (2*(unsigned long long)n+W-1)/W;
    parent  = malloc(aux*sizeof(uint));
    letters = malloc(n*sizeof(byte));
    aux     = (((unsigned long long)n)*bits(n-1)+W-1)/W;
    *ids    = malloc(aux*sizeof(uint));
    representTrie(T,parent,letters,*ids,NULL,bits(n-1));
#ifdef INDEXREPORT
    times(&time); t2 = time.tms_utime;
    printf("    User time: %f secs\n",(t2-t1)/(float)ticks); fflush(stdout);
    t1 = t2;
    printf("    Freing trie...\n"); fflush(stdout);
#endif
    destroyTrie(T);
#ifdef INDEXREPORT
    times(&time); t2 = time.tms_utime;
    printf("    User time: %f secs\n",(t2-t1)/(float)ticks); fflush(stdout);
    t1 = t2;
    printf("    Creating compressed trie...\n"); fflush(stdout);
#endif
    LZT = createLZTrie(parent,letters,*ids,n);
#ifdef INDEXREPORT
    times(&time); t2 = time.tms_utime;
    printf("    User time: %f secs\n",(t2-t1)/(float)ticks); fflush(stdout);
    t1 = t2;
    printf("  End of LZTrie\n"); fflush(stdout);
#endif
    return LZT;
 }


	// builds reverse trie from LZTrie, Map, and maximum LZTrie depth
	// returns reverse ids

revtrie buildRevTrie(lztrie T, uint maxdepth, uint **ids)
 { 
    byte *str;
    uint n,rn,depth,j;
    trieNode i;
    trie RT;
    uint *parent, *emptybmap, *inv_ids, nbits, pos;
    revtrie CRT;
    unsigned long long aux;
    // first create a full trie RT
#ifdef INDEXREPORT
    times(&time); t1 = time.tms_utime;
    printf ("  Building RevTrie...\n"); fflush(stdout);
    printf ("    Creating full trie...\n"); fflush(stdout);
#endif
    str = malloc(maxdepth*sizeof(byte));
    RT = createTrie(); 
    i = ROOT; depth = 0;
    for (j=1;j<T->n;j++) { 
       i = nextLZTrie(T,i,&depth);
       str[maxdepth-depth] = letterLZTrie(T,i);
       pos = leftrankLZTrie(T, i);
       insertstringTrie(RT,str+maxdepth-depth,depth,rthLZTrie(T,pos));
    }
    free(str);
    // now compresses it
#ifdef INDEXREPORT
    times(&time); t2 = time.tms_utime;
    printf("    User time: %f secs\n",(t2-t1)/(float)ticks); fflush(stdout);
    t1        = t2;
    printf("    Representing with parentheses and ids...\n"); fflush(stdout);
#endif
    n         = T->n; 
    rn        = RT->nid;
    aux       = (2*(unsigned long long)rn+W-1)/W;
    parent    = malloc(aux*sizeof(uint)); // 2*rn bits
    emptybmap = calloc(((rn+W-1)/W),sizeof(uint));   // rn bits
    aux       = (((unsigned long long)n)*bits(n-1)+W-1)/W;
    *ids      = malloc(aux*sizeof(uint)); // the rids array has n entries
                                         // (only for the non-empty nodes)
    inv_ids   = malloc(aux*sizeof(uint));
    representTrie(RT,parent,NULL,*ids,emptybmap,bits(n-1));
#ifdef INDEXREPORT
    times(&time); t2 = time.tms_utime;
    printf("    User time: %f secs\n",(t2-t1)/(float)ticks); fflush(stdout);
    t1 = t2;
    printf("    Freeing trie...\n"); fflush(stdout);
#endif
    destroyTrie(RT);
#ifdef INDEXREPORT
    times(&time); t2 = time.tms_utime;
    printf("    User time: %f secs\n",(t2-t1)/(float)ticks); fflush(stdout);
    t1 = t2;
    printf("    Creating compressed trie...\n"); fflush(stdout);
#endif
    nbits = bits(n-1);
    for (i = 0; i < n; i++)
       bitput(inv_ids,bitget(*ids, i*nbits, nbits)*nbits,nbits,i);
    free(*ids);
    CRT = createRevTrie(parent,inv_ids,T,emptybmap,rn);
#ifdef INDEXREPORT
    times(&time); t2 = time.tms_utime;
    printf("    User time: %f secs\n",(t2-t1)/(float)ticks); fflush(stdout);
    t1 = t2;
    printf("  End of RevTrie...\n"); fflush(stdout);
#endif
    return CRT;
 }

// builds the R mapping
uint *buildR(uint *ids, uint *rids_1, uint n)
 {
    uint i, nbits;
    uint *R;
    
    nbits = bits(n-1);
    R = malloc(((n*nbits+W-1)/W)*sizeof(uint));
    for (i=0;i<n;i++) {
       uint id  = bitget(ids,i*nbits,nbits);
       uint aux = bitget(rids_1,id*nbits,nbits);
       bitput(R, aux*nbits, nbits, i);
    }
    return R;
 } 
 

byte selectSymbol(byte *text, ulong length)
 {
    ulong i;
    byte s;
    bool *A = calloc(256, sizeof(bool));;
    
    for (i=0;i<length;i++) A[text[i]]= true;
    for (s=0;s<256;s++)
       if (!A[s]) break;
    return s;
 } 
 
        // creates lzindex over a null-terminated text
	// frees text
int build_index(byte *text, ulong length, char *build_options, void **index)
 { 
    lzindex *I;
    uint *ids,maxdepth;
    
    I = malloc(sizeof(lzindex));
    text[length] = selectSymbol(text, length);
    // build index
    I->fwdtrie = buildLZTrie(text,&ids,text[length]);
    maxdepth   = maxdepthLZTrie(I->fwdtrie);
    nbits_aux = I->fwdtrie->nbits;
    I->bwdtrie = buildRevTrie(I->fwdtrie,maxdepth,&ids);
    I->R       = buildR(I->fwdtrie->ids, I->bwdtrie->rids_1, I->fwdtrie->n);
    I->fwdtrie->R = I->R;
    I->bwdtrie->R = I->R;
    I->TPos    = createPosition(I->fwdtrie, I->bwdtrie, length); 
    I->u       = length;
    *index = I; // return index
    return 0; // no errors yet
 }

