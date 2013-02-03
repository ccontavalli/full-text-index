/* Functions to compress and decompress bucket and to count occurences 
   during search
 */
 
/* 
 	Compute # of occ of ch among the positions 0 ... k of the bwt.
	return < 0 is error occurs else the # of ch occ
*/
int occ(fm_index *index, uint32_t k, uchar ch);


/*
	Occ all
*/
int occ_all(uint32_t sp, uint32_t ep, uint32_t *occsp, uint32_t *occep,\
				  		  uchar *char_in, fm_index * s);

/* 
   Read informations from the header of the superbucket.
   "pos" is a position in the last column (that is the bwt). We
   we are interested in the information for the superbucket 
   containing "pos".
   Initializes the data structures:
        s->inv_map_sb, s->bool_map_sb, s->alpha_size_sb
   Returns initialized the array occ[] containing the number 
   of occurrences of the chars in the previous superbuckets.
*/
int get_info_sb(uint32_t pos, uint32_t *occ, fm_index *s);


/* 
   Read informations from the header of the bucket and decompresses the 
   bucket if needed. Initializes the data structures:
        index->inv_map_b, index->bool_map_b, index->alpha_size_b
   Returns initialized the array occ[] containing the number 
   of occurrences of all the chars since the beginning of the superbucket
   Explicitely returns the character (remapped in the alphabet of 
   the superbucket) occupying the absolute position pos.
   The decompression of the bucket when ch does not occur in the 
   bucket (because of Bool_map_b[ch]=0) is not always carried out. 
   The parameter "flag" setted to COUNT_CHAR_OCC indicates that we want 
   to count the occurreces of ch; in this case when Bool_map_b[ch]==0
   the bucket is not decompressed.
   When the flag is setted to WHAT_CHAR_IS, then ch is not significant 
   and we wish to retrieve the character in position k. In this case
   the bucket is always decompressed.
*/

int get_info_b(uchar ch, uint32_t pos, uint32_t *occ, int flag, fm_index *s);


/* 
   Multi-table-Huffman compressed bucket. Update the array occ[] 
   summing up all occurrencs of the chars in its prefix preceding
   the absolute position k. Note that ch is a bucket-remapped char.
*/
uchar get_b_multihuf(uint32_t k, uint32_t *occ, fm_index *s, int is_odd);


/* Functions to compress buckets */
/* 
   compress and write to file a bucket of length "len" starting at in[0].
   the compression is done as follows:
   first the charatcters are remapped (we expect only a few distinct chars
   in a single bucket) then we use mtf and we compress. 
*/ 
int compress_bucket(fm_index *s, uchar *in, uint32_t len, suint alphasize); 

/* Compute Move to front for string */
void mtf_string(uchar *in, uchar *out, uint32_t len, suint mtflen);

int fm_bwt_compress(fm_index * index);

int fm_bwt_uncompress(fm_index * index);
