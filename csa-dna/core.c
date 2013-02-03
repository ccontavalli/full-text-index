#include "typedef.h"
#include "comparray5.h"

#ifdef BIT64
static pb popcount2(pb x)
{
  pb r;
  r = x;
  r = ((r >> 2) + r) & 0x3333333333333333L;
  r = ((r>>4) + r) & 0x0f0f0f0f0f0f0f0fL;
  r = (r>>8) + r;
  r = (r>>16) + r;
  r = (r>>32) + r;
  r = r & 127;
  return r;
}
#else
static pb popcount2(pb x)
{
  pb r;
  r = x;
  r = ((r >> 2) + r) & 0x33333333;
  r = ((r>>4) + r) & 0x0f0f0f0f;
  r = (r>>8) + r;
  r = (r>>16) + r;
  r = r & 31;
  return r;
}
#endif
sa_t csa_rankc_slow(CSA *csa, sa_t i, int c);


/* The most important function                              */
/* that computes the number of occurrences of c in W[0..i]. */
#ifdef BIT64
sa_t csa_rankc(CSA *csa, sa_t i, int c)
{
  sa_t j,r;
  sa_t i2;
  //sa_t i3;
  int lmb;
  pb x,m,*p;
  static pb masktbl[4] = {0,0x5555555555555555L,0xAAAAAAAAAAAAAAAAL,0xFFFFFFFFFFFFFFFFL};

  //  i3 = i;
  if (i >= csa->last) i--;

  lmb = csa->lmb;
  r = csa->RL[(i>>logLB)*SIGMA + c] + csa->RM[(i>>lmb)*SIGMA + c];
  i2 = (i>>lmb) << lmb;
  p = &csa->W[i2/(DD/logSIGMA)];

  for (j=0; j+(DD/logSIGMA)-1 <= (i-i2); j+=(DD/logSIGMA)) {
	x = (*p++) ^ masktbl[c];
	x = (x | (x>>1)) & 0x5555555555555555L;
	r += (DD/logSIGMA) - popcount2(x);
  }
  x = (*p) ^ masktbl[c];
  x = (x | (x>>1)) & 0x5555555555555555L;
  m = 0x5555555555555555L >> (((i-i2) - j + 1) * logSIGMA);
  x |= m;
  r += (DD/logSIGMA) - popcount2(x);

#if 0
  if (r != csa_rankc_slow(csa,i3,c)) {
	  printf("rankc: error i=%d c=%d rank=%d (%d)\n",i3,c,r,csa_rankc_slow(csa,i3,c));
  }
#endif
  return r;
}

#else

sa_t csa_rankc(CSA *csa, sa_t i, int c)
{
  sa_t j,r;
  sa_t i2;
//  sa_t i3;
  int lmb;
  pb x,m,*p;
  static pb masktbl[4] = {0,0x55555555,0xAAAAAAAA,0xFFFFFFFF};
//  i3 = i;
  if (i >= csa->last) i--;

  lmb = csa->lmb;
  r = csa->RL[(i>>logLB)*SIGMA + c] + csa->RM[(i>>lmb)*SIGMA + c];
  i2 = (i>>lmb) << lmb;
  p = &csa->W[i2/(DD/logSIGMA)];

  for (j=0; j+(DD/logSIGMA)-1 <= (i-i2); j+=(DD/logSIGMA)) {
	x = (*p++) ^ masktbl[c];
	x = (x | (x>>1)) & 0x55555555;
	r += (DD/logSIGMA) - popcount2(x);
  }
  x = (*p) ^ masktbl[c];
  x = (x | (x>>1)) & 0x55555555;
  m = 0x55555555 >> (((i-i2) - j + 1) * logSIGMA);
  x |= m;
  r += (DD/logSIGMA) - popcount2(x);

#if 0
  if (r != csa_rankc_slow(csa,i3,c)) {
	  printf("rankc: error i=%d c=%d rank=%d (%d)\n",i3,c,r,csa_rankc_slow(csa,i3,c));
  }
#endif
  return r;
}

#endif

#if 0
sa_t csa_rankc_slow(CSA *csa, sa_t i, int c)
{
  int j;
  sa_t r;
  sa_t i2;
  int mb;

  if (i >= csa->last) i--;

  mb = 1 << csa->lmb;
  r = csa->RL[(i/LB)*SIGMA + c] + csa->RM[(i/mb)*SIGMA + c];
  printf("true r=%d ",r);
  i2 = (i/mb)*mb;
  for (j=0; j <= (i % mb); j++) {
	  if (csa_W(csa,i2+j) == c) r++;
  }
  printf("true r=%d\n",r);
  return r;
}
#endif

int csa_W(CSA *csa,sa_t i)
{
  pb x;
	//  if (i == csa->last) return -1;
//  if (i > csa->last) i--;
//    return getbits(csa->W,i*logSIGMA,logSIGMA);
  x = csa->W[i/(DD/logSIGMA)];
  x = (x >> (DD-logSIGMA-(i % (DD/logSIGMA))*logSIGMA)) & (SIGMA-1);
  return x;
}

sa_t csa_LF(CSA *csa, sa_t i)  /* 0 <= i <= n */
{
  int c;
  sa_t j;

  if (i == csa->last) return 0;
  if (i > csa->last) i--;
  c = csa_W(csa,i);
  j = csa->C[c] + csa_rankc(csa, i, c);
  if (i >= csa->last) j++;
  return j;
}

sa_t csa_psi(CSA *csa, sa_t i)  /* 0 <= i <= n */
{
  int c;
  sa_t j;
  sa_t l,r,m;
 
  if (i==0) return csa->last;
  
  i--;
  c = 0;
  while (csa->C[c+1] <= i) c++;
  i -= csa->C[c];

  l = 0; r = csa->n - 1;
  while (l < r) {
    m = (l+r)/2;
    if (i < csa_rankc(csa,m,c)) r = m;  else l = m+1;
  }
  j = r;

  //if (j >= csa->last) j++;
  return j;
}

int csa_bsearch(CSA *csa, unsigned char *P, int len, sa_t *ll, sa_t *rr)
{
  int c;
  sa_t l,r;
  c = convertchar(P[len-1]);
  l = csa->C[c]+1;  r = csa->C[c+1];

  while (--len > 0) {
    c = convertchar(P[len-1]);
    l = csa->C[c] + csa_rankc(csa, l-1,c)+1;
    r = csa->C[c] + csa_rankc(csa,r,c);
  }
  *ll = l;  *rr = r;
  return 0;
}

/* calculate SA[i] */
sa_t csa_lookup(CSA *csa, sa_t i)
{
  sa_t v,t;
  v = 0;  t = csa->two-1;
  while ((i & t) !=0) {
    i = csa_LF(csa,i);
    v++;
  }
//  i = i / two;
  i = i >> csa->logt;
  return csa->SA[i]+v;
}

sa_t csa_inverse(CSA *csa, sa_t suf)
{
  sa_t p,pos;
  sa_t t;
  
  t = (sa_t)csa->two2 - 1;

  if (suf+t < csa->n) {
//    p = ((suf+two2-1)/two2)*two2;
    p = (suf+t) & (~t);
    pos = csa->ISA[p >> csa->logt2];
    while (p > suf) {
      pos = csa_LF(csa,pos);
      p--;
	}
  } else {
    p = csa->n+1;
    pos = 0;
    while (p > suf) {
      pos = csa_LF(csa,pos);
      p--;
	}
  }
  return pos;
}


void csa_text(unsigned char *p,CSA *csa,sa_t s,sa_t t)
{
  sa_t i,i2;
  int j;
  static char cvt[4] = {'a','c','g','t'};

#if DEBUG
  if (s < 1 || t > csa->n || s > t) {
	  printf("csa_text: out of range [%d,%d] n=%d\n",s,t,csa->n);
  }
#endif

  i = csa_inverse(csa,(t+1));
  for (j=t-s; j>=0; j--) {
    i2 = i;
    if (i2 >= csa->last) i2--;
    p[j] = cvt[csa_W(csa,i2)];
    i = csa_LF(csa,i);
  }
}
