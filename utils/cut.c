
 // Cuts a file, leaving a prefix of a given number of megabytes

#include <stdio.h>
#include <errno.h>

#include "common.h"

char buf[1024*1024];

main (int argc, char **argv)

  { int n;
    char *iname;
    char oname[1024];
    FILE *ifile,*ofile;

    if (argc < 3)
	{ fprintf(stderr,"Usage: %s <file> <mb> generates <file>.<mb>MB \n"
			 "       with the first <mb> megabytes of <file>.\n",
			 argv[0]);
	  exit(1);
	}

    iname = argv[1];
    n = atoi(argv[2]);

    if (n <= 0) 
	{ fprintf(stderr,"Usage: cut <file> <mb> generates <file>.<mb>MB \n"
			 "       with the first <mb> megabytes of <file>.\n");
	  fprintf(stderr,"So <mb> must be positive!\n");
	  exit(1);
	}

    sprintf(oname,"%s.%iMB",iname,n);

    ifile = fopen64(iname,"r");
    if (ifile == NULL)
	{ fprintf(stderr,"Error: cannot open %s for reading.\n",iname);
	  fprintf(stderr,"errno = %i\n",errno);
	  exit(1);
	}

    ofile = fopen64(oname,"w");
    if (ifile == NULL)
	{ fprintf(stderr,"Error: cannot open %s for writing.\n",oname);
	  fprintf(stderr,"errno = %i\n",errno);
	  exit(1);
	}

    while (n--)
	{ if (fread (buf,1024*1024,1,ifile) != 1)
	     { fprintf(stderr,"Error: cannot read %s\n",iname);
	       fprintf(stderr,"errno = %i\n",errno);
	       if (errno == 0)
	          fprintf(stderr,"Maybe the file is too short?\n");
	       fclose (ifile); fclose (ofile);
	       unlink (oname);
	       exit(1);
	     }
	  if (fwrite (buf,1024*1024,1,ofile) != 1)
	     { fprintf(stderr,"Error: cannot write %s\n",oname);
	       fprintf(stderr,"errno = %i\n",errno);
	       fclose (ifile); fclose (ofile);
	       unlink (oname);
	       exit(1);
	     }
	}

    fclose (ifile);
    if (fclose (ofile) != 0)
       { fprintf(stderr,"Error: cannot write %s\n",oname);
	 fprintf(stderr,"errno = %i\n",errno);
	 unlink (oname);
	 exit(1);
       }

    fprintf (stderr,"Successfully created %s.\n",oname);
    exit(0);
  }
