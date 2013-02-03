/*
 * Read/Write on index Functions Rossano Venturini 
 */

#include <string.h>
#include "fm_index.h"
#include "fm_occurences.h"
#include "fm_mng_bits.h"	/* Function to manage bits */

static int open_file (char * filename, uchar ** file, uint32_t * size);
static int fm_read_basic_prologue (fm_index * s);

/*
 * Loads index from file called filename.fmi MODIFIED: index that is
 * allocated fmindex.compress fmindex.compress_size
 * 
 * with read_basic_prologue() fmindex.type_compression fmindex.text_size
 * fmindex.bwt_eof_pos fmindex.bucket_size_lev1 fmindex.bucket_size_lev2
 * fmindex.num_bucs_lev1 fmindex.alpha_size fmindex.specialchar
 * fmindex.skip fmindex.start_prologue_occ fmindex.start_prologue_info_sb
 * fmindex.start_prologue_info_b fmindex.subchar fmindex.bool_char_map[]
 * fmindex.char_map[] fmindex.inv_char_map[] fmindex.bits_x_occ
 * 
 * 
 * REQUIRES: filename: index file void *index point to NULL 
 */

int
load_index (char * filename, void ** index)
{
	
	int error;
	fm_index *fmindex;

	fmindex = (fm_index *) malloc (sizeof (fm_index));
	if (fmindex == NULL)
		return FM_OUTMEM;
	fmindex->compress_owner = 1;
	fmindex->owner = 0;
	
	/*
	 * Load index file 
	 */
	error =
		open_file (filename, &(fmindex->compress),
			   &(fmindex->compress_size));
	if (error)
		return error;

	error = fm_read_basic_prologue (fmindex);
	if (error)
		return error;

	if(fmindex->smalltext) {
		fmindex->skip = 0;
		if(fmindex->text_size<SMALLSMALLFILESIZE) {
			fmindex->text = fmindex->compress+4;
			*index = fmindex;
			return FM_OK;
		}
		fmindex->owner = 1;
		fmindex->smalltext = 2;
		error = fm_bwt_uncompress(fmindex);
		if (error < 0) return error;
		*index = fmindex;
		return FM_OK;
	}
	
	/*
	 * init some var 
	 */
	fmindex->int_dec_bits =
		int_log2 (int_log2
			  (fmindex->bucket_size_lev1 -
			   fmindex->bucket_size_lev2));
	
	
	if(fmindex->skip >1) {
	fmindex->occcharinf = fmindex->bwt_occ[fmindex->specialchar];
	if(fmindex->specialchar==fmindex->alpha_size-1)
		fmindex->occcharsup = fmindex->text_size-1;
	else 
		fmindex->occcharsup = fmindex->bwt_occ[fmindex->specialchar+1];
	
	fmindex->num_marked_rows = fmindex->occcharsup-fmindex->occcharinf;
	} else  fmindex->num_marked_rows = 0;
	
	fmindex->log2_row = int_log2(fmindex->text_size);
	
	fmindex->mtf_seq =
		(uchar *) malloc (fmindex->bucket_size_lev2 * sizeof (uchar));
	if (fmindex->mtf_seq == NULL)
		return FM_OUTMEM;
	
	fmindex->var_byte_rappr = ((fmindex->log2textsize + 7) / 8)*8;
	
	*index = fmindex;
	return FM_OK;
}

int fm_use_index (fm_index *fmindex) {
	int error = fm_read_basic_prologue (fmindex);
	if (error)
		return error;

	if(fmindex->smalltext) {
		fmindex->skip = 0;
		if(fmindex->text_size<SMALLSMALLFILESIZE)
			return FM_OK;
		fmindex->smalltext = 2;
		return FM_OK;
	}
	
	/*
	 * init some var 
	 */
	fmindex->int_dec_bits =
		int_log2 (int_log2
			  (fmindex->bucket_size_lev1 -
			   fmindex->bucket_size_lev2));
	
	
	if(fmindex->skip >1) {
	fmindex->occcharinf = fmindex->bwt_occ[fmindex->specialchar];
	if(fmindex->specialchar==fmindex->alpha_size-1)
		fmindex->occcharsup = fmindex->text_size-1;
	else 
		fmindex->occcharsup = fmindex->bwt_occ[fmindex->specialchar+1];
	
	fmindex->num_marked_rows = fmindex->occcharsup-fmindex->occcharinf;
	} else  fmindex->num_marked_rows = 0;

	fmindex->log2_row = int_log2(fmindex->text_size);
	fmindex->var_byte_rappr = ((fmindex->log2textsize + 7) / 8)*8;
	
	return FM_OK;

}

int
load_index_mem (void ** index, uchar *compress, uint32_t size)
{

	int error;
	fm_index *fmindex;

	fmindex = (fm_index *) malloc (sizeof (fm_index));
	if (fmindex == NULL)
		return FM_OUTMEM;

	fmindex->compress = compress;
	fmindex->compress_size = size;
	fmindex->compress_owner = 0;
	fmindex->text = NULL;
	fmindex->lf = NULL;
	fmindex->bwt = NULL;
	
	error = fm_read_basic_prologue (fmindex);
	if (error)
		return error;

	if(fmindex->smalltext) {
		fmindex->skip = 0;
		if(fmindex->text_size<SMALLSMALLFILESIZE) {
			fmindex->text = fmindex->compress+4;
			*index = fmindex;
			return FM_OK;
		}
		fmindex->smalltext = 2;
		error = fm_bwt_uncompress(fmindex);
		if (error < 0) return error;
		*index = fmindex;
		return FM_OK;
	}
	
	/*
	 * init some var 
	 */
	fmindex->int_dec_bits =
		int_log2 (int_log2
			  (fmindex->bucket_size_lev1 -
			   fmindex->bucket_size_lev2));
	
	
	if(fmindex->skip >1) {
	fmindex->occcharinf = fmindex->bwt_occ[fmindex->specialchar];
	if(fmindex->specialchar==fmindex->alpha_size-1)
		fmindex->occcharsup = fmindex->text_size-1;
	else 
		fmindex->occcharsup = fmindex->bwt_occ[fmindex->specialchar+1];
	
	fmindex->num_marked_rows = fmindex->occcharsup-fmindex->occcharinf;
	} else  fmindex->num_marked_rows = 0;
	
	fmindex->log2_row = int_log2(fmindex->text_size);
	fmindex->mtf_seq =
		(uchar *) malloc (fmindex->bucket_size_lev2 * sizeof (uchar));
	if (fmindex->mtf_seq == NULL)
		return FM_OUTMEM;
	
	fmindex->var_byte_rappr = ((fmindex->log2textsize + 7) / 8)*8;
	
	*index = fmindex;
	return FM_OK;
}

/*
 * Open and Read .fmi file (whitout mmap()) 
 */
static int
open_file (char * filename, uchar ** file, uint32_t * size)
{

	char *outfilename;
	FILE *outfile = NULL;

	outfilename =
		(char *) malloc ((strlen (filename) + strlen (EXT) + 1) *
				 sizeof (char));
	if (outfilename == NULL)
		return FM_OUTMEM;

	outfilename = strcpy (outfilename, filename);
	outfilename = strcat (outfilename, EXT);

	outfile = fopen (outfilename, "rb");	// b is for binary: required by
	// DOS
	if (outfile == NULL)
		return FM_READERR;

	/*
	 * store input file length 
	 */
	if (fseek (outfile, 0, SEEK_END) != 0)
		return FM_READERR;
	*size = ftell (outfile);

	if (*size < 1)
		return FM_READERR;
	rewind (outfile);

	/*
	 * alloc memory for text 
	 */
	*file = malloc ((*size) * sizeof (uchar));
	if ((*file) == NULL)
		return FM_OUTMEM;

	uint32_t t =
		(uint32_t) fread (*file, sizeof (uchar), (size_t) * size,
			       outfile);
	if (t != *size)
		return FM_READERR;

	fclose (outfile);
	free (outfilename);
	return FM_OK;
}


/*
 * read basic prologue from compress 
 */
static int
fm_read_basic_prologue (fm_index * s)
{
	int i;
	uint32_t size;

	fm_init_bit_reader (s->compress);
	s->text_size = fm_uint_read ();
	if(s->text_size< SMALLFILESIZE){
			s->smalltext=1; 
			return FM_OK;
		}
	s->smalltext = 0;
	s->type_compression = fm_bit_read (8);
	s->log2textsize = int_log2 (s->text_size - 1);
	s->bwt_eof_pos = fm_uint_read ();
	if (s->bwt_eof_pos > s->text_size)
		return FM_COMPNOTCORR;

	s->bucket_size_lev1 = fm_bit_read (16) << 10;
	s->bucket_size_lev2 = fm_bit_read (16);

	if (s->bucket_size_lev1 % s->bucket_size_lev2)
		return FM_COMPNOTCORR;

	s->num_bucs_lev1 =
		(s->text_size + s->bucket_size_lev1 - 1) / s->bucket_size_lev1;
	s->num_bucs_lev2 =
		(s->text_size + s->bucket_size_lev2 - 1) / s->bucket_size_lev2;

	/* mtf & alphabet information */
	s->alpha_size = fm_bit_read (8) + 1;


	/* read Mark mode & starting position of occ list */
	s->specialchar = (uchar) fm_bit_read (8);
	s->skip =  fm_bit_read (32);
	uint start = fm_uint_read ();

	s->start_prologue_occ = s->compress + start;
	s->start_prologue_info_sb = fm_uint_read ();
	s->pos_marked_row_extr = fm_uint_read ();

	s->subchar = (uchar) fm_bit_read (8);	/* remapped compress alphabet */

	/* some information for the user */
	#if 0
	fprintf (stdout, "Compression type %d\n", s->type_compression);
	fprintf (stdout, "Text Size %lu\n", s->text_size);
	fprintf (stdout, "Bwt EOF %lu\n",s->bwt_eof_pos);
	fprintf (stdout, "alphasize %d\n",s->alpha_size);
	fprintf(stdout, "start prologue %lu\n", s->start_prologue_occ);
	fprintf (stdout, "Compression method: ");
	switch (s->type_compression)
		{
		case MULTIH:
			fprintf (stdout, "Huffman with multiple tables.\n");
			break;
	
		default:
		return FM_COMPNOTSUP;
	}
	#endif

	/* alphabet info and inverse char maps */
	for (i = 0; i < ALPHASIZE; i++)
		s->bool_char_map[i] = fm_bit_read (1);

	for (i = 0, size = 0; i < ALPHASIZE; i++)
		if (s->bool_char_map[i])
		{
			s->char_map[i] = size;
			s->inv_char_map[size++] = (uchar) i;
		}
	assert (size == s->alpha_size);

	/* prefix summed char-occ info momorizzate con s->log2textsize bits */

	for (i = 0; i < s->alpha_size; i++)
	{			// legge somme occorrenze
		// caratteri
		s->bwt_occ[i] = fm_bit_read (s->log2textsize);
	}

	/*
	 * calcola le occorrenze di ogni carattere nel testo 
	 */
	for (i = 1; i < s->alpha_size; i++)
		s->char_occ[i - 1] = (s->bwt_occ[i]) - (s->bwt_occ[i - 1]);

	s->char_occ[(s->alpha_size) - 1] =
		(s->text_size) - (s->bwt_occ[(s->alpha_size) - 1]);

	/*
	 * Calcolo posizione inizio info buckets 
	 */
	s->sb_bitmap_size = (s->alpha_size+7)/8;
	
	s->start_prologue_info_b = s->start_prologue_info_sb + 
		(s->sb_bitmap_size*s->num_bucs_lev1)
		+ (s->alpha_size * sizeof(uint32_t) * (s->num_bucs_lev1 - 1));

	return FM_OK;
}


/*
 * Frees the memory occupied by index. 
 */
int
free_index (void * indexe)
{
	fm_index *index = (fm_index *) indexe;
	if (index == NULL) return FM_OK;
		
	if (index->compress_owner == 2 ) { // Arrivo dalla build
			free(index->compress);
			if (index->smalltext==0) free(index->mtf_seq); // libero mtf del bucket
			index->text = index->oldtext;
			}			
	
	if (index->compress_owner == 1 ) { // Arrivo lettura da file
		if (!index->smalltext) free(index->mtf_seq);
		free(index->compress);
		if(index->smalltext==2) free(index->text);
	}
	
	if (index->compress_owner == 0) { // Arrivo da lettura di memoria 
		if (!index->smalltext) free(index->mtf_seq);
		if (index->smalltext == 2) free(index->text);
		
	}

	free(index);

	return FM_OK;

} 

/*
 * Obtains the length of the text represented by index. 
 */
int
get_length(void *index, uint32_t *length)
{
	*length = ((fm_index *) index)->text_size - ((fm_index *) index)->num_marked_rows;
	return FM_OK;

}

/*
	Writes in size the size, in bytes, of index. This should be the size
	needed by the index to perform any of the operations it implements.
*/
int index_size(void *index, uint32_t *size) {

	*size = ((fm_index *) index)->compress_size;
	return FM_OK;

}

int index_size_count(void *index, uint32_t *size) {
	fm_index *i = (fm_index *) index;

	if(i->skip==0) *size = i->compress_size;
	else *size = i->compress_size - (i->num_marked_rows*i->log2_row)/8 - (i->num_marked_rows*i->log2textsize)/8;
	return FM_OK;

}
