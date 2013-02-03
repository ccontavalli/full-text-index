/* >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
   bwtopt.c 
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


/* --- read proptotypes and typedef for ds_ssort --- */
#include "ds_ssort.h"
#include "bwt_aux.h"
#include "lcp_aux.h"

#define ALPHA_SIZE _BW_ALPHA_SIZE



// ----- global variables -------
int Verbose;                    // verbosity level 
double Lambda;                  // used for entropy estimation
//static FILE *Outfile;         // output file 
static FILE *Infile;            // input file 
static int Infile_size;         // size input file;

// ----- prototypes ----------
void fatal_error(char *);
void out_of_mem(char *f);
double getTime ( void );


/* ***************************************************************
      1. Read Infile and store it to text[]. 
      2. Compute the bwt and the lcp arrays
      3. Compute an optimal paritioning and compress
   *************************************************************** */
void compress_file(void)
{
  double bwt_partition1(bwt_data *b, int *lcp);
  double pseudo_compr(uint8 *t, int size);
  uint8 *text;
  int *sa, n, overshoot, i, k, extra_bytes;
  int occ[ALPHA_SIZE], *lcp=NULL;
  bwt_data b;
  double start, end, estimate, apost_est=0, apost_est_tot=0;
  int bloque1,bloque2;

  // ----- init ds suffix sort routine -----
  overshoot=init_ds_ssort(500,2000);
  if(overshoot==0)
    fatal_error("ds initialization failed! (compress_file)\n");
  // ----- allocate text and suffix array -----
  n = Infile_size;                               // length of input text
  sa=malloc((n+1)*sizeof *sa);                   // suffix array
  text=malloc((n+overshoot)*sizeof *text);       // text
  if (! sa || ! text) out_of_mem("compress_file");
  // ----- read text and build suffix array ------
  rewind(Infile); 
  i=fread(text, (size_t) 1, (size_t) n, Infile);
  if(i!=n) fatal_error("Error reading the input file!");
  fprintf(stdout,"File size: %d bytes\n",n);
  // ----- build suffix array ----------------
  start = getTime();
  ds_ssort(text,sa+1,n);                         // sort suffixes
  end=getTime();
  fprintf(stdout,"Suffix array construction: %.2f seconds\n",end-start);
  // ---- compute lcp using 6n algorithm ---------
  start = getTime();
  for(i=0;i<ALPHA_SIZE;i++) occ[i]=0;
  for(i=0;i<n;i++) occ[text[i]]++;
  if( (b.bwt = (uint8 *) malloc(n+1)) == NULL)
    out_of_mem("bwtopt1_file");
  _bw_sa2bwt(text, n, sa, &b);
  extra_bytes = _lcp_sa2lcp_6n(text,&b,sa,occ);
  lcp = sa;
  end=getTime();
  fprintf(stdout,"lcp6 construction: %.2f seconds\n",end-start);
  fprintf(stdout,"Total memory for lcp6: %.2fn bytes\n",
	  6+(4.0*extra_bytes)/n);
  // ---- compute the optimal partition ---------
  start = getTime();
  estimate = bwt_partition1(&b,lcp);
  end=getTime();
  bloque1 = 0;
  bloque2 = lcp[0];
  while ( bloque2 != n+1 ) {
    printf("[%7d,%7d]\n", bloque1, bloque2-1);
    for (i=bloque1; i<=bloque2-1; i++)
      printf("%d ", b.bwt[i]);
    printf("\n");
    bloque1 = bloque2;
    bloque2 = lcp[bloque2];
  }
  printf("[%7d,%7d]\n", bloque1,bloque2-1);
  fprintf(stdout,"Optimal partition computation: %.2f seconds\n",end-start);
  // ---- compress --------
  for(k=i=0;i<=n; ) { 
    assert(lcp[i]>i);   // bwt[i] -> bwt[lcp[i]-1] is a segment
    if(Verbose>0) {
      apost_est_tot += apost_est = pseudo_compr(b.bwt+i,lcp[i]-i);
      if(Verbose>1)
	fprintf(stderr,"%d) %d <-> %d: %f bits\n",k,i,lcp[i]-1,apost_est);
    }
    i = lcp[i];      // starting point of next segment
    k++;             // increase # of segment 
  } 
  assert(i==n+1);
  fprintf(stdout, "Number of partitions: %d\n",k);
  fprintf(stdout, "Estimated compressed size: %lf (not reliable)\n",estimate);
  if(Verbose>0) {
    fprintf(stdout, "A posteriori estimate: %lf  ",apost_est_tot);
    fprintf(stdout, "Delta %lf (should be zero)\n",apost_est_tot-estimate);
  }
  free(b.bwt);
  free(text);
  free(sa);
}

/* ***************************************************************
   File compression via the compression booster 
   *************************************************************** */
int main(int argc, char *argv[])
{  
  double getTime ( void );
  void open_files(char *infile_name);
  void lcp_file(void);
  double end, start;
  char *infile_name;
  int c;
  extern char *optarg;
  extern int optind, opterr, optopt;


  /* ------------- read command line options ----------- */
  Verbose=0;                       // be quiet by default
  Lambda = 1.0;
  infile_name=NULL;

  if(argc<2) {
    fprintf(stderr, "Usage:\n\t%s infile ",argv[0]);
    fprintf(stderr,"[-v] [-l lambda]\n");
    fprintf(stderr,"\t-l lambda    entropy estimator ");
    fprintf(stderr,"(default %.2f)\n",Lambda);
    fprintf(stderr,"\t-v           verbose output\n\n");
    exit(0);
  }

  opterr = 0;
  while ((c=getopt(argc, argv, "l:v")) != -1) {
    switch (c)
    {
      //      case 'o':
      //  outfile_name = optarg; break;
      case 'l':
	Lambda=atof(optarg); break;
      case 'v':
	Verbose++; break;
      case '?':
        fprintf(stderr,"Unknown option: %c -main-\n", optopt);
        exit(1);
    }
  }
  if(optind<argc)
     infile_name=argv[optind];

  // ----- check input parameters--------------
  if(infile_name==NULL) fatal_error("The input file name is required\n");

  if(Verbose>2) {
    fprintf(stderr,"\n*****************************************************");
    fprintf(stderr,"\n             bwtopt  Ver 1.0\n");
    fprintf(stderr,"Created on %s at %s from %s\n",__DATE__,__TIME__,__FILE__);
    fprintf(stderr,"*****************************************************\n");
  }
  if(Verbose>1) {
    fprintf(stderr,"Command line: ");
    for(c=0;c<argc;c++)
      fprintf(stderr,"%s ",argv[c]);
    fprintf(stderr,"\n");
  }

  // ---- do the work ---------------------------
  start = getTime();
  open_files(infile_name);
  compress_file();
  end = getTime();
  if(Verbose)
    fprintf(stderr,"Elapsed time: %f seconds.\n", end-start);
  return 0;
}



/* ***************************************************************
   open input and output files; initializes Outfile, Infile, Infile_size
   *************************************************************** */
void open_files(char *infile_name)
{
  FILE *fopen(const char *path, const char *mode);

  /* ------ open input and output files ------ */
  if(infile_name==NULL)
    fatal_error("Please provide the input file name (open_files)\n");
  else {
    Infile=fopen( infile_name, "rb"); // b is for binary: required by DOS
    if(Infile==NULL) {
      perror(infile_name);
      exit(1);
    }
  }
  // ---- store input file length in Infile_size
  if(fseek(Infile,0,SEEK_END)!=0) {
    perror(infile_name); exit(1);
  }
  Infile_size=ftell(Infile);
  if (Infile_size==-1) {
    perror(infile_name); exit(1);
  } 
  if (Infile_size==0) fatal_error("Input file empty (open_files)\n");
}


void fatal_error(char *s)
{
  fprintf(stderr,"%s",s);
  exit(1);
}

void out_of_mem(char *f)
{
  fprintf(stderr, "Out of memory in function %s!\n", f);
  exit(1);
}

double getTime ( void )
{
   double usertime,systime;
   struct rusage usage;

   getrusage ( RUSAGE_SELF, &usage );

   usertime = (double)usage.ru_utime.tv_sec +
     (double)usage.ru_utime.tv_usec / 1000000.0;

   systime = (double)usage.ru_stime.tv_sec +
     (double)usage.ru_stime.tv_usec / 1000000.0;

   return(usertime+systime);
}

