/*
 * Funzioni per il calcolo delle occorrenze dei caratteri nei bucket e nei 
 * superbucket, e per la decompressione dei bucket.
 * 
 * 
 */

#include "fm_index.h"
#include "fm_extract.h"
#include "fm_mng_bits.h"
#include "fm_occurences.h"
#include <string.h> // per memcpy

static inline void unmtf_unmap (uchar * mtf_seq, int len_mtf, fm_index * s);

/*
 * Occ all Attenzione richiede che s->mtf_seq sia riempita anche se nel
 * bucket e' presente un solo carattere 
 */
int occ_all (uint32_t sp, uint32_t ep, uint32_t * occsp, uint32_t * occep,
	 uchar * char_in, fm_index * s)
{

	int i, state, diff, mod, b2end, remap;
	uint32_t occ_sb[s->alpha_size], occ_b[s->alpha_size];
	uint32_t occ_sb2[s->alpha_size], occ_b2[s->alpha_size];
	uint32_t num_buc_ep = ep / s->bucket_size_lev2;
	int char_present = 0;	// numero caratteri distinti presenti nel bucket 
	uchar *c, d;

	/*
	 * conta il numero di occorrenze fino al subperbucket che contiene sp 
	 */
	state = get_info_sb (sp, occ_sb, s);	// get occ of all chars in prev
	// superbuckets 
	if (state < 0)
		return state;

	/*
	 * Controlla se sp ed ep stanno nella stesso bucket. se si decodifica 
	 * solo un bucket 
	 */

	if (num_buc_ep == (sp / s->bucket_size_lev2))
	{	// stesso bucket
		uchar char_map[s->alpha_size_sb];
	
		if ((num_buc_ep%2 == 0)	&& ( num_buc_ep%(s->bucket_size_lev1 / s->bucket_size_lev2)!=0) 
			&& (num_buc_ep != s->num_bucs_lev2-1) ) 
			{        // bucket dispari
			state = get_info_b ('\0', sp, occ_b, WHAT_CHAR_IS, s);
			mod =  s->bucket_size_lev2 - (ep % s->bucket_size_lev2) - 1;
			diff = ep - sp;

		
			for (i = 0; i < s->alpha_size_sb; i++)
			{
				occ_b2[i] = occ_b[i];
				char_map[i] = 0;
			}
			
			c = s->mtf_seq + mod;
			int i;
			for (i=0; i < diff; i++,c++){
				occ_b2[*c]++;
				if (char_map[*c] == 0)
				{
					char_map[*c] = 1;
					char_in[char_present++] = *c;
				}
			}

			for (i = 0; i < char_present; i++)
			{
				d = s->inv_map_sb[char_in[i]];
				occsp[d] = occ_sb[d] + occ_b[char_in[i]];
				occep[d] = occ_sb[d] + occ_b2[char_in[i]];
				char_in[i] = d;
			}
			return char_present;
		} else {
			
		mod = sp % s->bucket_size_lev2;
		diff = ep - sp;
		b2end = mod + diff;	// posizione di ep nel bucket > 1023 => bucket diverso

		state = get_info_b ('\0', ep, occ_b2, WHAT_CHAR_IS, s);
		for (i = 0; i < s->alpha_size_sb; i++)
		{
			occ_b[i] = occ_b2[i];
			char_map[i] = 0;
		}

		mod++;
		c = s->mtf_seq + mod;

		for (; mod <= b2end; mod++, c++)
		{
			occ_b[*c]--;	// togli occorrenze b2
			if (char_map[*c] == 0)
			{
				char_map[*c] = 1;
				char_in[char_present++] = *c;
			}
		}

		for (i = 0; i < char_present; i++)
		{
			d = s->inv_map_sb[char_in[i]];
			occsp[d] = occ_sb[d] + occ_b[char_in[i]];
			occep[d] = occ_sb[d] + occ_b2[char_in[i]];
			char_in[i] = d;
		}

		return char_present;
	}
	}
	// calcola occorrenze fino a k e a k2
	state = get_info_b ('c', sp, occ_b, WHAT_CHAR_IS, s);
	if (state < 0)
		return state;	// if error return code

	/* add occ of All chars  */
	remap = 0;		/* rimappa i caratteri dall'alfabeto
				 * locale al superbucket a quello del
				 * testo */
	for (i = 0; i < s->alpha_size; i++)
	{
		if (s->bool_map_sb[i])
			occsp[i] = occ_sb[i] + occ_b[remap++];
		else
			occsp[i] = occ_sb[i];	/* non occorre nel sb corrente ma
						 			   devi dirgli le occorrenze fin qui !!! */
	}
	// calcola occorrenze fino a k e a k2
	state = get_info_sb (ep, occ_sb2, s);	// get occ of all chars in 
	// prev superbuckets 
	if (state < 0)
		return state;

	state = get_info_b ('c', ep, occ_b2, WHAT_CHAR_IS, s);
	if (state < 0)
		return state;	// if error return code

	/*
	 * add occ of All chars 
	 */
	remap = 0;		/* rimappa i caratteri dall'alfabeto
				 * locale al superbucket a quello del
				 * testo */
	for (i = 0; i < s->alpha_size; i++)
	{
		if (s->bool_map_sb[i])
			occep[i] = occ_sb2[i] + occ_b2[remap++];
		else
			occep[i] = occ_sb2[i];	/* non occorre nel sb corrente ma
						 * devi dirgli le occorrenze fin
						 * qui !!! */
		if (occep[i] != occsp[i])
			char_in[char_present++] = i;

	}

	return char_present;

}


/*
 * Read informations from the header of the superbucket. "pos" is a
 * position in the last column (that is the bwt). We we are interested in
 * the information for the superbucket containing "pos". Initializes the
 * data structures: s->inv_map_sb, s->bool_map_sb, s->alpha_size_sb
 * Returns initialized the array occ[] containing the number of
 * occurrences of the chars in the previous superbuckets. All
 * sb-occurences in the index are stored with log2textsize bits. 
 */
int
get_info_sb (uint32_t pos, uint32_t * occ, fm_index * s)
{

	uint32_t size, sb, *occpoint = occ, offset, i;

	if (pos >= s->text_size)
		return FM_SEARCHERR;	// Invalid pos

	// ----- initialize the data structures for I/O-reading
	sb = pos / s->bucket_size_lev1;	// superbucket containing pos

	offset = s->start_prologue_info_sb;


	// --------- go to the superbucket header 
	if (sb > 0)
	{
		// skip bool map in previous superbuckets
		// skip occ in previous superbuckets (except the first one)
		offset += (sb - 1) * (s->alpha_size * sizeof(uint32_t)) + (sb * s->sb_bitmap_size);
	} 

	fm_init_bit_reader (s->compress + offset);	// position of sb header

	/* get bool_map_sb[] */
	for (i = 0; i < s->alpha_size; i++)
	{
		fm_bit_read24 (1, s->bool_map_sb[i]);
	}

	/* compute alphabet size */
	s->alpha_size_sb = 0;
	for (i = 0; i < s->alpha_size; i++)
		if (s->bool_map_sb[i])
			s->alpha_size_sb++;

	/* Invert the char-map for this superbucket */
	for (i = 0, size = 0; i < s->alpha_size; i++)
		if (s->bool_map_sb[i])
			s->inv_map_sb[size++] = (uchar) i;

	assert (size == s->alpha_size_sb);

	/* for the first sb there are no previous occurrences */
	if (sb == 0)
	{
		for (i = 0; i < s->alpha_size; i++, occpoint++)
			*occpoint = 0;
	}
	else
	{
		/* otherwise copy # occ_map in previous superbuckets */
		memcpy(occ, s->compress + offset + s->sb_bitmap_size, s->alpha_size*sizeof(uint32_t));

	}

	return FM_OK; 
}


/*
 * Read informations from the header of the bucket and decompresses the
 * bucket if needed. Initializes the data structures: index->inv_map_b,
 * index->bool_map_b, index->alpha_size_b Returns initialized the array
 * occ[] containing the number of occurrences of all the chars since the
 * beginning of the superbucket Explicitely returns the character
 * (remapped in the alphabet of the superbucket) occupying the absolute
 * position pos. The decompression of the bucket when ch does not occur in 
 * the bucket (because of s->bool_map_b[ch]=0) is not always carried out. 
 * The parameter "flag" setted to COUNT_CHAR_OCC indicates that we want
 * to count the occurreces of ch; in this case when s->bool_map_b[ch]==0
 * the bucket is not decompressed. When the flag is setted to
 * WHAT_CHAR_IS, then ch is not significant and we wish to retrieve the
 * character in position k. In this case the bucket is always
 * decompressed. 
 */

int
get_info_b (uchar ch, uint32_t pos, uint32_t *occ, int flag, fm_index * s)
{

	uint32_t buc_start_pos, buc, size, nextbuc = 0;
	int i, offset, is_odd = 0, isnotfirst;
	uchar ch_in_pos = 0;

	buc = pos / s->bucket_size_lev2;	// bucket containing pos
	assert (buc < (s->text_size + s->bucket_size_lev2 - 1)
		/ s->bucket_size_lev2);
	isnotfirst = buc % (s->bucket_size_lev1 / s->bucket_size_lev2);

	/* read bucket starting position */
	offset = s->start_prologue_info_b + s->var_byte_rappr/8 * buc;
	fm_init_bit_reader ((s->compress) + offset);
	buc_start_pos = fm_bit_read (s->var_byte_rappr);

	if((buc%2 == 0) && (isnotfirst)  && (buc != s->num_bucs_lev2-1)) {
		is_odd = 1; // bucket per il quale non sono memorizzate le occorrenze
		nextbuc = fm_bit_read (s->var_byte_rappr);	
	}
	
	/* move to the beginning of the bucket */
	/* Se e' un bucket senza occ leggo quelle del successivo */
	if (is_odd) offset = nextbuc;
	else offset = buc_start_pos;
	fm_init_bit_reader ((s->compress) + offset);

	/* Initialize properly the occ array */
	if (isnotfirst == 0)
	{
		for (i = 0; i < s->alpha_size_sb; i++)
			occ[i] = 0;
	}
	else 
		{
		for (i = 0; i < s->alpha_size_sb; i++)
		{	
			occ[i] = fm_integer_decode (s->int_dec_bits);
		}
	}
	
	if (is_odd) { // se sono uno senza occ mi posiziono su quello corrente 
	fm_init_bit_reader ((s->compress) + buc_start_pos);
	}

	/* get bool char map */
	for (i = 0; i < s->alpha_size_sb; i++)
	{	
		fm_bit_read24 (1, s->bool_map_b[i]);
	}

	/* get bucket alphabet size and the code of ch in this bucket */
	s->alpha_size_b = 0;
	for (i = 0; i < s->alpha_size_sb; i++)
		if (s->bool_map_b[i])
			s->alpha_size_b++;	// alphabet size in the bucket

	/* if no occ of this char in the bucket then skip everything */
	if ((flag == COUNT_CHAR_OCC) && (s->bool_map_b[ch] == 0))
		return ((uchar) 0);	// dummy return

	/* Invert the char-map for this bucket */
	for (i = 0, size = 0; i < s->alpha_size_sb; i++)
		if (s->bool_map_b[i])
			s->inv_map_b[size++] = (uchar) i;

	assert (size == s->alpha_size_b);
		
	/* decompress and count CH occurrences on-the-fly */
//	switch (s->type_compression)
	//{

	//case MULTIH:		/* multihuffman compression of */		
		ch_in_pos = get_b_multihuf(pos, occ, s, is_odd);
		//break;

	//default:
		//return FM_COMPNOTSUP;
	//}
	return (int) (ch_in_pos); /* char represented in [0..s->alpha_size.sb-1] */
}

/*
 * Multi-table-Huffman compressed bucket. Update the array occ[] summing
 * up all occurrencs of the chars in its prefix preceding the absolute
 * position k. Note that ch is a bucket-remapped char. 
 */
uchar
get_b_multihuf(uint32_t k, uint32_t * occ, fm_index * s, int is_odd)
{
	
	int bit, bits = int_log2(s->alpha_size_b);
	uint32_t bpos, j;
	uchar prev;

	bpos = k % s->bucket_size_lev2;

	if (is_odd) bpos = s->bucket_size_lev2 - bpos - 1;

	if (s->alpha_size_b == 1)
	{			/* special case bucket with only one char */
		prev = s->inv_map_b[0];
		if(is_odd) {
			for (j=0; j <= bpos; j++)
			{
				s->mtf_seq[j] = prev;
				occ[prev]--;
				
			}
			occ[prev]++;
	    } else {	
			for (j = 0; j <= bpos; j++)
			{
				s->mtf_seq[j] = prev;
				occ[prev]++;
				
			}
		}
		return prev;
	}

	fm_bit_read24(bits, prev);
	
	s->mtf_seq[0] = prev = s->inv_map_b[prev];
	if (is_odd) occ[prev]--;
	else occ[prev]++;
		
	for(j=1; j<=bpos; j++) {
			  	fm_bit_read24(1, bit);
			 	if(bit){
					fm_bit_read24(bits, prev);
					s->mtf_seq[j] = prev = s->inv_map_b[prev];
				} else s->mtf_seq[j] = prev;
			
			if (is_odd) occ[prev]--;
			else occ[prev]++;			
	}
		
	if (is_odd) occ[prev]++;
			
	return prev;
}


/*
 * Receives in input a bucket in the MTF form, having length len_mtf;
 * returns the original bucket where MTF-ranks have been explicitely
 * resolved. The characters obtained from s->mtf[] are UNmapped according
 * to the ones which actually occur into the superbucket. Therefore, the
 * array s->inv_map_b[] is necessary to unmap those chars from
 * s->alpha_size_b to s->alpha_size_sb. 
 */
static inline void
unmtf_unmap (uchar * mtf_seq, int len_mtf, fm_index * s)
{
	int i, j, rank;
	uchar next;

	/* decode "inplace" mtf_seq */
	for (j = 0; j < len_mtf; j++, mtf_seq++)
	{
		rank = *mtf_seq;
		assert (rank < s->alpha_size_b);
		next = s->mtf[rank];			/* decode mtf rank */
		*mtf_seq = s->inv_map_b[next];	/* apply invamp	*/
	    assert(*mtf_seq < s->alpha_size_sb);
	
		for (i = rank; i > 0; i--)		/* update mtf list */
			s->mtf[i] = s->mtf[i - 1];
		s->mtf[0] = next;				/* update mtf[0] */
	}
}


/* Functions to compress buckets */

/* 
   compress and write to file a bucket of length "len" starting at in[0].
   the compression is done as follows:
   first the charatcters are remapped (we expect only a few distinct chars
   in a single bucket) then we use mtf and we compress. 
*/ 
int compress_bucket(fm_index *s, uchar *in, uint32_t len, suint alphasize) {
	
  int fm_multihuf_compr(uchar *, int, int);
	
  suint local_alpha_size, j;
  uchar c, local_bool_map[256], local_map[256]; 
 
  /* ---------- compute and write local boolean map ------ */
  for(j=0; j<alphasize; j++){     
    local_bool_map[j]=0;
	local_map[j]=0;
  }
  local_alpha_size=0;
  
  for(j=0;j<len;j++) {             // compute local boolean map
    c = in[j];                     // remapped char
    assert(c<alphasize);                              
    local_bool_map[c] = 1;     
  }

  for(j=0; j<alphasize; j++)      // compute local map
    if(local_bool_map[j])
      local_map[j] = local_alpha_size++; 
	
  for(j=0;j<alphasize;j++)     // write bool char map to file 
   		if(local_bool_map[j]) {fm_bit_write24(1,1);}
   		else {fm_bit_write24(1,0);} 
  
  for(j=0;j<len;j++)             // remap bucket
    in[j]=local_map[in[j]];
  
  int error = 0;
  switch ( s->type_compression ) {
	  case ( MULTIH ):
		   if (local_alpha_size == 1) { 
					fm_bit_flush(); 
					return FM_OK;
					}
			int bit = int_log2(local_alpha_size);
			char prev = in[0];
			fm_bit_write(bit, in[0]);
			for(j=1;j<len;j++) {
				if(prev==in[j]){
					fm_bit_write(1, 0);
				} else {
					fm_bit_write(1, 1);
					fm_bit_write(bit, in[j]);
				}
				prev = in[j];
								
			}
		    /* compute mtf picture */
  			/*mtf_string(in, s->mtf_seq, len, local_alpha_size);
			error = fm_multihuf_compr(s->mtf_seq, len, local_alpha_size);
			if ( error < 0 ) return error;*/
			fm_bit_flush(); 
			break;
	  default: 
	  		return FM_COMPNOTSUP;
  	}
 
  return FM_OK;

}

/* Compute Move to Front for string */
void mtf_string(uchar *in, uchar *out, uint32_t len, suint mtflen){
	
  uint32_t i,o,m;
  suint j,h;
  uchar c, mtf_start[mtflen];

  for(j=0; j<mtflen; j++) 
	  mtf_start[j] = j;
  
  m = mtflen;   // # of chars in the mtf list
  o = 0;   // # of char in the output string

  for(i=0; i<len; i++) {
    c = in[i];
    /* search c in mtf */
  	for(h=0; h<m; h++)
       	if(mtf_start[h]==c) break;
	  
    /* c found in mtf[h] */
    out[o++] = (uchar) h;   
    for(j=h; j>0; j--)   // update mtf[]
      	 mtf_start[j] = mtf_start[j-1];
    mtf_start[0] = c;
  }
  assert(o<=2*len);
}

int fm_bwt_compress(fm_index * index) {

	int fm_multihuf_compr(uchar *, int, int);	
	int error;
	
	index->compress = malloc((400+floor(1.1*index->text_size))*sizeof(uchar));
	if (index->compress == NULL) 
			return FM_OUTMEM;

	index->compress_size = 0;
	fm_init_bit_writer(index->compress, &index->compress_size);
	fm_uint_write(index->text_size);
	fm_uint_write(index->bwt_eof_pos);
	
	/* MTF */
	index->mtf_seq = (uchar *) malloc (index->text_size * sizeof (uchar));
	if (index->mtf_seq == NULL)
		return FM_OUTMEM;
	
	mtf_string(index->bwt, index->mtf_seq, index->text_size, ALPHASIZE);
	
	free(index->bwt);
	/* compress rle + huffman */
	error = fm_multihuf_compr(index->mtf_seq, index->text_size, ALPHASIZE);
	if ( error < 0 ) return error;

	fm_bit_flush(); 
	
	free(index->mtf_seq);
	index->compress = realloc(index->compress, sizeof(uchar)*index->compress_size);

	return FM_OK;
}

void fm_unmtf(uchar *in, uchar *out, int length)
{
  int i,k, pos;
  uchar mtf_list[256];

  for (i=0; i<256; i++)  // Initialize the MTF list
    mtf_list[i]= (uchar) i;
  for (i=0; i < length; i++) {
	pos = (int) in[i];
	out[i] = mtf_list[pos]; // MTF unencode the next symbol
	for (k=pos; k>0; k--) // move-to-front bwt[i]
		mtf_list[k]=mtf_list[k-1];
	mtf_list[0]=out[i];
  }
}

int fm_bwt_uncompress(fm_index * index) {
	int fm_multihuf_decompr (uchar *, int, int);

	uint32_t i;
	int error;
	index->bwt_eof_pos = fm_uint_read();
	
	index->mtf_seq = malloc(sizeof(uchar) * index->text_size);
	if (index->mtf_seq == NULL) return FM_OUTMEM;
	
	fm_multihuf_decompr (index->mtf_seq, ALPHASIZE, index->text_size);
	index->bwt = malloc(sizeof(uchar) * index->text_size);
	if (index->bwt == NULL) return FM_OUTMEM;
		
	/* The chars in the unmtf_bucket are already un-mapped */
	fm_unmtf(index->mtf_seq, index->bwt, index->text_size );
	free(index->mtf_seq);
	
	/* compute bwt occ */
  	for(i=0;i<ALPHASIZE;i++) index->bwt_occ[i]=0;
  	for(i=0;i<index->text_size;i++) index->bwt_occ[index->bwt[i]]++;
  	for(i=1;i<ALPHASIZE;i++) index->bwt_occ[i] += index->bwt_occ[i-1];
  	assert(index->bwt_occ[ALPHASIZE-1]==index->text_size);
  	for(i=ALPHASIZE-1;i>0;i--) index->bwt_occ[i] = index->bwt_occ[i-1];
	index->bwt_occ[0] = 0;

	error = fm_compute_lf(index);
	if (error < 0) return error;

	error = fm_invert_bwt(index);
	if (error < 0) return error;

	return FM_OK;
}
