/* >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
   lcp_aux.c 
   Ver 1.0    28-apr-2004
   Functions for lightweight computation of the LCP array.

   Copyright (C) 2004 Giovanni Manzini (manzini@mfn.unipmn.it)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

   See COPYRIGHT file for further copyright information	   
   >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> */
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "bwt_aux.h"

/* ==================================================================
   Description of the data:
   text: [0,n-1] -> [0,255]
   suffix array: [1,n] -> [0,n-1], 
      sa[i] is the starting position of the i-th suffix 
      in lexicographic order
   lcp: [1,n]->[0,n-1],
      lcp[i] is the length of the LCP between t[sa[i],n-1] and 
      its predecessor in the lexicographic order (which is t[sa[i-1],n-1]).
      lcp[1] is undefined since t[sa[1],n-1] has no predecessor
  ======================================================================== */

/* ***********************************************************************
    Compute the lcp array from the suffix array
    using the algorithm by Kasai et al. (CPM '01)
    input
      t[0,n-1] input text 
      sa[1,n] suffix array
    return
      lcp array, or NULL if an error (e.g. out of memory) occurs.
   *********************************************************************** */
int *_lcp_sa2lcp_13n(uint8 *t, int n, int *sa)
{
  int i,h,k,j, *lcp, *rank;

  lcp = (int*)malloc((n+1)*sizeof(int));
  rank = (int*)malloc(n*sizeof(int));
  if(lcp==NULL || rank==NULL)
    return NULL;

  // compute rank = sa^{-1}
  for(i=1;i<=n;i++) rank[sa[i]] = i;
  // traverse suffixes in rank order
  h=0;
  for(i=0;i<n;i++) {
    k = rank[i];          // rank of s[i ... n-1]
    if(k>1) {
      j = sa[k-1];        // predecessor of s[i ... n-1]
      while(i+h<n && j+h<n && t[i+h]==t[j+h])
	h++;
      lcp[k] = h;
    }
    if(h>0) h--;
  }
  free(rank);
  return lcp;
}

/* ***********************************************************************
    Compute the lcp array from the suffix array
    input
      t[0,n-1] input text 
      sa[1,n] suffix array
    return
      lcp array, or NULL if an error (e.g. out of memory) occurs.
      This is alg. in Sect. 4 of "Space Conscious LCP computation" 26/3/04
   *********************************************************************** */
int *_lcp_jk_13n(uint8 *t, int n, int *sa)
{
  int i,h,j,k;
  int *lcp, *plcp, *pred;

  lcp = (int*)malloc((n+1)*sizeof(int));
  plcp = (int*)malloc(n*sizeof(int));  // lcp values ordered by starting position not rank
  pred = plcp;                         // lexicographic predecessors
  if(lcp==NULL || plcp==NULL)
    return NULL;

  // ---------- compute lexicographic predecessors
  for(k=2; k<=n; ++k) 
    pred[sa[k]] = sa[k-1];

  // --------- compute lcps into plcp (overwriting pred)
  h=0;
  for(i=0;i<n;i++) {     // this is the usual kasai's loop
    if (i != sa[1]) {
      j = pred[i];
      while(i+h<n && j+h<n && t[i+h]==t[j+h])
	h++;
      plcp[i] = h;
    }
    if(h>0) h--;
  }
  // --------- move lcps into the correct order
  for(k=2; k<=n; ++k) {
    lcp[k] = plcp[sa[k]];
  }
  free(plcp);
  return lcp;
}

/* ***********************************************************************
   space economical computation of the lcp array
   This procedure is similar to the previous one, but we compute the rank
   of i using the rank_next map (stored in the rn array) instead of
   the rank array. The advantage is that as soon as we have read rn[k] that
   position is no longer needed and we can use it to store the lcp.
   Thus, rn[] and lcp[] share the same memory and the overall space
   requirement of the procedure is 9n instead of 13n.
    input
      t[0,n-1] input text 
      sa[1,n] suffix array
      occ[0,_BW_ALPHA_SIZE-1]  # of occurrences of each char
    return
      lcp array, or NULL if an error (e.g. out of memory) occurs.
    space
     9n
   *********************************************************************** */
int *_lcp_sa2lcp_9n(uint8 *t, int n, int *sa, int *occ)
{
  int i,h,j,k,nextk=-1;
  int *rn, *lcp;

  lcp = (int*)malloc((n+1)*sizeof(int));
  if(lcp==NULL)
    return NULL;  // out of memory

  // rn and lcp are pointers to the same array
  rn = lcp;
  // compute rank_next map
  k = _bw_sa2ranknext(t, n, sa, occ, rn); // k is the rank of s
  for(h=i=0; i<n; i++,k=nextk) {
    assert(k>0 && i==sa[k]); 
    nextk=rn[k];          // read nextk before it is overwritten
    if(k>1) {
      // compute lcp between suffixes of rank k and k-1 (recall i==sa[k])
      j = sa[k-1];
      while(i+h<n && j+h<n && t[i+h]==t[j+h])
	h++;
      lcp[k]=h;           // h is the lcp, write it to lcp[k];  
    }
    if(h>0) h--;
  }
  assert(nextk==0);        // we have reached the last suffix s[n-1]
  return lcp;
}



/* ***********************************************************************
   space economical computation of the lcp array based on the bwt
    input
      t[0,n-1] input text 
      b->bwt[0,n] b->eof_pos   bwt of t[0,n-1]
      sa[1,n] suffix array
      occ[0,_BW_ALPHA_SIZE-1]  # of occurrences of each char
    output 
      the lcp array is overwritten to sa
    return
      num = number of extra int32 used by the algorithm, 
      or -1 if an error (e.g. out of memory) occurs.
    space
     (6 + delta)n, where delta=(4*num)/n
   *********************************************************************** */
int _lcp_sa2lcp_6n(uint8 *t, bwt_data *b, int *sa, int *occ)
{
  int i,h,j,k,num,num2,nextk=-1,n=b->size;
  int *rn=sa, *lcp=sa, *sa_aux;

  // ----- dirty trick: make b->bwt[eof_pos] different from 
  //       b->bwt[eof_pos-1] and b->bwt[eof_pos+1]
  assert(b->eof_pos!=0);
  b->bwt[b->eof_pos] = (uint8) (b->bwt[b->eof_pos-1] ^ 1); // change 0th bit
  if(b->eof_pos<n && b->bwt[b->eof_pos]==b->bwt[b->eof_pos+1])
    b->bwt[b->eof_pos] = (uint8) (b->bwt[b->eof_pos] ^ 2); // change 1st bit
    
  // ----- count # of sa positions that we need later
  for(num=0,i=2; i<=n; i++) 
    if(b->bwt[i-1]!=b->bwt[i]) num++;
  assert(num>0 && num<=n);

  // ---------- alloc extra memory -----------
  sa_aux = (int *) malloc(num*sizeof(int));
  if(sa_aux==NULL)
    return -1;
 
  // ---------- compute the ranknext map from the bwt ----
  k = _bw_bwt2ranknext(b, occ, rn);   // k is the rank of t[0,n-1]

  // ---------- convert rn->sa (in place) and save useful sa positions
  for(num2=i=0; k!=0; ) {
    assert(i<n);
    if( k>1 && (b->bwt[k-1]!=b->bwt[k]) )
      sa_aux[num2++] = k-1;
    nextk = rn[k];
    sa[k] = i++;
    k = nextk;
  }
  assert(num2==num);      // all needed positions have been recorded
  assert(i==n);           // we have reached the last suffix t[n-1]

  // ------ save sa values in sa_aux
  for(i=0;i<num;i++) {
    assert(sa_aux[i]>0 && sa_aux[i]<=n);
    sa_aux[i]=sa[sa_aux[i]];
  }

  // ------ now compute the lcp
  k = _bw_bwt2ranknext(b, occ, rn);   // k is the rank of t[0,n-1]
  for(num2=h=i=0; i<n; i++,k=nextk) {
    assert(k>0);
    nextk=rn[k];               // read nextk before it is overwritten
    if(k>1) {
      if(b->bwt[k]!=b->bwt[k-1]) {
	j = sa_aux[num2++];    // retrieve sa[k-1]
	while(i+h<n && j+h<n && t[i+h]==t[j+h]) 
          h++;                 // extend lcp if possible
      }
      lcp[k]=h;                // h is the lcp, write it to lcp[k];  
    }
    if(h>0) h--;
  }
  assert(nextk==0);        // we have reached the last suffix t[n-1]
  assert(num2==num);
  free(sa_aux);
  return num;
}




/* ***********************************************************************
    Compute the lcp array from the suffix array using the algorithm 
    by V. Makinen (Fund. Inf. '03), in which plcp is transformed in place 
    into lcp using the algorithm of Karkkainen (see 9125jk)
    input
      t[0,n-1] input text 
      sa[1,n] suffix array
    return
      lcp array, or NULL if an error (e.g. out of memory) occurs.
   *********************************************************************** */
#define MARKER (1<<31)
int *_lcp_vmjk_9125n(uint8 *t, int n, int *sa)
{
  int i,h,k,j, end, *lcp, *plcp, *rank;

  lcp = (int*)malloc((n+1)*sizeof(int));
  if(lcp==NULL)
    return NULL;

  // --- the array lcp[] stores rank->plcp->lcp (in this order) 
  rank=plcp=lcp;

  // compute rank = sa^{-1}
  for(i=1;i<=n;i++) rank[sa[i]] = i;

  // traverse suffixes in rank order
  for(h=i=0;i<n;i++) {
    k = rank[i];          // rank of s[i ... n-1]
    if(k>1) {
      j = sa[k-1];        // predecessor of s[i ... n-1]
      while(i+h<n && j+h<n && t[i+h]==t[j+h])
	h++;
      plcp[i] = h |MARKER;         // h is lcp[k]=lcp[rank[i]]
    }
    if(h>0) h--;
  }
  plcp[n] = plcp[sa[1]] = MARKER;
  // plcp[i] is the lcp between t[i,n-1] and its lex. predecessor
  // hence plcp: [0,n-1] -> [0,n-1], plcp[sa[1]] is undefined

  // now the lcp values have been computed but they are in
  // in the wrong positions....

   // ------ move lcps into the correct order ---------------------
   for(k=n; k>0; --k) {    // starting from end due to the non-cycle
     if(plcp[k] & MARKER) {
       i=k, j=sa[k];
       if(k==n) end=0; else end=k;    // there is one non-cycle that
                                      // starts at n and ends at 0
       h = plcp[end] & ~MARKER;
       while(j != end) {
         lcp[i] = plcp[j] & ~MARKER;
         i = j; j = sa[j];
       }
       lcp[i] = h;
     }
   }
   return lcp;
}
