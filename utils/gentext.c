
// Generates a random file

#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>

static int Seed;
#define ACMa 16807
#define ACMm 2147483647
#define ACMq 127773         
#define ACMr 2836
#define hi (Seed / ACMq)
#define lo (Seed % ACMq)
  
static int fst=1;

	/* fills A[0..n-1] with characters in the range 1..s */

void fill (unsigned char *A, unsigned long n, int s)

   { unsigned long i;
     long test;
     struct timeval t;
     if (fst)
	{ gettimeofday (&t,NULL);
          Seed = t.tv_sec*t.tv_usec;
	  fst=0;
	}
     for (i=0;i<n;i++)
       { Seed = ((test = ACMa * lo - ACMr * hi) > 0) ? test : test + ACMm;
         A[i] = 1 + ((double)Seed) * s/ACMm;
       }
   }

static unsigned char text[1024*1024];

main (int argc, char **argv)

   { unsigned long n;
     int s;
     FILE *f;

     if (argc < 3)
        { fprintf (stderr,"Usage: %s <file> <n> <sigma>\n"
		          " generates <file> with <n> megabytes of text"
			  " uniformly distributed in [1,<sigma>]\n",argv[0]);
          exit(1);
        }

     n = atoi(argv[2]);
     s = atoi(argv[3]);
     
     if ((n < 0) || (n > 4096))
        { fprintf (stderr,"Error: file length (which is given in MB) must"
			  " be between 0 and 4096 (= 4 GB file)\n");
          exit(1);
        }

     if ((s < 1) || (s > 256))
        { fprintf (stderr,"Error: alphabet size must be between 1 and 256\n");
          exit(1);
        }

     if (s == 256)
        { fprintf (stderr,"Warning: several indexes need alphabet size < 256"
			  " to work properly.\n Generating anyway.\n");
	}

     f = fopen (argv[1],"w");
     if (f == NULL)
        { fprintf (stderr,"Error: cannot open %s for writing\n",argv[1]);
 	  fprintf (stderr," errno = %i\n",errno);
          exit(1);
        }

     while (n--)
        { fill (text,1024*1024,s);
          if (fwrite (text,1024*1024,1,f) != 1)
             { fprintf (stderr,"Error: cannot write %s\n",argv[1]);
 	       fprintf (stderr," errno = %i\n",errno);
	       fclose(f);
	       unlink (argv[1]);
               exit(1);
             }
	}

     if (fclose(f) != 0)
        { fprintf (stderr,"Error: cannot write %s\n",argv[1]);
 	  fprintf (stderr," errno = %i\n",errno);
	  unlink (argv[1]);
          exit(1);
        }

     fprintf (stderr,"File %s successfully generated\n",argv[1]);

     exit (0);
   }
