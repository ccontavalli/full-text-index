
 // Computes empirical k-th order entropy for all k, using a suffix array.
 // It works for all entropies at once (redirect the output!).
 // Context is given by characters following the current one, not preceding.
 // So results can be slightly different from those of entrop and entrop2
 // (look for comment "REVERSE" to achieve the same result of entrop[2])

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>

double entrop0 (unsigned long *freq, unsigned long tot)

  { double entr;
    int i;
    entr = tot * log (tot);
    for (i=0;i<256;i++)
	if (freq[i]) entr -= freq[i] * log(freq[i]);
    return entr;
  }

double calcentrop (unsigned char **A, unsigned long n, int k, 
		   unsigned long *ctx)

  { unsigned long freq[256];
    int j,i = 0;
    int c;
    double entrop = 0.0;
    *ctx = 0;
    while (i < n)
      { while ((i < n) && memcmp(A[i],A[i+1],k)) { (*ctx)++; i++; }
        j = i;
        for (c=0;c<256;c++) freq[c] = 0;
	do { freq[A[j++][k]]++; }
        while ((j <= n) && !memcmp(A[i],A[j],k));
        (*ctx)++; 
        entrop += entrop0(freq,j-i);
        i = j;
      }
    return entrop/((n+1)*log(2));
  }

int mystrcmp (unsigned char **s1, unsigned char **s2)

  { unsigned char *c1 = *s1, *c2 = *s2;
    int d;
    while (!(d = *c1++ - *c2++));
    return d;
  }

main (int argc, char **argv)

  { int t,i;
    int k,bord;
    char *iname;
    FILE *ifile;
    unsigned long tot;
    double entrop;
    struct stat sdata;
    unsigned char **A;
    unsigned char *T;
    unsigned long n,ctx;

    if (argc < 2)
	{ fprintf(stderr,"Usage: %s <file>\n"
			 "       gives all kth order entropies of <file> (1-byte chars)\n"
			 "Note: needs enough memory to build the suffix array.\n",argv[0]);
	  exit(1);
	}

    iname = argv[1];
    if (stat (iname,&sdata) != 0)
        { fprintf (stderr,"Error: cannot stat file %s\n",iname);
          fprintf (stderr," errno = %i\n",errno);
          exit(1);
        }
     n = sdata.st_size;

    ifile = fopen64(iname,"r");
    if (ifile == NULL)
	{ fprintf(stderr,"Error: cannot open %s for reading.\n",iname);
	  fprintf(stderr,"errno = %i\n",errno);
	  exit(1);
	}

     T = (void*)malloc ((n+1) * sizeof(unsigned char));
     if (T == NULL)
        { fprintf (stderr,"Error: not enough memory to build suffix array\n");
          fprintf (stderr," errno = %i\n",errno);
          exit(1);
        }

    if (fread (T,1,n,ifile) != n)
	{ fprintf(stderr,"Error: cannot read %s.\n",iname);
	  fprintf(stderr,"errno = %i\n",errno);
	  exit(1);
	}
    fclose (ifile);
// Uncomment this to REVERSE the text before computing entropy
// { int j;
//   for (j=0;j<n/2;j++) { int c = T[j]; T[j] = T[n-j-1]; T[n-j-1] = c; }
// }
    T[n] = 0;

    A = (void*)malloc ((n+1) * sizeof(unsigned char*));
    if (A == NULL)
        { fprintf (stderr,"Error: not enough memory to build suffix array\n");
          fprintf (stderr," errno = %i\n",errno);
          exit(1);
        }

    for (i=0;i<=n;i++) A[i] = T+i;
   
    qsort(A,n+1,sizeof(unsigned char *),mystrcmp);

    k = -1;
    do { entrop = calcentrop(A,n,++k,&ctx);
         printf ("\n\nFile %s has %i-th order entropy = %f bits/symbol (%i contexts)\n",
		iname,k,entrop,ctx); 
       }
    while (entrop > 0.0);
    exit(0);
  }
