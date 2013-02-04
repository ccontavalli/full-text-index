/*
 *	pattern search in .fmi files 
 */

#include <getopt.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "utils/interface.h"

void print_usage(char *program_name);
void do_extract(void  *index, uint32_t  position, uint32_t nchars);
void do_display(void *index, uchar *pattern, uint32_t nchars);
void do_locate(void *index, uchar *pattern);
void do_count(void *index, uchar *pattern);
void do_unbuild(void *index, char * extfilename);
void output_char(uchar c);

/* macro to detect and notify errors  */
#define IFERROR(error) {{if (error) { fprintf(stderr, "%s\n", error_index(error)); exit(1); }}}

/* Fm-Index Search MAIN */
int main(int argc, char *argv[]) {

	char * program_name = argv[0];
	char * filename, * extfilename = NULL;
	void *index;
	uchar * pattern;
	uint32_t nchars, position = 0;
	int unbuild, count, locate, extract, display, error;

	nchars = 10;	
	count = locate = extract = unbuild = display = 0;
	
	int next_option;
	const char* short_options = "hlce:s:d:n:";
	const struct option long_options[] = {
		{"help" , 		0,	NULL, 	'h'},
		{"locate",		0,	NULL,	'l'},
		{"count",		0,	NULL,	'c'},
		{"extract",		1,	NULL,	'e'},
		{"display",		1,	NULL,	's'},
		{"unbuild",		1,	NULL,	'd'},
		{"numchars",	1,	NULL,	'n'},
		{NULL,			0,	NULL,	0}
	 };
 
 	if(argc<3) print_usage(program_name);
	
		do { 
		next_option = getopt_long(argc, argv, short_options,
								  long_options, NULL);
		
		switch (next_option) {
			    
			case 'l': /* report position */
         		locate = 1; 
				break;
			
			case 's': /* display len chars sourronding each occ */
         		display = 1;
			    nchars = (uint32_t) atol(optarg); 
				break;
			
			case 'd': /* unbuild */
         		unbuild = 1;
			    extfilename = (char *) optarg;
				break;
			
	  		case 'e': /* extract */
		 		extract = 1;
				position = (uint32_t) atol(optarg); 
    	 		break;
			
			case 'c': /* count */
		 		count = 1;
    	 		break;
			
	  		case 'n': /* numchars for extract function */
				extract = 1;
		 		nchars = (uint32_t) atol(optarg); 
	  	 		break;

			case '?': /* The user specified an invalid option. */
        		fprintf(stderr,"Unknown option.\n");
        		print_usage(program_name);
				break;	
			
			case -1: /* Done with options. */
				break;
			
			default: 
				print_usage(program_name);
    	}
	}
	while (next_option != -1);
	
	if (optind == argc)  {	
		  	fprintf(stderr,"You must supply a pattern and a .fmi filename\n\n");
			print_usage(program_name);
     		exit(1);
    }
	
	/* priorita' extract display locate count */
	if(!(extract || unbuild)) /* pattern */ 
		pattern = (uchar *) argv[optind++];
	else pattern = NULL;
		
	if (optind == argc)  {	
		  	fprintf(stderr,"You must supply a pattern and a .fmi filename\n\n");
			print_usage(program_name);
     		exit(1);
    }
			
	filename = argv[optind];
	
	error = load_index (filename, &index);
	IFERROR (error);

	if (unbuild==1) 
		do_unbuild(index, extfilename);
	if (extract==1) 
		do_extract(index, position, nchars);
	if (display==1) 
		do_display(index, pattern, nchars);
	if (locate==1) 
		do_locate(index, pattern);
	
	do_count(index, pattern);
	exit(0);
}	


void do_extract(void *index, uint32_t position, uint32_t nchars) {

	int error;
	uint32_t i, read;
	uchar *text;

	error = extract(index, position, position+nchars-1, &text, &read);
	IFERROR(error);

	fprintf(stdout, "Extracted from %lu to %lu\n\n",position, position+nchars-1);
	for(i=0; i< read; i++)
		output_char(text[i]);
	fprintf(stdout,"\n\n");
	free(text);
	
	error = free_index(index);
	IFERROR(error);
	exit(0);
}


void do_display(void *index, uchar *pattern, uint32_t nchars) {

	uint32_t numocc, i, j, *length_snippet, p, len;
	uchar *snippet_text;
	int error;

	len = strlen((char *)pattern) + 2*nchars;

	error=	display (index, pattern, strlen((char *)pattern), nchars, &numocc, 
				    	 &snippet_text, &length_snippet);
	IFERROR (error);

	fprintf (stdout, "Pattern: %s # Snippets: %lu\n\n\n", pattern, numocc);
	for (i = 0, p = 0; i < numocc; i++, p+=len) {
		fprintf (stdout, "Snippet length: %lu\nText: ", length_snippet[i]);
		for(j=0; j < length_snippet[i]; j++) 			
			output_char(snippet_text[p+j]);
		fprintf (stdout, "\n\n");
	}
			
	if (numocc) {	
		free (length_snippet);
		free (snippet_text);
		}

	error = free_index(index);
	IFERROR(error);
	exit(0);
}	


void do_locate(void *index, uchar *pattern) { 

	uint32_t *occs, numocc, i;
	int error;

	error = locate (index, pattern, strlen((char *)pattern), &occs, &numocc);
	IFERROR (error);
		
	fprintf(stdout,"Pattern: %s occs %lu\n\n", pattern, numocc);
		
	for (i = 0; i < numocc; i++)
			fprintf (stdout, "Position: %lu\n", occs[i]);
	
	if (numocc) free (occs);
	
	error = free_index(index);
	IFERROR(error);
	
	exit(0);
}	


void do_count(void *index, uchar *pattern) {
	
	uint32_t numocc;
	int error;
	
	error = count (index, pattern, strlen((char *)pattern), &numocc);
	IFERROR (error);
	fprintf(stdout,"Pattern: %s %lu occs\n", pattern, numocc);
	
	error = free_index(index);
	IFERROR(error);
	
	exit(0);
}


void do_unbuild(void *index, char * extfilename) {

	FILE *decfile;
	uchar *text;
	uint32_t length, text_len = 0;
	int error;

	decfile = fopen(extfilename, "wb");
	if (decfile == NULL)
	{
		fprintf (stderr, "Error: cannot open file %s\n", extfilename);
		perror ("fm-search");
		exit (1);
	}

	error = get_length(index, &text_len);
	IFERROR(error);

	error = extract(index, 0, text_len-1, &text, &length);
	IFERROR(error);
	
	fprintf(stdout,"Original text size = %lu Kbytes\n", text_len/1024);
	
	error = fwrite(text, sizeof(uchar), length, decfile);
	if ((error < 0) || (error < (int) length)) {
		fprintf (stderr, "Error: cannot write file %s\n", extfilename);
		perror ("fm-search");
		exit (1);
	}

	error = free_index(index);
	IFERROR(error);
	fclose(decfile);
	free(text);
	exit(0);
}


void print_usage(char * program_name) { 
	fprintf(stderr, "\n-----------------------------------------------------------------\n\n");
	fprintf(stderr, "                        FM-index Version 2\n");
	fprintf(stderr, "                          September 2005\n");
	fprintf(stderr, "          Authors: Paolo Ferragina and Rossano Venturini\n");
	fprintf(stderr, "      Dipartimento di Informatica, University of Pisa, Italy\n");
	fprintf(stderr, "\n-----------------------------------------------------------------\n\n");
	fprintf(stderr, "Search patterns over an FM-index file.\n\n");
    fprintf(stderr, "Usage:\t%s ", program_name);
    fprintf(stderr,"[-c] [-l] [-s numchars] [-e position] [-n numchars]\n");
	fprintf(stderr,"       \t\t  [-d dfilename] [pattern] fmifile\n");
    fprintf(stderr,"\tFlags:\n");
	fprintf(stderr,"\t-c            count number of occurrences of 'pattern'\n");
    fprintf(stderr,"\t-l            locate occurrences of 'pattern'\n");
    fprintf(stderr,"\t-s numchars   display 'numchars' chars sourrounding each occ of 'pattern'\n");
    fprintf(stderr,"\t-d dfilename  extract the whole indexed file and save on file 'dfilename'\n");
	fprintf(stderr,"\t-e pos        extract a text substring from pos to pos+numchars-1\n");
	fprintf(stderr,"\t-n numchars   length of the extracted substring (default 10) (use with -e)\n\n");
	fprintf(stderr,"NOTE: 'fmifile' must be indicated without extension '.fmi'\n\n");
	exit(1);
}


/*
   print a char to stdout taking care of newlines, tab etc.
   To be completed!!!
*/
void output_char(uchar c)	{      
	
  switch(c) {
	  
    case '\n': fprintf(stdout, "[\\n]"); break;
    
	case '\r': fprintf(stdout, "[\\r]"); break;
	
	case '\t': fprintf(stdout, "[\\t]"); break;
	
	case '\a': fprintf(stdout, "[\\a]"); break;
	
	case '\0': fprintf(stdout, "[\\0]"); break;
    
	default: fprintf(stdout, "%c", c);
	
  }
}
