
 // Computes empirical k-th order entropy.
 // Could be made a bit more efficient

#include "common.h"

#include <stdio.h>
#include <errno.h>

#define maxk 8
#define hsize (1 << 25) // 512 MB of RAM required
#define PRIME1 ((unsigned)51493313)
#define PRIME2 ((unsigned)1334467)

unsigned char buf[1024*1024];

typedef unsigned char *Tseq;
typedef unsigned long *Tfreq;

typedef struct
  { unsigned char context[maxk];
    union
       { unsigned char chars[4];
	 Tseq seq;
	 Tfreq freq;
       } chardata;
    unsigned long tot;
  } cdata;

cdata data[hsize];

void addfreq (cdata *data, unsigned char symbol)

  { Tseq seq;
    Tfreq freq;
    int i;
    if (data->tot < 4) // inplace
       { data->chardata.chars[data->tot++] = symbol;
         return;
       }
    if (data->tot == 4) // convert to char seq
       { seq = (Tseq) malloc (64*sizeof(unsigned char));
	 if (seq == NULL)
	    { fprintf(stderr,"Error: cannot allocate memory.\n");
	      fprintf(stderr,"errno = %i\n",errno);
	      exit(1);
	    }
         memcpy (seq,data->chardata.chars,4);
         data->chardata.seq = seq;
       }
    if (data->tot < 64) // char seq
       { data->chardata.seq[data->tot++] = symbol;
         return;
       }
    if (data->tot == 64) // convert to freq table
       { freq = (Tfreq) malloc (256*sizeof(unsigned long));
	 if (freq == NULL)
	    { fprintf(stderr,"Error: cannot allocate memory.\n");
	      fprintf(stderr,"errno = %i\n",errno);
	      exit(1);
	    }
         seq = data->chardata.seq;
	 for (i=0;i<256;i++) freq[i] = 0;
         for (i=0;i<64;i++) freq[seq[i]]++;
	 free (seq);
         data->chardata.freq = freq;
       }
    data->chardata.freq[symbol]++;
    data->tot++;
  }

void insfreq (unsigned char *context, int k, unsigned char symbol)

  { unsigned okey,key = 0;
    int i;
    if (k == 0) key = 0;
    else {
      for (i=0;i<k;i++) key = key*PRIME1 + context[i];
      key &= (hsize-1);
      okey = key;
      while (data[key].context[0] && (memcmp(data[key].context,context,k)))
	{ key = (key+PRIME2) & (hsize-1);
	  if (key == okey)
	     { fprintf (stderr,"Hash table size (hsize) in entrop.c too small, "
			       "recompile.\n");
	       exit(1);
	     }
	}
      if (!data[key].context[0]) memcpy (data[key].context,context,k);
      }
    addfreq(data+key,symbol);
  } 

double entrop0 (Tfreq freq, unsigned long tot)

  { double entr = 0.0;
    double prop;
    int i;
    for (i=0;i<256;i++)
	if (freq[i])
	   { prop = freq[i]/(double)tot;
	     entr -= prop * log(prop);
	   }
    return entr;
  }

double calcentrop0 (int n)

  { unsigned long freq[256];
    int i;
    if (data[n].tot <= 4)
       { for (i=0;i<256;i++) freq[i] = 0;
         for (i=0;i<data[n].tot;i++)
	     freq[data[n].chardata.chars[i]]++;
	 return entrop0 (freq,data[n].tot);
       }
    if (data[n].tot <= 64)
       { for (i=0;i<256;i++) freq[i] = 0;
         for (i=0;i<data[n].tot;i++)
	     freq[data[n].chardata.seq[i]]++;
	 return entrop0 (freq,data[n].tot);
       }
    return entrop0 (data[n].chardata.freq,data[n].tot);
  }

main (int argc, char **argv)

  { int t,n,ctx;
    int k,bord;
    char *iname;
    FILE *ifile;
    unsigned long tot;
    double entrop;

    if (argc < 3)
	{ fprintf(stderr,"Usage: %s <file> <k>\n"
			 "       gives kth order entropy of <file> (1-byte chars)\n",argv[0]);
	  exit(1);
	}

    iname = argv[1];
    k = atoi(argv[2]);
    if ((k < 0) || (k > maxk))
	{ fprintf(stderr,"Error: k must be between 0 and %i\n",maxk);
	  exit(1);
	}

    ifile = fopen64(iname,"r");
    tot = 0;
    if (ifile == NULL)
	{ fprintf(stderr,"Error: cannot open %s for reading.\n",iname);
	  fprintf(stderr,"errno = %i\n",errno);
	  exit(1);
	}

    for (n=0;n<hsize;n++) 
       { data[n].tot = 0;
   	 data[n].context[0] = 0;
       }

    bord = 0;
    while (n = fread (buf+bord,1,1024*1024-bord,ifile))
        { tot += n;
	  for (t=k;t<n+bord;t++) insfreq (buf+t-k,k,buf[t]);
	  memcpy (buf,buf+t-k,k);
	  bord = k;
        }
    n -= k;

    fclose (ifile);

    entrop = 0.0; ctx = 0;
    for (n=0;n<hsize;n++) 
       if (data[n].tot) 
	  { entrop += calcentrop0(n) * data[n].tot;
	    ctx++; 
	  }
    entrop /= (tot * log(2));

    printf ("\n\nFile %s has %i-th order entropy = %f bits/symbol (%i contexts)\n",
		iname,k,entrop,ctx); 
    exit(0);
  }
