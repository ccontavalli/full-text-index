/* compressed suffix arrays 

 */

#ifndef _COMPARRAY_H_
#define _COMPARRAY_H_

#define VERSION 3

typedef u32 sa_t;

#ifdef BIT64
 typedef u64 pb;
 #define logDD 6
#else
 typedef u32 pb;
 #define logDD 5
#endif
#define DD (1<<logDD)

//#define USE_MMAP

#ifdef USE_MMAP
#include "mman.h"
#endif

#ifndef min
#define min(x,y) ((x)<(y)?(x):(y))
#endif
#ifndef max
#define max(x,y) ((x)>(y)?(x):(y))
#endif

#define logSIGMA 2
#define SIGMA (1<<logSIGMA)

//#define SB 32
//#define MB 256
#define logLB 16
#define LB (1<<logLB)

typedef struct csa {
  sa_t n; /* length of the text */
  sa_t last; /* lex order of the longest suffix */
  int two; /* interval between two SA values stored explicitly */
  int two2; /* interval between two inverse SA values stored explicitly */
  int logt;
  int logt2;
  int lmb;

  sa_t C[SIGMA+2]; /* table to convert character code to cumulative frequency*/
  sa_t *SA,*ISA;

  sa_t *RL;
  u16 *RM;

  pb *W; /* bwt */
  int size;
  int size_count;
} CSA;

extern int msize;
#define mymalloc(p,n,f) {p = malloc((n)*sizeof(*p)); msize += (f)*(n)*sizeof(*p); /* if (f) printf("malloc %d bytes at line %d total %d\n",(n)*sizeof(*p),__LINE__,msize);  */ if ((p)==NULL) {printf("not enough memory (%d bytes) in line %d\n",msize,__LINE__); exit(1);};}


int csa_W(CSA *csa,sa_t i);

/* calculate SA[i] */
sa_t csa_lookup(CSA *SA, sa_t i);
sa_t csa_lookup_test(CSA *SA, sa_t i);

/* calculate Psi[i] */
sa_t csa_psi(CSA *SA,sa_t i);

/* calculate LF[i] */
sa_t csa_LF(CSA *SA,sa_t i);

/* calculate SA^{-1}[i] */
sa_t csa_inverse(CSA *SA, sa_t suf);

/* decode T[i..j] into p */
void csa_text(unsigned char *p,CSA *SA,sa_t i,sa_t j);

/* decode T[SA[pos]..SA[pos]+len-1] into p */
void csa_substring(unsigned char *p,CSA *SA,sa_t pos,sa_t len);

void csa_new(CSA *SA,i64 n, i64 *p, unsigned char *s, char *fname1, char *fname2,int D,int D2,int L);
int csa_write(CSA *SA,char *fname1,char *fname2);
int csa_read(CSA *SA,char *fname1,char *fname2);

sa_t csa_rankc(CSA *csa, sa_t i, int c);

int csa_bsearch(CSA *csa, unsigned char *P, int len, sa_t *ll, sa_t *rr);

int convertchar(unsigned char t);

#endif
