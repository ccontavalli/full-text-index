#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "typedef.h"
#include "comparray5.h"
#include "interface.h"

static int strlen2(unsigned char *p)
{
  int l;
  l = 0;
  while (*p++ != 0) l++;
  return l;
}



/* 
 * Loads index from file(s) called filename[.<ext>] 
 */
int load_index(char *filename, void **index)
{
  CSA *csa;
  char *fname1,*fname2;
  int fnamelen;

  fnamelen = strlen2(filename);
  fname1 = malloc(fnamelen + 5);
  fname2 = malloc(fnamelen + 5);
  if (fname1 == NULL || fname2 == NULL) {
    printf("load_index:1. not enough mem.\n");
    exit(1);
  }
  sprintf(fname1,"%s.bwt",filename);
  sprintf(fname2,"%s.idx",filename);

  csa = malloc(sizeof(CSA));
  if (csa == NULL) {
    printf("load_index:2. not enough mem.\n");
    exit(1);
  }
  csa_read(csa,fname1,fname2);
  *index = (void *)csa;

  free(fname2);
  free(fname1);
  return 0;
}



/*
 * Frees the memory occupied by index.
 */
int free_index(void *index)
{
  CSA *csa;

  csa = (CSA *)index;
  free(csa->W);
  free(csa->SA);
  free(csa->ISA);
  free(csa->RL);
  free(csa->RM);
  free(csa);

  return 0;
}


/*  
 * Writes in numocc the number of occurrences of pattern[0..length-1] in 
 * index. 
 */
int count(void *index, uchar *pattern, ulong length, ulong *numocc)
{
  CSA *csa;
  sa_t l,r;

#if 0
  {
    int i;
    printf("count: pattern=");
    for (i=0;i<(int)length;i++) putchar(pattern[i]);
    printf("\n");
  }
#endif
  
  csa = (CSA *)index;
  csa_bsearch(csa, pattern, (int)length, &l, &r);
  *numocc = (ulong)(r - l + 1);

  return 0;
}


/*  
 * Writes in numocc the number of occurrences of pattern[0..length-1] in 
 * index. It also allocates occ (which must be freed by the caller) and 
 * writes the locations of the numocc occurrences in occ, in arbitrary 
 * order.
 */
int locate(void *index, uchar *pattern, ulong length, ulong **occ, 
		   ulong *numocc)
{
  CSA *csa;
  sa_t l,r;
  ulong *buf;
  ulong i,oc;

  csa = (CSA *)index;
  csa_bsearch(csa, pattern, (int)length, &l, &r);
  oc = (ulong)(r - l + 1);

  buf = malloc((*numocc) * sizeof(ulong));
  if (buf == NULL) {
    printf("locate: not enough mem.\n");
    exit(1);
  }

  for (i=0; i<oc; i++) {
    buf[i] = csa_lookup(csa,l + i) - 1;
  }

  *numocc = oc;
  *occ = buf;
  return 0;
}

 
/*	
 * Allocates snippet (which must be freed by the caller) and writes 
 * text[from..to] on it. Returns in snippet_length the length of the 
 * snippet actually extracted (that could be less than to-from+1 if to is 
 * larger than the text size). 
 */
int extract(void *index, ulong from, ulong to, uchar **snippet,
			ulong *snippet_length)
{
  CSA *csa;
  uchar *text;
  int i,len;

  csa = (CSA *)index;

  from++;  to++;
  if (to > csa->n) to = csa->n;

  len = (int)to - (int)from + 1;
  text = malloc(len);

  csa_text(text,csa,(sa_t)from,(sa_t)to);

  *snippet = text;
  *snippet_length = (ulong)len;
  return 0;
}

/*
 * Displays the text (snippet) surrounding any occurrence of the substring 
 * pattern[0..length-1] within the text indexed by index. 
 * The snippet must include numc characters before and after the pattern
 * occurrence, totalizing length+2*numc characters, or less if the 
 * text boundaries are reached. Writes in numocc the number of occurrences,
 * and allocates the arrays snippet_text and snippet_lengths 
 * (which must be freed by the caller). 
 * The first is a character array of numocc*(length+2*numc) 
 * characters, with a new snippet starting at every multiple of 
 * length+2*numc. 
 * The second gives the real length of each of the numocc snippets.
 */

int display(void *index, uchar *pattern, ulong length, ulong numc, 
			ulong *numocc, 	uchar **snippet_text, ulong **snippet_len)
{
  printf("display: not supported.\n");
  return 0;
}


/* 
 * Obtains the length of the text represented by index. 
 */
int get_length(void *index, ulong *length)
{
  *length = ((CSA *)(index))->n;
  return 0;
}


/*
 * Writes in size the size, in bytes, of index. This should be the size
 * needed by the index to perform any of the operations it implements.
 */
int index_size(void *index, ulong *size)
{
  *size = (ulong)(((CSA *)(index))->size);
  return 0;
}

int index_size_count(void *index, ulong *size)
{
  *size = (ulong)(((CSA *)(index))->size_count);
  return 0;
}

/*
 * The integer returned by each procedure indicates an error code if
 * different of zero. In this case, the error message can be displayed 
 * by calling error_index(). We recall that text and pattern indexes start
 * at zero.
 */
char* error_index(int e)
{
  printf("error_index: %d\n",e);
  return "hoge";
}


/* 
 * Creates index from text[0..length-1]. Note that the index is an 
 * opaque data type. Any build option must be passed in string 
 * build_options, whose syntax depends on the index. The index must 
 * always work with some default parameters if build_options is NULL. 
 * The returned index is ready to be queried. 
 */
int build_index(uchar *text, ulong length, char *build_options, void **index)
{
  CSA *csa;
  int D,D2,L;

  i64 *x, *p, *pi;
  i64 i, n, k, l,m;

  csa = malloc(sizeof(CSA));
  if (csa==NULL) {
    fprintf(stderr, "build_index: malloc failed 0\n");
    return 1;
  }

  csa->two = 64;
  csa->two2 = csa->two*4;
  csa->lmb = 8;

  parse_options(csa, build_options);

  D = csa->two;
  D2 = csa->two2;
  L = 1 << csa->lmb;
  fprintf(stderr,"build_index: D=%d D2=%d L=%d\n",D,D2,L);

  n = (i64)length;

  p=(i64 *)malloc((n+1)*sizeof *p);
  x=(i64 *)malloc((n+1)*sizeof *x);

  if (p==NULL || x==NULL) {
    fprintf(stderr, "malloc failed 1p\n");
    return 1;
  }
  pi = x;
  for (i=0; i<n; i++) pi[i] = text[i];

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

   p[0] = n;
   for (i=0; i<=n; ++i) p[i]++;  /* p[1..n]‚É1..n‚ª“ü‚Á‚Ä‚¢‚éBp[0]=n+1*/

   csa_new(csa,n,p,text,NULL,NULL,D,D2,L);
   //csa_write(csa,fname1,fname2);

   *index = csa;

  //printf("build_index: not supported.\n");
  return 0;
}

/*
 * Saves index on disk by using single or multiple files, having 
 * proper extensions. 
 */
int save_index(void *index, char *filename)
{
  CSA *csa;
  int i,len;
  char *fname1,*fname2;
  
  csa = (CSA *)index;

  len = strlen2(filename);
  fname1 = malloc(len+5);
  fname2 = malloc(len+5);
  if (fname1 == NULL || fname2 == NULL) {
    fprintf(stdout,"save_index: not enough mem.\n");
    exit(1);
  }

  sprintf(fname1,"%s.bwt",filename);
  sprintf(fname2,"%s.idx",filename);

  csa_write(csa,fname1,fname2);
   
  free(fname1);
  free(fname2);

  return 0;
}

/* 
 * Save index on memory 
 */
int save_index_mem(void *index, uchar *compress)
{
  printf("save_index_mem: not supported.\n");
  return 99;
}

/* 
 * Load index from memory 
 */
int load_index_mem (void ** index, uchar *compress, ulong size)
{
  printf("load_index_mem: not supported.\n");
  return 99;
}

int parse_options(CSA *csa, char *optionz) {
	
  int i, numtoken = 1;
  
  if (optionz == NULL) 
    return 0;
  char *options = malloc(sizeof(char)*(strlen(optionz)+1));
  if (options == NULL) 
    return -1;
  memcpy(options,optionz,sizeof(char)*(strlen(optionz)+1));
  
  i = 0;
  while (options[i] != '\0' )
    if(options[i++] == ' ')	 numtoken++;
  
  int j = 0;
  char *array[numtoken];
  array[0] = options;
  for(i = 0; i<numtoken-1; i++) {
    while(options[j] != ' ') j++;	
    options[j++] = '\0';
    array[i+1] = options+j;	
  }
  
  i = 0;
  while(i<numtoken) {
    if(!strcmp(array[i], "-D")) {
      if (i+1<numtoken ) {
	csa->two = atoi(array[++i]); 	
	csa->two2 = csa->two * 4;
	i++;
	continue;
      } else return -1;}

    if(!strcmp(array[i], "-D2")) {
      if (i+1<numtoken ) {
	csa->two2 = atoi(array[++i]); 
	i++;
	continue;
      } else return -1; }

    if(!strcmp(array[i], "-L")) {
      if (i+1<numtoken ) {
	csa->lmb = atoi(array[++i]); 
	if (csa->lmb < DD/logSIGMA) {
	  printf("error: L should be >= %d\n",DD/logSIGMA);
	  exit(1);
	}
	i++;
	continue;
      } else return -1;}
    
    return -1;
  }

  free(options);
  
  return 0;
	
}
