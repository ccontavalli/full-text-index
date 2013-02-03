/* suftest.c
   Copyright N. Jesper Larsson 1999.
   
   Program to test suffix sorting function. Reads a sequence of bytes from a
   file and calls suffixsort. This is the program used in the experiments in
   "Faster Suffix Sorting" by N. Jesper Larsson (jesper@cs.lth.se) and Kunihiko
   Sadakane (sada@is.s.u-tokyo.ac.jp) to time the suffixsort function in the
   file qsufsort.c.

   This software may be used freely for any purpose. However, distribution of
   the source code is permitted only for the complete file without any
   modifications. No warranty is given regarding the quality of this
   software.*/

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include "typedef.h"
#include "comparray5.h"

#ifndef COMPACT
#define COMPACT 2
#endif

void suffixsort(i64 *x, i64 *p, i64 n, i64 k, i64 l);

CSA SA;

int main(int argc, char *argv[])
{
   int i, j;
   i64 *x, *p, *pi;
   i64 n, k, l,m;
   unsigned char *s,*s2;
   char *fnam;
   char fname1[128];
   char fname2[128];
   FILE *f;
   int D,D2,L;
   CSA csa;

   if (argc<2) {
      fprintf(stderr, "syntax: %s D L file1 file2...\n",argv[0]);
      return 1;
   }
#if 1
   /*D = 32;
     L = 128;*/
   D = atoi(argv[1]);
   //   D2 = D*2;
   D2 = D*4;
   L = atoi(argv[2]);
   printf("D=%d (stores SA for every D)\n",D);
   printf("L=%d (directory for rank)\n",L);
#endif

   n = 0;
   for (i=3; i<argc; i++) {
     fnam = argv[i];
     if (! (f=fopen(fnam, "rb"))) {
       perror(argv[i]);
       return 1;
     }
     if (fseek(f, 0L, SEEK_END)) {
       perror(fnam);
       return 1;
     }
     m=ftell(f);
     printf("%s: %u bytes\n",fnam,(int)m);
     n += m;
     fclose(f);
   }

   if (n==0) {
      fprintf(stderr, "%s: file empty\n", fnam);
      return 0;
   }
   //printf("s=%d, %d, %d, %d\n",sizeof *p, sizeof(long), sizeof(int), sizeof( int ));
   //printf("sizeof p= %d\n",(sa_t)(n+1)*sizeof *p);
   p=(i64 *)malloc((n+1)*sizeof *p);

   //printf("size of x=%d\n",(sa_t)(n+1)*sizeof *x);
   x=(i64 *)malloc((n+1)*sizeof *x);

   if (p==NULL ) {
      fprintf(stderr, "malloc failed 1p\n");
      return 1;
   }
   if (! x ) {
      fprintf(stderr, "malloc failed 1x\n");
      return 1;
   }

#if 0
   for (rewind(f), pi=x; pi<x+n; ++pi) {
     c=getc(f);
     if (c==EOF) c = 0;
     *pi=c;
   }
#else
   pi = x;
   for (i=3; i<argc; i++) {
     fnam = argv[i];
     f=fopen(fnam, "rb");
     fseek(f, 0L, SEEK_END);  m=ftell(f);  rewind(f);
     for (j=0; j<m; j++) *pi++ = getc(f);
     fclose(f);
   }
   *pi = 0;
#endif
#if COMPACT==0
   l=0;
   k=UCHAR_MAX+1;
#elif COMPACT==2
   {
     i64 q[257];
     for (i=0; i<=UCHAR_MAX; ++i)
       q[i]=0;
     for (pi=x; pi<x+n; ++pi)
       q[*pi]=1;
     for (i=k=0; i<=UCHAR_MAX; ++i)
       if (q[i])
         q[i]=k++;
     for (pi=x; pi<x+n; ++pi)
       *pi=q[*pi]+1;
     l=1;
     ++k;
   }
#endif
   
   suffixsort(x, p, n, k, l);
   
   free(x);

   s=malloc(n * sizeof *s);
   if (! s) {
      fprintf(stderr, "malloc failed 3\n");
      return 1;
   }
   //rewind(f);
   fseek(f, 0L, SEEK_SET);

   s2 = s;
   for (i=3; i<argc; i++) {
     fnam = argv[i];
     f=fopen(fnam, "rb");
     fseek(f, 0L, SEEK_END);  m=ftell(f);  rewind(f);
     fread(s2,sizeof(*s),n,f);  s2 += m;
     fclose(f);
   }

   p[0] = n;
   for (i=0; i<=n; ++i) p[i]++;  /* p[1..n]に1..nが入っている。p[0]=n+1*/

   sprintf(fname1,"%s.bwt","new");
   sprintf(fname2,"%s.idx","new");

   csa_new(&csa,n,p,s,fname1,fname2,D,D2,L);
   csa_write(&csa,fname1,fname2);

   //   printf("write ok\n");

   return 0;
}
