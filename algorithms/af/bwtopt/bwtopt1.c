/* >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
   bwtopt1.c 
   Ver 1.0   17-may-04
   bwt optimal partitioning and compression using suffix tree visit

   Copyright (C) 2003 Giovanni Manzini (manzini@mfn.unipmn.it)

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
#include <string.h>
#include <unistd.h>
#include <sys/times.h>
#include <sys/resource.h>
#include <assert.h>
#include <math.h>

/* --- read proptotypes and typedef for ds_ssort --- */
#include "ds_ssort.h"
#include "bwt_aux.h"
#include "lcp_aux.h"
#include "bwtopt1.h"

#define ALPHA_SIZE _BW_ALPHA_SIZE


// typedef unsigned long long uint64;

// esternal global vars
//extern int Verbose;
extern double Lambda;

typedef struct {
  int *count;           // # occ of each char
  int count_tot;        // total # of chars
} stats;

typedef struct {
  int    depth;         // depth of the ancestor of this node   
  int    sa_start;      // first leaf of this subtree
  stats  *lstats;       // statistics (# occ) for the subtree leaves 
  double cost;          // cost of the optimal (so far) partitioning 
} st_node;


// ------------- prototypes ---------------
void fatal_error(char *);
static void out_of_mem(char *f) {
   fprintf(stderr, "Out of memory in function %s!n", f);
   exit(1);
}
double getTime ( void );
stats *allocate_stats(void);
stats *init_stats(stats *s, int c);
void merge_stats(stats *s1, stats *s2);
void reduce_stack(int d, int next_leaf);
void merge_top_elements(int s, int e, int next_leaf);
double comulative_cost(stats *s);
double pseudo_compr(uint8 *t, int size);
void push_node(int sa_pos, int lcp, int c);
void enlarge_stack(void);
void init_stack(void);
int remap_bwt(bwt_data *b);
void print_stack(int sa_max);
int free_stack(void);


// ------------ local global variables ---------------
static st_node *Stack;         // stack containing suffix tree nodes 
static int Top;                // pointer to the top of the stack
static int Stack_size;         // current size of the stack
static ulong *End_point;         // endpoints of segments
static int Alpha_size;
static double Log_Alpha_size;
// BEGIN ADD //
static int Bits_n;
inline int bits (int n){
  int b = 0;
  while (n) { b++; n >>= 1; }
    return b;
}

// END ADD //
static int Freq_old[ALPHA_SIZE];
static int Alpha_old[ALPHA_SIZE];
static int Alpha_new[ALPHA_SIZE];

//uint8 *BWT; // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

/* ********************************************************************
   This function computes the optiml partition of the BWT with respect
   to the cost function:
       C(s) = (#chars in s)log(|\Sigma|)+Lambda H_0^*(s)
   by means of a post-order suffix tree visit emulated via
   the a single pass over the suffix array. 
   The optimal partition of the BWT is written to the LCP array and is 
   given by:
        [0, t0-1] [t0, t1-1] [t1, t2-1] .... [tk,n]
   where t0 = lcp[0], t{i+1} = lcp[ti] and lcp[tk]=n+1
   ****************************************************************** */
double bwt_partition1(bwt_data *b, ulong *lcp,int alphasize)
//in my case I have the text previously remaped then only need to pass alphasize
{
  int i,n,depth,last_depth,stack_mem_usage;
  double optimal_cost;
  
  //BWT=b->bwt; // !!!!!!!!!!!!!!!!!!!!!!!
  Alpha_size = alphasize;//remap_bwt(b);
  Log_Alpha_size = log(Alpha_size);
  End_point = lcp;
  n = b->size;
  // BEGIN ADD //
  Bits_n=bits(n);
  // END ADD //
  init_stack();

  // insert leaves corresponding to # and first suffix
  //push_node(0, -1, b->bwt[b->eof_pos]);
  //push_node(1, 0, b->bwt[b->eof_pos+1]);
  push_node(0, -1, b->bwt[0]);
  push_node(1, 0, b->bwt[1]);
  last_depth = 0;
  for(i=2;i<=n;i++) {
    depth = (int) lcp[i];
    if(depth<last_depth) {
      reduce_stack(depth,i);
    }
    push_node(i, depth, b->bwt[i]);
    last_depth=depth;
  }
  reduce_stack(-1,n+1);    // reduce stack to suffx tree root 
  assert(Top==0);
  optimal_cost =  Stack[0].cost;
  stack_mem_usage = free_stack();
  //if(Verbose>0) 
    //fprintf(stderr,"Memory for suffix-tree stack: %d bytes\n",stack_mem_usage);
  return optimal_cost;
}


// remove from stack all elements with depth > d
void reduce_stack(int d, int next_leaf)
{
  int i,curr_depth;

  // this function should be called only if there is something to do
  assert(Top>=0);  
  assert(Stack[Top].depth>d);

  while(Stack[Top].depth>d) {
    curr_depth=Stack[Top].depth;
    // find first item with depth<d 
    for(i=Top-1;i>0;i--) {
      assert(Stack[i].depth<=curr_depth);
      if(Stack[i].depth<curr_depth)
	break;
    }
    assert(i>0 || d==-1);
    // find optimal encoding for the subtrees rooted at 
    //   Stack[i], Stack[i+1], ..., Stack[Top]
    // and create in Stack[i] a node for the parent-subtree
    merge_top_elements(i,Top,next_leaf);
    Top=i;
    assert(Top>=0);
  }
  assert(Top>0 || d==-1);
}


/* **********************************************************************
   this function check if the leaves of the subtrees rooted at
      Stack[s], Stack[s+1], ... , Stack[e]
   should be compressed together, or not.  
   ********************************************************************** */
void merge_top_elements(int s, int e, int next_leaf)
{
  int i;
  double sum, mergedcost;

#ifndef NDEBUG
  // --- check consistency of endpoints
  for(i=s;i<e;i++) {
    int ep = Stack[i].sa_start;
    while(ep<Stack[i+1].sa_start) {
      // sum += pseudo_compr(BWT+ep,End_point[ep]-ep);
      ep = End_point[ep];
    }
    assert(ep==Stack[i+1].sa_start);
    assert(Stack[i+1].sa_start-Stack[i].sa_start==Stack[i].lstats->count_tot);
    // if(fabs(sum-Stack[i].cost)>.01)
    //  fprintf(stderr,">>> something wrong at %d-%d  (%f vs %f)\n",
    //	      Stack[i].sa_start,ep,sum, Stack[i].cost);
  }
  // check last node
  {
    int ep = Stack[e].sa_start;
    while(ep<next_leaf) {
      // sum += pseudo_compr(BWT+ep,End_point[ep]-ep);
      ep = End_point[ep];
    }
    assert(ep==next_leaf);
    assert(next_leaf-Stack[e].sa_start==Stack[e].lstats->count_tot);
    // if(fabs(sum-Stack[e].cost)>.01)
    //   fprintf(stderr,">>> something wrong at %d-%d (%f vs %f)\n",
    //	      Stack[e].sa_start,ep,sum, Stack[e].cost);
  }
#endif

  // compute sum of single costs and comulative statistics
  sum = Stack[s].cost;
  for(i=s+1;i<=e;i++) {
    sum += Stack[i].cost;
    merge_stats(Stack[s].lstats, Stack[i].lstats);
  }
  // choose whether to merge segments or not
  mergedcost = comulative_cost(Stack[s].lstats);
  if(mergedcost<=sum) {
    Stack[s].cost = mergedcost;
    End_point[Stack[s].sa_start]=next_leaf; 
#if 0
    // !!!!! check validity of Stack[s].stats !!!!!!!!!!!!!!!!!!!!!!
    { int beg,end; double cc, pseudo_compr(uint8 *,int);
    void compare_stats(uint8 *,int,stats *);
      beg = Stack[s].sa_start;
      end = next_leaf;
      cc = pseudo_compr(BWT+beg,end-beg);
      if(fabs(cc-mergedcost)>.01) {
	fprintf(stderr,"#### Created %d %d, cost %f (%f)\n", 
		beg, end, mergedcost,cc);
	// compare_stats(BWT+beg,end-beg,Stack[s].lstats);
	//for(i=s;i<=e;i++) {
	//  fprintf(stderr,"%d] %d-%d (%d)\n",i,Stack[i].sa_start,
	//	  End_point[Stack[i].sa_start],Stack[i].lstats->count_tot);
	// } fprintf(stderr,"\n");
      }
    }
#endif
  }
  else 
    Stack[s].cost = sum;
}




/* *****************************************************************
   compute the cost of encoding a segment whose statistics are
   in *s. In this function the cost is defined as 
     #(dist. chars)log(Alpha_size) + Lambda H_0
   For greater speed, logarithms are to the base e since 
   a multiplicative constant does not affect the optimal partition
   ***************************************************************** */ 
double comulative_cost(stats *s)
{
  int i, numchar=0;
  double tot=0;

  assert(s->count_tot>0);
  for(i=0;i<Alpha_size;i++)
    if(s->count[i]!=0) {
      assert(s->count[i]>0);
      tot += s->count[i]*log(((double) s->count_tot)/s->count[i]);
      //printf("%f %f\n", s->count[i]*log(((double) s->count_tot)/s->count[i]),s->count[i]*log(((double) s->count_tot)/s->count[i])/log(2.0));
      numchar++;
    }
  assert(tot>=0);
  if(numchar == 1) 
  // BEGIN ADD //
    return Alpha_size * Bits_n + 32;
  return Alpha_size * Bits_n + 32 + 32 + (numchar + 2) * 8 + numchar*bits(numchar) + numchar*6*32 + 1.375 * tot/log(2.0);
  // END ADD //

  // BEGIN COMMENT //
    //return Log_Alpha_size + Lambda * (1+log(s->count_tot+1));
  //return numchar * Log_Alpha_size + Lambda * tot;
  // END COMMENT //
}

/* ******************************************************************
   compute the cost of compressing the segment  t[0], t[size-1] 
   according to the function comulative_cost(). The purpose is only 
   to double check the correctness of the value returned as the cost  
   of the optimal partition.
   ****************************************************************** */ 
double pseudo_compr(uint8 *t, int size)
{
  stats *s, *allocate_stats(void);
  int free_stats(stats *t);
  int i;
  double cost;

  s = allocate_stats();
  // fill stats with # of occ in bwt[0] ... bwt[size-1] 
  for(i=0;i<size;i++) {
    assert(t[i]<Alpha_size);
    s->count[t[i]]++;
  }
  s->count_tot = size;
  // compute cost of partition 
  cost = comulative_cost(s);
  free(s);
  return cost;
}

void compare_stats(uint8 *t, int size,stats *w)
{
  stats *s, *allocate_stats(void);
  int free_stats(stats *t);
  int i;

  s = allocate_stats();
  // fill stats with # of occ in bwt[0] ... bwt[size-1] 
  for(i=0;i<size;i++) {
    assert(t[i]<Alpha_size);
    s->count[t[i]]++;
  }
  s->count_tot = size;
  // compute cost of partition 
  if(size!=w->count_tot)
    fprintf(stderr,">>> %d vs %d\n",size,w->count_tot); 
  free(s);
  return;
}



// ############################################################
// functions for handling stack
// ############################################################

// add a node to the stack
void push_node(int sa_pos, int lcp, int c)
{
  if(++Top==Stack_size)
    enlarge_stack();
  assert(Top<Stack_size);

  Stack[Top].sa_start = sa_pos;
  Stack[Top].depth = lcp;
  Stack[Top].lstats = init_stats(Stack[Top].lstats, c);
  Stack[Top].cost = comulative_cost(Stack[Top].lstats);
  End_point[sa_pos] = sa_pos+1;
}

// enlarge the global stack by 1000 positions
void enlarge_stack()
{
  int i,old_size;

  old_size = Stack_size;
  Stack_size += 1000;

  Stack = (st_node *) realloc(Stack,Stack_size*sizeof(st_node));
  if(Stack==NULL) 
    out_of_mem("enlarge_stack");
  for(i=old_size;i<Stack_size;i++) 
    Stack[i].lstats = NULL;
}

void init_stack(void)
{
  Stack=NULL;
  Stack_size = 0;
  Top=-1;
}

// count and deallocate the memory used for the stack
int free_stack(void)
{
  int free_stats(stats *t);
  int i,mem=0;

  for(i=Stack_size-1;i>=0;i--) {
    if(Stack[i].lstats!=NULL)
      mem += free_stats(Stack[i].lstats);
  }
  mem += Stack_size*sizeof(st_node);
  free(Stack);
  init_stack();
  return mem; 
}

void print_stack(int sa_max)
{
  int i,j,limite;

  printf("----%d----\n",sa_max);
  for(i=0;i<=Top;i++) {
    if(i==Top)
      limite=sa_max;
    else
      limite=Stack[i+1].sa_start;
    printf("%d] %d (c=%f) ",i,Stack[i].sa_start,Stack[i].cost);
    printf("d=%d ",Stack[i].depth);
    for(j=End_point[Stack[i].sa_start]; ;j=End_point[j]) {
      printf("%d ",j);
      if(j>=limite) break;
    }
    printf("\n");
  }
  printf("----\n");
}



// ############################################################
// functions for handling statistics
// ############################################################

/* ****************************************************************
   allocate a structure for containing the statics of a 
   bwt segment (that is, the leaves of a suffix-tree subtree)
   **************************************************************** */
stats *allocate_stats(void)
{
  stats *t;

  t=(stats *) malloc(sizeof(stats));
  if(t==NULL)
    out_of_mem("allocate_stats (1)");
  t->count_tot=0;
  t->count = (int *) calloc(Alpha_size,sizeof(int));
  if(t->count==NULL)
    out_of_mem("allocate_stats (2)");  
  return t;
}

int free_stats(stats *t)
{
  free(t->count);
  free(t);
  return sizeof(stats) + Alpha_size*sizeof(int); 
}

stats *init_stats(stats *s, int c)
{
  int i;

  assert(c>=0 && c<Alpha_size); 
  if(s==NULL)
    s = allocate_stats();
  for(i=0;i<Alpha_size;i++)
    s->count[i]=0;
  s->count[c] = s->count_tot = 1;
  return s;
}

/* ******************************************************************
   add the statistics in *s2 to those in *s1
   ****************************************************************** */
void merge_stats(stats *s1, stats *s2)
{
  int i;

  for(i=0;i<Alpha_size;i++)
    s1->count[i] += s2->count[i];
  s1->count_tot += s2->count_tot;
}


// ################# remapping #########################
int remap_bwt(bwt_data *b)
{
  int i,j,size=0;
  
  assert(b->eof_pos>0);
  b->bwt[b->eof_pos] = b->bwt[b->eof_pos-1];  // avoid special case
  // init freq old
  for(i=0;i<ALPHA_SIZE;i++)
    Freq_old[i]=0;
  // compute freq old 
  for(i=0;i<=b->size;i++) {
    //assert(b->bwt[i]<ALPHA_SIZE);
    if(Freq_old[b->bwt[i]]++==0) size++;
  }
  // remap alphabet
  for(i=j=0;j<ALPHA_SIZE;j++) 
    if(Freq_old[j]!=0) {
      Alpha_new[j]=i;
      Alpha_old[i++]=j;
    }
  assert(i==size);
  // remap bwt
  for(i=0;i<=b->size;i++) {
    b->bwt[i]=Alpha_new[b->bwt[i]];
    assert(b->bwt[i]<size);
  }
  return size;  
}

