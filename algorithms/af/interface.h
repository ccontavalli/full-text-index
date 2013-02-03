#ifndef DATATYPE 
typedef unsigned char uchar;
#ifndef __USE_MISC
typedef unsigned long ulong;
#endif

#define DATATYPE 1
#endif
#include <stdlib.h>

/* 
 * Loads index from file(s) called filename[.<ext>] 
 */
int load_index(char *filename, void **index);


/*
 * Frees the memory occupied by index.
 */
int free_index(void *index);


/*  
 * Writes in numocc the number of occurrences of pattern[0..length-1] in 
 * index. 
 */
int count(void *index, uchar *pattern, ulong length, ulong *numocc);


/*  
 * Writes in numocc the number of occurrences of pattern[0..length-1] in 
 * index. It also allocates occ (which must be freed by the caller) and 
 * writes the locations of the numocc occurrences in occ, in arbitrary 
 * order.
 */
int locate(void *index, uchar *pattern, ulong length, ulong **occ, 
		   ulong *numocc);

 
/*	
 * Allocates snippet (which must be freed by the caller) and writes 
 * text[from..to] on it. Returns in snippet_length the length of the 
 * snippet actually extracted (that could be less than to-from+1 if to is 
 * larger than the text size). 
 */
int extract(void *index, ulong from, ulong to, uchar **snippet,
			ulong *snippet_length);

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
			ulong *numocc, 	uchar **snippet_text, ulong **snippet_len); 


/* 
 * Obtains the length of the text represented by index. 
 */
int get_length(void *index, ulong *length);


/*
 * Writes in size the size, in bytes, of index. This should be the size
 * needed by the index to perform any of the operations it implements.
 */
int index_size(void *index, ulong *size);

int index_size_count(void *index, ulong *size);

/*
 * The integer returned by each procedure indicates an error code if
 * different of zero. In this case, the error message can be displayed 
 * by calling error_index(). We recall that text and pattern indexes start
 * at zero.
 */
char* error_index(int e);


/* 
 * Creates index from text[0..length-1]. Note that the index is an 
 * opaque data type. Any build option must be passed in string 
 * build_options, whose syntax depends on the index. The index must 
 * always work with some default parameters if build_options is NULL. 
 * The returned index is ready to be queried. 
 */
int build_index(uchar *text, ulong length, char *build_options, void **index);

/*
 * Saves index on disk by using single or multiple files, having 
 * proper extensions. 
 */
int save_index(void *index, char *filename);

/* 
 * Save index on memory 
 */
int save_index_mem(void *index, uchar *compress);

/* 
 * Load index from memory 
 */
int load_index_mem (void ** index, uchar *compress, ulong size);
