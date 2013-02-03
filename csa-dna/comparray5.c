#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
//#include <sys/mman.h>
#include "typedef.h"
#include "comparray5.h"

int msize=0;

static int blog(int x)
{
int l;
  l = -1;
  while (x>0) {
    x>>=1;
    l++;
  }
  return l;
}

int getbit(pb *B, sa_t i)
{
  sa_t j,l;
  j = i / DD;
  l = i & (DD-1);
  return (B[j] >> (DD-1-l)) & 1;
}


pb getbits(pb *B, sa_t i, int w)
{
	int j;
	sa_t x;

	x = 0;
	for (j=0; j<w; j++) {
		x <<= 1;
		x += getbit(B,i+j);
	}
	return x;

}


int setbit(pb *B, sa_t i,pb x)
{
  sa_t j,l;
  j = i / DD;
  l = i % DD;
  if (x==0) B[j] &= (~(1L<<(DD-1-l)));
  else if (x==1) B[j] |= (1L<<(DD-1-l));
  else {
    printf("error setbit x=%d\n",x);
    exit(1);
  }
  return x;
}

int setbits(pb *B, sa_t i, pb x, int w)
{
	int j;

	for (j=0; j<w; j++) {
		setbit(B,i+j,(x>>(w-1-j)) & 1);
	}
	return 0;
}

int convertchar(unsigned char t)
{
  int c = 0;
  switch (t) {
  case 'a':  c = 0;  break;
  case 'c':  c = 1;  break;
  case 'g':  c = 2;  break;
  case 't':  c = 3;  break;
  default:  printf("error char = %c [%02x]\n",t,t);
  }
  return c;
}

void bwt(CSA *csa, i64 *p, unsigned char *T)
{
  sa_t n;
  i64 i,j;
  int x,c;
  pb *W;

  n = csa->n;

  mymalloc(W,n/(DD/logSIGMA)+1,0);
  csa->W = W;

  j = 0;
  for (i = 0; i <= n; i++) {
    x = p[i]-1;
    if (x==0) {
      csa->last = i;
      j++;
    } else {
      c = convertchar(T[x]);
      setbits(W,(i-j)*logSIGMA,c,logSIGMA);
    }
  }
}

void make_tbl(CSA *csa)
{
  sa_t n;
  i64 i,sum;
  i64 C[SIGMA];
  int c,mb;
  pb *W;

  n = csa->n;
  W = csa->W;
  mb = 1 << csa->lmb;

  mymalloc(csa->RL,((n+LB-1)/LB+1)*SIGMA,0);
  mymalloc(csa->RM,((n+mb-1)/mb+1)*SIGMA,0);

  for (i=0;i<((n+LB-1)/LB)*SIGMA;i++) csa->RL[i] = 0;
  for (i=0;i<((n+mb-1)/mb)*SIGMA;i++) csa->RM[i] = 0;

  for (c = 0; c < SIGMA; c++) C[c] = 0;

  for (i = 0; i < n; i++) {
	  if (i % LB == 0) {
		  for (c = 0; c < SIGMA; c++) {
			  csa->RL[(i / LB)*SIGMA + c] = C[c];
		  }
	  }
	  if (i % mb == 0) {
		  for (c = 0; c < SIGMA; c++) {
			  csa->RM[(i / mb)*SIGMA + c] = (u16)(C[c] - csa->RL[(i / LB)*SIGMA + c]);
		  }
	  }
	  c = csa_W(csa,i);
	  C[c]++;
  }

  sum = 0;
  for (i = 0; i < SIGMA; i++) {
    sum = sum + C[i];
    C[i] = sum - C[i];
    csa->C[i] = C[i];
  }
  csa->C[SIGMA] = sum; /* !!! */
}


void writeint(i64 x,FILE *f)
{
  sa_t tmp;
  if (x<0 && 0) {
    printf("writeint: x %ld %u\n",x,(sa_t)x);
    exit(1);
  }
  tmp = (sa_t)x;
  fwrite(&tmp,sizeof(sa_t),1,f);
}

void writelong(pb x,FILE *f)
{
  u32 tmp;
#ifdef BIT64
  tmp = x >> 32;
  fwrite(&tmp,sizeof(u32),1,f);
  tmp = x & 0xffffffff;
  fwrite(&tmp,sizeof(u32),1,f);
#else
  fwrite(&x,sizeof(pb),1,f);
#endif
}

i64 readint(FILE *f)
{
  sa_t tmp;
  i64 x;
  fread(&tmp,sizeof(sa_t),1,f);
  x = tmp;
  return x;
}

u64 readlong(FILE *f)
{
  u32 tmp;
  pb x;
#ifdef BIT64
  fread(&tmp,sizeof(u32),1,f);
  x = (u64)tmp << 32;
  fread(&tmp,sizeof(u32),1,f);
  x += tmp;
#else
  fread(&tmp,sizeof(u32),1,f);
  x = tmp;
#endif
  return x;
}

void csa_new(CSA *csa,i64 n, i64 *p, unsigned char *s, char *fname1, char *fname2,int D, int D2,int mb)
{
  i64 i;
  sa_t x;
  i64 psize,isize;

  csa->n = n;
  csa->two = D;
  csa->two2 = D2;
  csa->lmb = blog(mb);

  if (mb != (1<<csa->lmb)) {
    printf("error mb=%d\n",mb);
    exit(1);
  }

  if (mb < (DD/logSIGMA)) {
    printf("L should be >= %d\n",(DD/logSIGMA));
    exit(1);
  }

  bwt(csa,p,s-1);
  make_tbl(csa);

  mymalloc(csa->SA,n/D+1,0);
  mymalloc(csa->ISA,n/D2+1,0);

  csa->SA[0] = 0;
  for (i=D; i<=n; i+=D) {
	  csa->SA[i/D] = p[i];
  }

  csa->ISA[0] = 0;
  for (i=1; i<=n; i++) {
    if (p[i] % D2 == 0) {
		csa->ISA[p[i] / D2] = i;
    }
  }

  //  csa_write(csa,fname1,fname2);

}

int csa_write(CSA *csa,char *fname1,char *fname2)
{
  i64 i;
  sa_t x;
  i64 psize,isize;
  FILE *f1,*f2;
  i64 n;
  int D,D2,mb;

  n = csa->n;
  D = csa->two;
  D2 = csa->two2;
  mb = 1 << csa->lmb;

  f1 = fopen(fname1,"wb"); /* bwt */
  f2 = fopen(fname2,"wb"); /* directory */
  if (f1 == NULL || f2 == NULL) {
    perror("csa2_new1: ");
    exit(1);
  }

  psize = isize = 0;

  for (i=0; i<n/(DD/logSIGMA)+1; i++) {
    writelong(csa->W[i],f1);
	psize += sizeof(pb);
  }

  writeint(VERSION,f2); /* version */
  writeint(n,f2);   /* length of the text */
  writeint(csa->last,f2);
  writeint(mb,f2); /* interval between two psi values stored explicitly */
  writeint(D,f2); /* interval between two SA values stored explicitly */
  writeint(D2,f2); /* interval between two inverse SA values stored explicitly */
  writeint(SIGMA,f2);   /* alphabet size */
  isize += 6*sizeof(sa_t);

  for (i=0; i<SIGMA+2; i++) writeint(csa->C[i],f2);
  isize += (SIGMA+2) * sizeof(sa_t);

  for (i=0; i<((n+LB-1)/LB)*SIGMA; i++) {
    writeint(csa->RL[i],f2);
	isize += sizeof(sa_t);
  }
  for (i=0; i<((n+mb-1)/mb)*SIGMA+1; i+=2) {
    x = csa->RM[i] << 16;
	x += csa->RM[i+1];
	writeint(x,f2);
	isize += sizeof(sa_t);
  }

  for (i=0; i<n/D+1; i++) {
    writeint(csa->SA[i],f2);
	isize += sizeof(sa_t);
  }
  for (i=0; i<n/D2+1; i++) {
    writeint(csa->ISA[i],f2);
	isize += sizeof(sa_t);
  }

  fclose(f1);
  fclose(f2);

  printf("n     %u\n",(sa_t)n);
  printf("bwt   %u bytes (%1.3f bpc)\n",(sa_t)psize,(double)psize*8/n);
  printf("Total %u bytes (%1.3f bpc)\n",(sa_t)(psize+isize),(double)(psize+isize)*8/n);


  csa->size = psize+isize;
  csa->size_count = psize+isize - sizeof(sa_t) * (n/D+1 + n/D2+1);

}

int csa_read(CSA *csa,char *fname1,char *fname2)
{
  sa_t i,n,m;
  FILE *f1,*f2;
  int psize,isize;
  int D,D2,mb;

  f1 = fopen(fname1,"rb");
  if (f1 == NULL) {
    perror("csa2_read1: ");
    exit(1);
  }
  fseek(f1,0,SEEK_END);
  psize = ftell(f1);
  fseek(f1,0,SEEK_SET);

  f2 = fopen(fname2,"rb");
  if (f2 == NULL) {
    perror("csa2_read3: ");
    exit(1);
  }
  fseek(f2,0,SEEK_END);
  isize = ftell(f2);
  fseek(f2,0,SEEK_SET);

  i = readint(f2); /* version */
  if (i != VERSION) {
    printf("Version %d is not supported.\n",i);
    exit(1);
  }

  csa->n = n = readint(f2);   /* length of the text */
  csa->last = readint(f2);
  mb = readint(f2); /* interval between two psi values stored explicitly */
  csa->lmb = blog(mb);
  csa->two = D = readint(f2); /* interval between two SA values stored explicitly */
  csa->two2 = D2 = readint(f2); /* interval between two inverse SA values stored explicitly */

  if (mb != (1<<csa->lmb)) {
	  printf("error mb=%d\n",mb);
	  exit(1);
  }
  if (mb < (DD/logSIGMA)) {
    printf("L should be >= %d\n",(DD/logSIGMA));
    exit(1);
  }

  csa->logt  = blog(D);
  csa->logt2 = blog(D2);
  if (D != (1<<csa->logt)) {
	  printf("error D=%d\n",D);
	  exit(1);
  }
  if (D2 != (1<<csa->logt2)) {
	  printf("error D2=%d\n",D2);
	  exit(1);
  }

#if 1
  fprintf(stderr,"D=%d (stores SA for every D)\n",csa->two);
  fprintf(stderr,"mb=%d\n", mb);
  fprintf(stderr,"n     %u\n",csa->n);
  fprintf(stderr,"bwt   %d bytes (%1.3f bpc)\n",psize,(double)psize*8/n);
  fprintf(stderr,"Total %d bytes (%1.3f bpc)\n",psize+isize,(double)(psize+isize)*8/n);
#endif
  if ((m=readint(f2)) != SIGMA) {   /* alphabet size */
    printf("error sigma=%d\n",m);
  }

  for (i=0; i<SIGMA+2; i++) csa->C[i] = readint(f2);


  mymalloc(csa->W,n/(DD/logSIGMA)+1,0);
  for (i=0; i<n/(DD/logSIGMA)+1; i++) {
    csa->W[i] = readlong(f1);
  }

  mymalloc(csa->SA,n/D+1,0);
  mymalloc(csa->ISA,n/D2+1,0);

  mymalloc(csa->RL,((n+LB-1)/LB+1)*SIGMA,0);
  mymalloc(csa->RM,((n+mb-1)/mb+1)*SIGMA,0);
  for (i=0; i<((n+LB-1)/LB)*SIGMA; i++) {
    csa->RL[i] = readint(f2);
  }
  for (i=0; i<((n+mb-1)/mb)*SIGMA+1; i+=2) {
    sa_t x;
    x = readint(f2);
    csa->RM[i] = x >> 16;
	csa->RM[i+1] = x & 0xffff;
  }

  for (i=0; i<n/D+1; i++) {
    csa->SA[i] = readint(f2);
  }
  for (i=0; i<n/D2+1; i++) {
    csa->ISA[i] = readint(f2);
  }

  //  csa->size = psize+isize;
  //  csa->size_count = psize+isize - sizeof(sa_t) * (n/D+1 + n/D2+1);

  fclose(f1);
  fclose(f2);

  return 0;
}

