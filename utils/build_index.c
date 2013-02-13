#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "interface.h"

/* only for getTime() */
#include <sys/time.h>
#include <sys/resource.h>
 
/* macro to detect and notify errors  */
#define IFERROR(error) {{if (error) { fprintf(stderr, "%s\n", error_index(error)); exit(1); }}}

int read_file(char *filename, uchar **textt, ulong *length);
void print_usage(char *);
double getTime(void);

int main(int argc, char *argv[]) {

	char *infile, *outfile;
    uchar *text;
	char *params = NULL;
	ulong text_len;
	void *index;
	int error, i;
	double start, end;

    if (argc < 3) print_usage(argv[0]);
	if (argc > 3) { 
		int nchars, len;
		nchars = argc-3;
		for(i=2;i<argc;i++)
			nchars += strlen(argv[i]);
		params = (char *) malloc((nchars+1)*sizeof(char));
		params[nchars] = '\0';
		nchars = 0;
		for(i=3;i<argc;i++) {
			len = strlen(argv[i]);
			strncpy(params+nchars,argv[i],len);
			params[nchars+len] = ' ';
			nchars += len+1;
		}
		params[nchars-1] = '\0';
	}

	infile = argv[1];
	outfile = argv[2];

	start = getTime();
	error = read_file(infile, &text, &text_len);
	IFERROR(error);

	error = build_index(text, text_len, params, &index);
	IFERROR(error);

	error = save_index(index, outfile);
	IFERROR(error);
	end = getTime();	

	fprintf(stderr, "Building time: %.3f secs\n", end-start );
	
	ulong index_len;
	index_size(index, &index_len);
	fprintf(stdout,"Input: %lu bytes --> Output %lu bytes.\n", text_len, index_len);
	fprintf(stdout,"Overall compression --> %.2f%% (%.2f bits per char).\n\n",
     			(100.0*index_len)/text_len, (index_len*8.0)/text_len);

	error = free_index(index);
	IFERROR(error);


	exit(0);
}

/* 
  Opens and reads a text file 
*/
int read_file(char *filename, uchar **textt, ulong *length) {

  uchar *text;
  unsigned long t; 
  FILE *infile;
  
  infile = fopen(filename, "rb"); // b is for binary: required by DOS
  if(infile == NULL) return 1;
  
  /* store input file length */
  if(fseek(infile,0,SEEK_END) !=0 ) return 1;
  *length = ftell(infile);
  
  /* alloc memory for text (the overshoot is for suffix sorting) */
  text = (uchar *) malloc((*length)*sizeof(*text)); 
  if(text == NULL) return 1;  
  
  /* read text in one sweep */
  rewind(infile);
  t = fread(text, sizeof(*text), (size_t) *length, infile);
  if(t!=*length) return 1;
  *textt = text;
  fclose(infile);
  
  return 0;
}

double
getTime (void)
{

	double usertime, systime;
	struct rusage usage;

	getrusage (RUSAGE_SELF, &usage);

	usertime = (double) usage.ru_utime.tv_sec +
		(double) usage.ru_utime.tv_usec / 1000000.0;

	systime = (double) usage.ru_stime.tv_sec +
		(double) usage.ru_stime.tv_usec / 1000000.0;

	return (usertime + systime);

}

void print_usage(char * progname) {
	fprintf(stderr, "Usage: %s <source file> <index file> [<parameters>]\n", progname);
	fprintf(stderr, "\nIt builds the index for the text in file <source file>,\n");
	fprintf(stderr, "storing it in <index file>. Any additional <parameters> \n");
	fprintf(stderr, "will be passed to the construction function.\n");
	fprintf(stderr, "At the end, the program sends to the standard error \n");
	fprintf(stderr, "performance measures on time to build the index.\n\n");
	exit(1);
}
