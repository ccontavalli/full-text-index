

// Implements operations over a sequence of balanced parentheses

#include "parentheses.h"

  // I have decided not to implement Munro et al.'s scheme, as it is too
  // complicated and the overhead is not so small in practice. I have opted 
  // for a simpler scheme. Each open (closing) parenthesis will be able to
  // find its matching closing (open) parenthesis. If the distance is shorter
  // than b, we will do it by hand, traversing the string. Otherwise, the
  // answer will be stored in a hash table. In fact, only subtrees larger than 
  // s will have the full distance stored, while those between b and s will
  // be in another table with just log s bits. The traversal by hand proceeds
  // in fact by chunks of k bits, whose answers are precomputed.
  // Space: there cannot be more than n/s subtrees larger than s, idem b.
  //   So we have (n/s)log n bits for far pointers and (n/b)log s for near 
  //   pointers. The space for the table is 2^k*k*log b. The optimum s=b log n,
  //   in which case the space is n/b(1 + log b + log log n) + 2^k*k*log b.
  // Time: the time is O(b/k), and we want to keep it O(log log n), so
  //   k = b/log log n.
  // (previous arguments hold if there are no unary nodes, but we hope that 
  //  there are not too many -- in revtrie we compress unary paths except when
  //  they have id)
  // Settings: using b = log n, we have 
  //   space = n log log n / log n + 2^(log n / log log n) log n
  //   time = log log n
  // In practice: we use k = 8 (bytes table), b = W (so work = 4 or 8)
  //   and space ~= n/3 + 10 Kbytes (fixed table). 
  // Notice that we need a hash table that stores only the deltas and does not
  // store the keys! (they would take log n instead of log s!). Collisions are
  // resolved as follows: see all the deltas that could be and pick the smallest
  // one whose excess is the same of the argument. To make this low we use a
  // load factor of 2.0, so it is 2n/3 after all.
  // We need the same for the reverses, for the forward is only for ('s and
  // reverses for )'s, so the proportion stays the same.
  // We also need the stream to be a bitmap to know how many open parentheses
  // we have to the left. The operations are as follows:
  // findclose: use the mechanism described above
  // findparent: similar, in reverse, looking for the current excess - 1
  //       this needs us to store the (near/far) parent of each node, which may
  //       cost more than the next sibling.
  // excess: using the number of open parentheses
  // enclose: almost findparent

	// creates a parentheses structure from a bitstring, which is shared
        // n is the total number of parentheses, opening + closing

static uint calcsizes (parentheses P, uint posparent, uint posopen, 
		       uint *near, uint *far, uint *pnear, uint *pfar)

   { uint posclose,newpos;
     if ((posopen == P->n) || bitget1(P->data,posopen))
	return posopen; // no more trees
     newpos = posopen;
     do { posclose = newpos+1;
	  newpos = calcsizes (P,posopen,posclose,near,far,pnear,pfar);
	}
     while (newpos != posclose);
     if ((posclose < P->n) && (posclose-posopen > W)) // exists and not small
        if (posclose-posopen < (1<<P->sbits)) (*near)++; // near pointer
        else (*far)++;
     if ((posopen > 0) && (posopen-posparent > W)) // exists and not small
        if (posopen-posparent < (1<<P->sbits)) (*pnear)++; // near pointer
        else (*pfar)++;
     return posclose;
   }


static uint filltables (parentheses P, uint posparent, uint posopen, bool bwd)

   { uint posclose,newpos;
     if ((posopen == P->n) || bitget1(P->data,posopen))
	return posopen; // no more trees
     newpos = posopen;
     do { posclose = newpos+1;
	  newpos = filltables (P,posopen,posclose,bwd);
	}
     while (newpos != posclose);
     if ((posclose < P->n) && (posclose-posopen > W)) // exists and not small
        { if (posclose-posopen < (1<<P->sbits)) // near pointers
	       insertHash (P->bftable,posopen,posclose-posopen);
          else // far pointers
               insertHash (P->sftable,posopen,posclose-posopen); 
	}
     if (bwd && (posopen > 0) && (posopen-posparent > W)) //exists and not small
        { if (posopen-posparent < (1<<P->sbits)) // near pointer
	       insertHash (P->bbtable,posopen,posopen-posparent);
	  else // far pointers
	       insertHash (P->sbtable,posopen,posopen-posparent);
	}
     return posclose;
   }

static byte FwdPos[256][W/2];
static byte BwdPos[256][W/2];
static char Excess[256];
static bool tablesComputed = false;

static void fcompchar (byte x, byte *pos, char *excess)

   { int exc = 0;
     uint i;
     for (i=0;i<W/2;i++) pos[i] = 0;
     for (i=0;i<8;i++)
	 { if (x & 1) // closing
	      { exc--; 
		if ((exc < 0) && !pos[-exc-1]) pos[-exc-1] = i+1;
	      }
	   else exc++;
	   x >>= 1;
	 }
     *excess = exc;
   }

static void bcompchar (byte x, byte *pos)

   { int exc = 0;
     uint i;
     for (i=0;i<W/2;i++) pos[i] = 0;
     for (i=0;i<8;i++)
	 { if (x & 128) // opening, will be used on complemented masks
	      { exc++; 
		if ((exc > 0) && !pos[exc-1]) pos[exc-1] = i+1;
	      }
	   else exc--;
	   x <<= 1;
	 }
   }

parentheses createParentheses(uint *string, uint n, bool bwd, bool isfullbitmap)
 { 
    parentheses P;
    uint i,s,nb,ns,nbits,near,far,pnear,pfar;

    P = malloc(sizeof(struct sparentheses));
    P->data = string;
    P->n = n;
    P->bdata = createBitmap(string,n,isfullbitmap);
    nbits = bits(n-1);
    s = nbits*W;
    P->sbits = bits(s-1);
    s = 1 << P->sbits; // to take the most advantage of what we can represent
    ns = (n+s-1)/s; nb = (s+W-1)/W; // adjustments
    near = far = pnear = pfar = 0;
    calcsizes(P,~0,0,&near,&far,&pnear,&pfar);
#ifdef INDEXREPORT
    printf("   Parentheses: total %i, near %i, far %i, pnear %i, pfar %i\n",n,near,far,pnear,pfar);
#endif
    P->sftable = createHash(far,nbits,1.8);
    P->bftable = createHash(near,P->sbits,1.8);
    if (bwd) {
       P->sbtable = createHash(pfar,nbits,1.8);
       P->bbtable = createHash(pnear,P->sbits,1.8);
    }
    else P->sbtable = P->bbtable = NULL;
    filltables(P,~0,0,bwd);
    if (!tablesComputed) {
       tablesComputed = true;
       for (i=0;i<256;i++) {
          fcompchar(i,FwdPos[i],Excess+i);
          bcompchar(i,BwdPos[i]);
       }
    }
    return P;
 }

	// frees parentheses structure, including the bitstream

void destroyParentheses (parentheses P)

   { destroyBitmap (P->bdata);
     destroyHash (P->sftable);
     if (P->sbtable) destroyHash (P->sbtable);
     destroyHash (P->bftable);
     if (P->bbtable) destroyHash (P->bbtable);
     free (P);
   }

	// the position of the closing parenthesis corresponding to (opening)
	// parenthesis at position i

uint findclose (parentheses P, uint i)

   { uint bitW;
     uint len,res,minres,exc;
     byte W1;
     handle h;
     uint myexcess;
	// first see if it is at small distance
     len = W; if (i+len >= P->n) len = P->n-i-1;
     bitW = bitget (P->data,i+1,len);
     exc = 0; len = 0;
     while (bitW && (exc < W/2))  
		// either we shift it all or it only opens parentheses or too
		// many open parentheses
        { W1 = bitW & 255;
          if (res = FwdPos[W1][exc]) return i+len+res;
          bitW >>= 8; exc += Excess[W1];
	  len += 8;
	}
	// ok, it's not a small distance, try with hashing btable
     minres = 0;
     myexcess = excess (P,i);
     res = searchHash (P->bftable,i,&h);
     while (res)
	{ if (!minres || (res < minres)) 
	     if ((i+res+1 < P->n) && (excess(P,i+res+1) == myexcess)) 
		minres = res;
	  res = nextHash (P->bftable,&h);
	}
     if (minres) return i+minres;
	// finally, it has to be a far pointer
     res = searchHash (P->sftable,i,&h);
     while (res)
	{ if (!minres || (res < minres)) 
	     if ((i+res+1 < P->n) && (excess(P,i+res+1) == myexcess))
	        minres = res;
	  res = nextHash (P->sftable,&h);
	}
     return i+minres; // there should be one if the sequence is balanced!
   }

	// find enclosing parenthesis for an open parenthesis
	// assumes that the parenthesis has an enclosing pair

uint findparent (parentheses P, uint i)

   { uint bitW;
     uint len,res,minres,exc;
     byte W1;
     handle h;
     uint myexcess;
	// first see if it is at small distance
     len = W; if (i < len) len = i-1;
     bitW = ~bitget (P->data,i-len,len) << (W-len);
     exc = 0; len = 0;
     while (bitW && (exc < W/2))  
		// either we shift it all or it only closes parentheses or too
		// many closed parentheses
        { W1 = (bitW >> (W-8));
          if (res = BwdPos[W1][exc]) return i-len-res;
          bitW <<= 8; exc += Excess[W1]; // note W1 is complemented!
	  len += 8;
	}
	// ok, it's not a small distance, try with hashing btable
     minres = 0;
     myexcess = excess (P,i) - 1;
     res = searchHash (P->bbtable,i,&h);
     while (res)
	{ if (!minres || (res < minres)) 
	     if ((i-res >= 0) && (excess(P,i-res) == myexcess)) 
		minres = res;
	  res = nextHash (P->bbtable,&h);
	}
     if (minres) return i-minres;
	// finally, it has to be a far pointer
     res = searchHash (P->sbtable,i,&h);
     while (res)
	{ if (!minres || (res < minres)) 
	     if ((i-res >= 0) && (excess(P,i-res) == myexcess))
		minres = res;
	  res = nextHash (P->sbtable,&h);
	}
     return i-minres; // there should be one if the sequence is balanced!
   }

	// # open - # close at position i, not included

uint excess (parentheses P, uint i)

   { return i - 2*rank(P->bdata,i);
   }

        // open position of closest parentheses pair that contains the pair
        // that opens at i, ~0 if no parent

uint enclose (parentheses P, uint i)

   { if (i == 0) return ~0; // no parent!
     return findparent (P,i);
   }



uint sizeofParentheses(parentheses P)
 {
    return sizeof(struct sparentheses) +
           sizeofBitmap(P->bdata) +
           sizeofHash(P->sftable) +
           sizeofHash(P->sbtable) +
           sizeofHash(P->bftable) +
           sizeofHash(P->bbtable); 
 }
