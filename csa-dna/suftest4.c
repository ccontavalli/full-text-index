#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#if 1
 #include <sys/timeb.h>
#else
 #include <sys/time.h>
 #include <sys/resource.h>
#endif
#include "typedef.h"
#include "comparray5.h"

#ifndef min
#define min(x,y) ((x)<(y)?(x):(y))
#endif

#if 1
typedef struct timeb mytimestruct;
void mygettime(mytimestruct *t)
{
  ftime(t);
}
double mylaptime(mytimestruct *before,mytimestruct *after)
{
  double t;
  t = after->time - before->time;
  t += (double)(after->millitm - before->millitm)/1000;
  return t;
}
#else
typedef mytimestruct struct rusage;
void mygettime(mytimestruct *t)
{
  getrusage(RUSAGE_SELF,t);
}
double mylaptime(mytimestruct *before,mytimestruct *after)
{
  double t;
  t = after->ru_utime.tv_sec - before->ru_utime.tv_sec;
  t += (double)(after->ru_utime.tv_usec
		- before->ru_utime.tv_usec)/1000000;
  return t;
}
#endif

int setbit(unsigned short *B, sa_t i,int x);
int getbit(unsigned short *B, sa_t i);

int strlen2(unsigned char *p)
{
  int l;
  l = 0;
  while (*p++ != 0) l++;
  return l;
}

unsigned char key[25600];
int main(int argc, char *argv[])
{
  sa_t i,n;
  CSA csa;
  mytimestruct before,after;
  double t;
  char fname1[128],fname2[128];

   if (argc<2) {
      fprintf(stderr, "syntax: suftest file\n");
      return 1;
   }

   sprintf(fname1,"%s.bwt",argv[1]);
   sprintf(fname2,"%s.idx",argv[1]);
   //if (argc>=3) sprintf(fname2,"%s",argv[2]);
   csa_read(&csa,fname1,fname2);
   n = csa.n;

#if 0
   printf("BWT ");
   for (i=0; i<n; i++) {
	   char cvt[4] = {'a','c','g','t'};
	   printf("%c",cvt[csa_W(&csa,i)]);
   }
   printf("\n");

   printf("rank a: ");
   for (i=0; i<n; i++) printf("%d ",csa_rankc(&csa,i,convertchar('a')));
   printf("\n");
   printf("rank c: ");
   for (i=0; i<n; i++) printf("%d ",csa_rankc(&csa,i,convertchar('c')));
   printf("\n");
   printf("rank g: ");
   for (i=0; i<n; i++) printf("%d ",csa_rankc(&csa,i,convertchar('g')));
   printf("\n");
   printf("rank t: ");
   for (i=0; i<n; i++) printf("%d ",csa_rankc(&csa,i,convertchar('t')));
   printf("\n");

   for (i=0; i<=n; i++) {
	   printf("LF[%d] = %d\n",i,csa_LF(&csa,i));
   }

   {
	   int l,r;
	   unsigned char *p = "cg";
	   csa_bsearch(&csa,p,2,&l,&r);
	   printf("search (%s) = [%d,%d]\n",p,l,r);

   }

   for (i=1; i<=n; i++) {
	   printf("SA[%d] = %d\n",i,csa_lookup(&csa,i));
   }
   for (i=1; i<=n; i++) {
	   printf("ISA[%d] = %d\n",i,csa_inverse(&csa,i));
   }

#endif


#if 0
   {
     int i,j;
     mygettime(&before);
     i = csa_inverse(&csa,n);
     for (j=0; j<csa.n; j++) {
       char cvt[4] = {'a','c','g','t'};
       printf("%c",cvt[csa_W(&csa,i)]);
       i = csa_LF(&csa,i);
     }
     mygettime(&after);
     t = mylaptime(&before,&after);
     fprintf(stderr,"text %f sec\n",t);
//     exit(0);
   }
#endif
#if 0
   {
     int j,m;
     char buf[100];
     mygettime(&before);
	 m = min(n,100);
     csa_text(buf,&csa,1,m);
     for (j=0; j<m; j++) {
       printf("%c",buf[j]);
     }
     mygettime(&after);
     t = mylaptime(&before,&after);
     fprintf(stderr,"text %f sec\n",t);
     //exit(0);
   }
#endif
#if 1
   {
     int j,j2;
     mygettime(&before);
     for (j=0; j<n; j++) {
       i = csa_LF(&csa,j);
       //printf("%u %u\n",j,i);
       j2 = csa_psi(&csa,i);
       if (j != j2) {
	 printf("LF[%d] = %d, Psi[%d] = %d\n",j,i,i,j2);
       }
     }
     mygettime(&after);
     t = mylaptime(&before,&after);
     fprintf(stderr,"LF %f sec\n",t);
    // exit(0);
   }
#endif
#if 0
   {
     int j;
     mygettime(&before);
     for (i=1; i<=min(10000,n); i++) {
       j = csa_lookup(&csa,i);
     //  printf("%u %u\n",i,j);
       if (i % 1000 == 0) {
	 //printf("%10u\r",i);
	 fflush(stdout);
       }
     }
     mygettime(&after);
     t = mylaptime(&before,&after);
     fprintf(stderr,"lookup %f sec\n",t);
   //  exit(0);
   }
#endif
#if 0
   mygettime(&before);
   for (i = 0; i < n; i++) {
     int c;
     c = csa_W(&csa,i);
   }
   mygettime(&after);
   t = mylaptime(&before,&after);
   fprintf(stderr,"W %f sec\n",t);
#endif
#if 0
   mygettime(&before);
   for (i = 1; i <= n; i++) {
     int j;
     if (i % 1000 == 0) {
       printf("%u\r",i);
       fflush(stdout);
     }
     j = csa_inverse(&csa,i);
#if 1
     if (csa_lookup(&csa,j) != i) {
       printf("error %d %d\n",i,j);
     }
#endif
   }
   mygettime(&after);
   t = mylaptime(&before,&after);
   fprintf(stderr,"inverse %f sec\n",t);
#endif

//   printf("Hit enter key\n");	getchar();

   return 0;
}
