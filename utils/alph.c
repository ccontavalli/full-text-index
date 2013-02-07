
 // Counts alphabet size of a file

#include <stdio.h>
#include <errno.h>

#include "common.h"

unsigned char buf[1024*1024];

unsigned long long sigma[256];


main (int argc, char **argv)

  { int t,n;
    char *iname;
    FILE *ifile;
    unsigned long long tot;
    double prob,tprob;

    if (argc < 2)
	{ fprintf(stderr,"Usage: %s <file>\n"
			 "       gives alphabet statistics of <file> (1-byte chars)\n",argv[0]);
	  exit(1);
	}

    iname = argv[1];

    for (n=0;n<256;n++) sigma[n] = 0;

    ifile = fopen64(iname,"r");
    tot = 0;
    if (ifile == NULL)
	{ fprintf(stderr,"Error: cannot open %s for reading.\n",iname);
	  fprintf(stderr,"errno = %i\n",errno);
	  exit(1);
	}

    while (n = fread (buf,1,1024*1024,ifile))
        { tot += n;
	  while (n--) sigma[buf[n]]++;
        }

    fclose (ifile);

    t = 0;
    tprob = 0.0;
    printf ("Chars in %s:",iname);
    for (n=0;n<256;n++) 
       if (sigma[n]) 
	  { t++;
	    printf (" %i",n);
	    prob = sigma[n]/(double)tot;
	    tprob += prob*prob;
	  }

    printf ("\n\nFile %s has %i different characters\n",iname,t); 
    printf ("Inverse prob. of matching is %f\n",1/tprob);

    exit(0);
  }
