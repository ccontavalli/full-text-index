// gcc 2index1fileExample.c -o 2index1fileExample  fm_index.a ds_ssort.a to compile

#include <stdio.h>
#include <string.h>

#include "utils/interface.h"

/* macro to detect and notify errors  */
#define IFERROR(error) {{if (error) { fprintf(stderr, "%s\n", error_index(error)); exit(1); }}}

int read_file(char *filename, uchar **textt, uint32_t *length);

int main(int argc, char *argv[]) {

	char *infile1, *infile2;
    uchar *text1;
	uchar *text2;
	uint32_t text_len1;
	uint32_t text_len2;
	uint32_t index_size1;
	uint32_t index_size2;
	void *index1;
	void *index2;
	int error;

    if (argc < 2) printf("%s file1 file2\n", argv[0]);

	infile1 = argv[1];  // input file 
	infile2 = argv[2]; // output file

	// First index
	error = read_file(infile1, &text1, &text_len1);
	IFERROR(error);
	error = build_index(text1, text_len1, "-a 2", &index1);
	IFERROR(error);
	free(text1); text1 = NULL;
	
	// Second index
	error = read_file(infile2, &text2, &text_len2);
	IFERROR(error);
	error = build_index(text2, text_len2, "-a 2", &index2);
	IFERROR(error);
	free(text2); text2 = NULL;
	
	// Size
	index_size(index1, &index_size1);
	index_size(index2, &index_size2);
	printf("index size %lu %lu\n", index_size1, index_size2);
	
	uint32_t toc_size = 2*sizeof(uint32_t);
	uchar * compress = (uchar *) malloc(sizeof(uchar)*(index_size1+index_size2+toc_size));
	
	// save TOC. the 2 index_size
	memcpy(compress, &index_size1, sizeof(uint32_t));
	memcpy(compress+sizeof(uint32_t), &index_size2, sizeof(uint32_t));
	
	error = save_index_mem(index1, compress+toc_size); 
	IFERROR(error);
	
	error = save_index_mem(index2, compress+toc_size+index_size1); 
	IFERROR(error);
	
	free_index(index1); 
	free_index(index2); 
	index_size1 = index_size2 = text_len1 = text_len2 = 0;
	
	// Extract the indexes
	
	//Read sizes
	memcpy(&index_size1, compress, sizeof(uint32_t));
	memcpy(&index_size2, compress+sizeof(uint32_t),sizeof(uint32_t));
	printf("index size %lu %lu\n", index_size1, index_size2);
	
	error = load_index_mem(&index1, compress+toc_size, index_size1); 
	IFERROR(error);
	
	error = load_index_mem(&index2, compress+toc_size+index_size1, index_size2); 
	IFERROR(error);

	error = get_length(index1, &text_len1);
	IFERROR(error);
	
	error = get_length(index2, &text_len2);
	IFERROR(error);
	
	// extract first text 
	uint32_t datalen;
	error = extract(index1, 0, text_len1-1, &text1, &datalen);	
	IFERROR(error);
	error = free_index(index1);
	IFERROR(error);
	
	// extract second text 
	error = extract(index2, 0, text_len2-1, &text2, &datalen);	
	IFERROR(error);
	error = free_index(index2);
	IFERROR(error);
	
	//print the texts
	printf("FILE %s\n", infile1);
	//fwrite(text1, sizeof(uchar), text_len1, stdout);
	printf("\n\n\nFILE %s\n", infile2);
	//fwrite(text2, sizeof(uchar), text_len2, stdout);
	
	free(text1); free(text2);
	free(compress);
}


/* 
  Opens and reads a text file. 
  This procedure allocates the overshoot space necessary to the ds_ssort library.
*/
int read_file(char *filename, uchar **textt, uint32_t *length) {

  uchar *text;
  uint32_t t;
  FILE *infile;
  
  infile = fopen(filename, "rb"); // b is for binary: required by DOS
  if(infile == NULL) {
	  	fprintf (stderr, "Error: cannot open file %s for reading\n", filename);
	  	exit(1);
  }
  
  /* store input file length */
  if(fseek(infile,0,SEEK_END) !=0 ) {
	  	fprintf (stderr, "Error: cannot read file %s for reading\n", filename);
	  	exit(1);
  }
  *length = ftell(infile);
  
  text = malloc((*length)*sizeof(*text));
  if(text == NULL) 	{
		fprintf (stderr, "Error: cannot allocate %lu bytes\n", (*length));
	    exit(1);
  }
  
  /* read text in one sweep */
  rewind(infile);
  t = fread(text, sizeof(*text), (size_t) *length, infile);
  if(t!=*length) {
	  	fprintf (stderr, "Error: cannot read file %s for writing\n", filename);
	  	exit(1);
  }
  *textt = text;
  fclose(infile);
  return 0;
}
