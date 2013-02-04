/* This program doesn't use the Pizza&Chili Api */

#include "fm_index.h"
#include "fm_build.h"
#include <unistd.h>
#include "utils/interface.h"

void print_usage();

/* macro to detect and to notify errors  */
#define IFERROR(error) {{if (error) { fprintf(stderr, "%s\n", error_index(error)); exit(1); }}}

static char *programname;

int main(int argc, char *argv[]) {

	char *infile = NULL, *outfile = NULL;
    uchar *text;
	uint32_t text_len, index_len;
	uint32_t bsl1 = 16;
	uint32_t bsl2 = 512;
	uint32_t freq = 64;
	suint tc = MULTIH;
	int error;
	fm_index *index = malloc(sizeof(fm_index));
	if(index==NULL) return -1;
		
	programname = argv[0];

	int next_option;
	const char *short_options = "B:b:o:f:h";
	
	if(argc<2) print_usage();
		
	do { 
		next_option = getopt(argc, argv, short_options);
		
		switch (next_option) {
			
			case 'o': /* output file name */
         		outfile = optarg; 
				break;
			
	   		case 'B': /* Superbucket size */
        		bsl1 = atoi(optarg); 
				break;
			
			case 'b': /* Bucket size */
        		bsl2 = atoi(optarg); 
				break;
			
      		case 'f': /* frequency */
        		freq=atoi(optarg); 
				break;
			
			case 'h': /* Decompression */
        		print_usage(); 
				break;
			
      		case '?': /* The user specified an invalid option. */
        		fprintf(stderr,"Unknown option.\n");
        		print_usage();
				break;
			
			case -1: /* Done with options. */
				break;
			
			default: 
				print_usage();
    	}
	}
	while (next_option != -1);
		
	if(optind<argc)
    	 infile = argv[optind];
	
	if(outfile == NULL) outfile = infile;
		
	/* Some check */ 
	if ((bsl2<=0)|| bsl1 <=0) {
		fprintf(stderr,"The size of bucketa and superbuckets must be greater than 0\n");  
		exit(1);
	}

	
	if (tc!=MULTIH) {
     	fprintf(stderr,"Available compression algorithms:\n\t"); 
     	fprintf(stderr,"4=MultiTableHuffman\n");  
     	exit(1);
  	}

	error = fm_read_file(infile, &text, &text_len);
	IFERROR(error);

	error = fm_build_config (index, tc, freq, bsl1, bsl2, 1);
	IFERROR(error);

	error =  fm_build(index, text, text_len);
	IFERROR(error);

	error = save_index(index, outfile);
	IFERROR(error);
		
	index_size(index, &index_len);

	/* How to use int save_index_mem(void *indexe, uchar *compress) */
	#if 0
	uchar *foo = malloc(index_len * sizeof(uchar));
	if(!foo) {
		fprintf(stderr,"Alloc Failed\n");
		exit(1);
		}
	error = save_index_mem(index, foo);
	IFERROR(error);
	#endif


	fprintf(stdout,"\n  Input: %lu bytes --> Output %lu bytes.\n", text_len, index_len);
	fprintf(stdout,"  Overall compression --> %.2f%% (%.2f bits per char).\n\n",
     			(100.0*index_len)/text_len, (index_len*8.0)/text_len);

	error = free_index(index);
	IFERROR(error);

	exit(0);
}

void print_usage() {
	fprintf(stderr, "\n-----------------------------------------------------------------\n\n");
	fprintf(stderr, "                        FM-index Version 2\n");
	fprintf(stderr, "                          September 2005\n");
	fprintf(stderr, "          Authors: Paolo Ferragina and Rossano Venturini\n");
	fprintf(stderr, "      Dipartimento di Informatica, University of Pisa, Italy\n");
	fprintf(stderr, "\n-----------------------------------------------------------------\n\n");
	fprintf(stderr, "Build the FM-index of the input file.\n\n");
	fprintf(stderr, "Usage: \t%s [-B Bsize] [-b bsize] [-f freq] \n", programname);
    fprintf(stderr,"\t\t   [-o outfile] infilename\n");
	fprintf(stderr,"\tFlags:\n");
    fprintf(stderr,"\t-B Bsize    size in Kb of level 1 buckets ");
    fprintf(stderr,              "(default %d)\n", 16);
    fprintf(stderr,"\t-b bsize    size in byte of level 2 buckets ");
    fprintf(stderr,              "(default %d)\n", 512);
	fprintf(stderr,"\t            bsize must divide Bsize*1024\n");
	fprintf(stderr,"\t-f freq     frequency of marked char [integer] ");
    fprintf(stderr,              "(default %d)\n", 64); 
    fprintf(stderr,"\t-o outfile  output file name to store the FM-index.\n");
	fprintf(stderr,"\t            'outfile' must be indicated without\n");
	fprintf(stderr,"\t            extension '.fmi'\n\n"); 
    //fprintf(stderr,"\t-t 4	      compression type: \n");
    //fprintf(stderr,"\t            4=MHuf (default %i)\n\n", MULTIH);
    fprintf(stderr,"If no output file name is given default is ");
    fprintf(stderr,"infilename[.fmi].\n\n");
    exit(0);
 }
