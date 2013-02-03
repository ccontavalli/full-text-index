/*
 * Run Queries
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "interface.h"

/* only for getTime() */
#include <sys/time.h>
#include <sys/resource.h>

#define COUNT 		('C')
#define LOCATE 		('L')
#define EXTRACT 	('E')
#define DISPLAY 	('D')
#define VERBOSE 	('V')

/* macro to detect and to notify errors */
#define IFERROR(error) {{if (error) { fprintf(stderr, "%s\n", error_index(error)); exit(1); }}}


/* local headers */
void do_locate (void);
void do_count (void);
void do_extract (void);
void do_display(ulong length);
void pfile_info (ulong *length, ulong *numpatt);
void output_char(uchar c, FILE * where);
double getTime (void);
void usage(char * progname);
  		
static void *Index;	 /* opauque data type */
static int Verbose = 0; 
static ulong Index_size, Text_length;
static double Load_time;

/*
 * Temporary usage: run_queries <index file> <type> [length] [V]
 */
int main (int argc, char *argv[])
{
	int error = 0;
	char *filename;
	char querytype;
	
	if (argc < 3)	{
		usage(argv[0]);	
		exit (1);
	}
	
	filename = argv[1];
	querytype = *argv[2];
	
	Load_time = getTime ();
	error = load_index (filename, &Index);
	IFERROR (error);
	Load_time = getTime () - Load_time;
	fprintf (stderr, "Load index time = %.2f secs\n", Load_time);

	error = index_size(Index, &Index_size);
	IFERROR (error);
	error = get_length(Index, &Text_length);
	IFERROR (error);
	Index_size /=1024;
	fprintf(stderr, "Index size = %lu Kb\n", Index_size);

	switch (querytype){
		case COUNT:
			if (argc > 3) 
				if (*argv[3] == VERBOSE) {
					Verbose = 1;
					fprintf(stdout,"%c", COUNT);
				}				
			do_count();
			break;
		case LOCATE:
			if (argc > 3) 
				if (*argv[3] == VERBOSE) {
					Verbose = 1;
					fprintf(stdout,"%c", LOCATE);
				}
			do_locate();
			break;	
		case EXTRACT:
			if (argc > 3) 
				if (*argv[3] == VERBOSE) {
						Verbose = 1;
						fprintf(stdout,"%c", EXTRACT);
				}

			do_extract();
			break;
		case DISPLAY:
			if (argc < 4) {
				usage(argv[0]);	
				exit (1);
			}
			if (argc > 4) 
				if (*argv[4] == VERBOSE){
						Verbose = 1;
						fprintf(stdout,"%c", DISPLAY);

				}
			do_display((ulong) atol(argv[3]));
			break;
		default:
			fprintf (stderr, "Unknow option: main ru\n");
			exit (1);
	}

	error = free_index(Index);
	IFERROR(error);

	return 0;
}


void
do_count ()
{
	int error = 0;
	ulong numocc, length, tot_numocc = 0, numpatt, res_patt;
	double time, tot_time = 0;
	uchar *pattern;

	pfile_info (&length, &numpatt);
	res_patt = numpatt;

	pattern = malloc (sizeof (uchar) * (length));
	if (pattern == NULL)
	{
		fprintf (stderr, "Error: cannot allocate\n");
		exit (1);
	}

	while (res_patt)
	{
	
		if (fread (pattern, sizeof (*pattern), length, stdin) != length)
		{
			fprintf (stderr, "Error: cannot read patterns file\n");
			perror ("run_queries");
			exit (1);
		}

		/* Count */
		time = getTime ();
		error = count (Index, pattern, length, &numocc);
		IFERROR (error);
	
		if (Verbose) {
			fwrite(&length, sizeof(length), 1, stdout);
			fwrite(pattern, sizeof(*pattern), length, stdout);
			fwrite(&numocc, sizeof(numocc), 1, stdout);
			}
		tot_time += (getTime () - time);
		tot_numocc += numocc;
		res_patt--;
	}

	fprintf (stderr, "Total Num occs found = %lu\n", tot_numocc);
	fprintf (stderr, "Count time = %.4f msecs\n", tot_time*1000);
	fprintf (stderr, "Count_time/Pattern_chars = %.4f msecs/chars\n",
		 (tot_time * 1000) / (length * numpatt));
	fprintf (stderr, "Count_time/Num_patterns = %.4f msecs/patterns\n\n",
		 (tot_time * 1000) / numpatt);
	fprintf (stderr, "(Load_time+Count_time)/Pattern_chars = %.4f msecs/chars\n",
		 ((Load_time+tot_time) * 1000) / (length * numpatt));
	fprintf (stderr, "(Load_time+Count_time)/Num_patterns = %.4f msecs/patterns\n\n",
		 ((Load_time+tot_time) * 1000) / numpatt);	

	free (pattern);
}


void
do_locate ()
{
	int error = 0;
	ulong numocc, length, *occ, tot_numocc = 0, numpatt;
	double time, tot_time = 0;
	uchar *pattern;

	pfile_info (&length, &numpatt);

	pattern = malloc (sizeof (uchar) * (length));
	if (pattern == NULL)
	{
		fprintf (stderr, "Error: cannot allocate\n");
		exit (1);
	}

	while (numpatt) {
	
		if (fread (pattern, sizeof (*pattern), length, stdin) != length)
		{
			fprintf (stderr, "Error: cannot read patterns file\n");
			perror ("run_queries");
			exit (1);
		}

		/* Locate */
		time = getTime ();
		error =	locate (Index, pattern, length, &occ, &numocc);
		IFERROR (error);
		tot_time += (getTime () - time);
		
		tot_numocc += numocc;
		numpatt--;
		
		if (Verbose) {
			fwrite(&length, sizeof(length), 1, stdout);
			fwrite(pattern, sizeof(*pattern), length, stdout);
			fwrite(&numocc, sizeof(numocc), 1, stdout);
			fwrite(occ, sizeof(*occ), numocc, stdout);
			}
			
		if (numocc) free (occ);
	}

	fprintf (stderr, "Total Num occs found = %lu\n", tot_numocc);
	fprintf (stderr, "Locate time = %.2f secs\n", tot_time);
	fprintf (stderr, "Locate_time/Num_occs = %.4f msec/occss\n\n", (tot_time * 1000) / tot_numocc);
	fprintf (stderr, "(Load_time+Locate_time)/Num_occs = %.4f msecs/occs\n\n", ((tot_time+Load_time) * 1000) / tot_numocc);

	free (pattern);
}


void do_display(ulong numc) {
	
	int error = 0;
	ulong numocc, length, i, *snippet_len, tot_numcharext = 0, numpatt;
	double time, tot_time = 0;
	uchar *pattern, *snippet_text;

	pfile_info (&length, &numpatt);

	pattern = malloc (sizeof (uchar) * (length));
	if (pattern == NULL)
	{
		fprintf (stderr, "Error: cannot allocate\n");
		exit (1);
	}

	fprintf(stderr, "Snippet length %lu\n", numc);

	while (numpatt)
	{

		if (fread (pattern, sizeof (*pattern), length, stdin) != length)
		{
			fprintf (stderr, "Error: cannot read patterns file\n");
			perror ("run_queries");
			exit (1);
		}

		/* Display */
		time = getTime ();
		error =	display (Index, pattern, length, numc, &numocc, 
				    	 &snippet_text, &snippet_len);
		IFERROR (error);
		tot_time += (getTime () - time);
		
		if (Verbose) {
			ulong j, len = length + 2*numc;
		    char blank = '\0';
			fwrite(&length, sizeof(length), 1, stdout);
			fwrite(pattern, sizeof(*pattern), length, stdout);
			fwrite(&numocc, sizeof(numocc), 1, stdout);
			fwrite(&len, sizeof(len), 1, stdout);
		
			for (i = 0; i < numocc; i++){
				fwrite(snippet_text+len*i,sizeof(uchar),snippet_len[i],stdout);
				for(j=snippet_len[i];j<len;j++)
				   fwrite(&blank,sizeof(uchar),1,stdout);
			}

		}
		numpatt--;
		
		for(i=0; i<numocc; i++) {
			tot_numcharext += snippet_len[i];
		}
			
		if (numocc) {
			free (snippet_len);
			free (snippet_text);
		}
	}

	fprintf (stderr, "Total num chars extracted = %lu\n", tot_numcharext);
	fprintf (stderr, "Display time = %.2f secs\n", tot_time);
	fprintf (stderr, "Time_display/Tot_num_chars = %.4f msecs/chars\n\n", (tot_time*1000) / tot_numcharext);
	fprintf (stderr, "(Load_time+Time_display)/Tot_num_chars = %.4f msecs/chars\n\n", ((Load_time+tot_time)*1000) / tot_numcharext);

	free (pattern);
}


/* Open patterns file and read header */
void
pfile_info (ulong * length, ulong * numpatt)
{
	int error;
	uchar c;
	uchar origfilename[257];

	error = fscanf (stdin, "# number=%lu length=%lu file=%s forbidden=", numpatt,
					length, origfilename);
	if (error != 3)
	{
		fprintf (stderr, "Error: Patterns file header not correct\n");
		perror ("run_queries");
		exit (1);
	}

	fprintf (stderr, "# patterns = %lu length = %lu file = %s forbidden chars = ", *numpatt, *length, origfilename);

	while ( (c = fgetc(stdin)) != 0) {
		if (c == '\n') break;
		fprintf (stderr, "%d",c);
	}

	fprintf(stderr, "\n");

}


void
do_extract()
{
	int error = 0;
	uchar *text, orig_file[257];
	ulong num_pos, from, to, numchars, readen, tot_ext = 0;
	double time, tot_time = 0;

	error = fscanf(stdin, "# number=%lu length=%lu file=%s\n", &num_pos, &numchars, orig_file);
	if (error != 3)
	{
		fprintf (stderr, "Error: Intervals file header is not correct\n");
		perror ("run_queries");
		exit (1);
	}
	fprintf(stderr, "# number=%lu length=%lu file=%s\n", num_pos, numchars, orig_file);

	while(num_pos) {
	
		if(fscanf(stdin,"%lu,%lu\n", &from, &to) != 2) {
			fprintf(stderr, "Cannot read correctly intervals file\n");
			exit(1);
		}

		time = getTime();
		error = extract(Index, from, to, &text, &readen);
		IFERROR(error);
		tot_time += (getTime() - time);
	
		tot_ext += readen;
		
		if (Verbose) {
			fwrite(&from,sizeof(ulong),1,stdout);
			fwrite(&readen,sizeof(ulong),1,stdout);
			fwrite(text,sizeof(uchar),readen, stdout);
		}

		num_pos--;
		free(text);
	}
	
	fprintf (stderr, "Total num chars extracted = %lu\n", tot_ext);
	fprintf (stderr, "Extract time = %.2f secs\n", tot_time);
	fprintf (stderr, "Extract_time/Num_chars_extracted = %.4f msecs/chars\n\n",
		 (tot_time * 1000) / tot_ext);
	fprintf (stderr, "(Load_time+Extract_time)/Num_chars_extracted = %.4f msecs/chars\n\n",
		 ((Load_time+tot_time) * 1000) / tot_ext);
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

void usage(char * progname) {	
	fprintf(stderr, "\nThe program loads <index> and then executes over it the\n");
	fprintf(stderr, "queries it receives from the standard input. The standard\n");
	fprintf(stderr, "input comes in the format of the files written by \n");
	fprintf(stderr, "genpatterns or genintervals.\n");
	fprintf(stderr, "%s reports on the standard error time statistics\n", progname);
	fprintf(stderr, "regarding to running the queries.\n\n");
	fprintf(stderr, "Usage:  %s <index> <type> [length] [V]\n", progname);
	fprintf(stderr, "\n\t<type>   denotes the type of queries:\n");
	fprintf(stderr, "\t         %c counting queries;\n", COUNT);
	fprintf(stderr, "\t         %c locating queries;\n", LOCATE);
	fprintf(stderr, "\t         %c displaying queries;\n", DISPLAY);
	fprintf(stderr, "\t         %c extracting queries.\n\n", EXTRACT);
	fprintf(stderr, "\n\t[length] must be provided in case of displaying queries (D)\n");
	fprintf(stderr, "\t         and denotes the number of characters to display\n");
	fprintf(stderr, "\t         before and after each pattern occurrence.\n");
	fprintf(stderr, "\n\t[V]      with this options it reports on the standard output\n");
	fprintf(stderr, "\t         the results of the queries. The results file should be\n");
	fprintf(stderr, "\t         compared with trusted one by compare program.\n\n");
}
