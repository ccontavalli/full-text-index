/* 
 * Main Build Functions 
 */

#include "fm_index.h"
#include "fm_build.h"
#include "fm_common.h"
#include "fm_occurences.h"
#include "ds_ssort.h"
#include <string.h>
#include "fm_mng_bits.h"	/* Functions to manage bits */

#define POINTPROLOGUE (19);

static int select_subchar(fm_index *s);
int build_sa(fm_index *s);
void count_occ(fm_index *s);
int build_bwt(fm_index *s);
int compute_locations(fm_index *s);
int compute_info_superbuckets(fm_index *s);
int compute_info_buckets(fm_index *s);
void write_prologue(fm_index *s);
int compress_superbucket(fm_index *s, uint32_t num);
void write_susp_infos(fm_index *s);
void write_locations(fm_index *s);
static void dealloc(fm_index *s);
static void dealloc_bucketinfo(fm_index *s);
static int errore(fm_index *s, int error);

#if TESTINFO
typedef struct { // Solo per i test: memorizza spazio occupato
	uint32_t bucket_compr; 
	uint32_t bucket_occ;
	uint32_t bucket_alphasize;
	uint32_t bucket_pointer;		
	uint32_t sbucket_occ;       	
	uint32_t sbucket_alphasize; 	
	uint32_t prologue_size;
	uint32_t sbucket_bitmap;
	uint32_t bucket_bitmap;
	uint32_t marked_pos;
	uint32_t temp;
} measures;

measures Test;
#endif

int fm_build_config (fm_index *index, suint tc, suint freq, 
							uint32_t bsl1, uint32_t bsl2, suint owner) {
				
	bsl1 = bsl1<<10;
	/* Checks */
	if ( tc != MULTIH ) return FM_COMPNOTSUP;
	if (bsl2 > 4096) return FM_CONFERR;
	if (bsl1%bsl2) return FM_CONFERR;
	
	index->owner = owner;
	index->type_compression = tc;
	index->bucket_size_lev1 = bsl1;
	index->bucket_size_lev2 = bsl2;					
	index->skip = (suint) freq;	/* 1/Mark_freq 2% text size */
	return FM_OK;
}

int parse_options(fm_index * index, char *optionz) {
	
	int i, numtoken = 1;
  	
	/* default */
	suint tc = MULTIH;
	uint32_t bsl1 = 16;
	uint32_t bsl2 = 256;
	suint owner = 1;
	suint freq = 64;	

	if (optionz == NULL) 
			return fm_build_config (index, tc, freq, bsl1, bsl2, owner);
	
	char *options = malloc(sizeof(char)*(strlen(optionz)+1));
	if (options == NULL) 
			return FM_OUTMEM;
	memcpy(options,optionz,sizeof(char)*(strlen(optionz)+1));
	
	i = 0;
	while (options[i] != '\0' )
		if(options[i++] == ' ')	 numtoken++;

	int j = 0;
	char *array[numtoken];
	array[0] = options;
	for(i = 0; i<numtoken-1; i++) {
		while(options[j] != ' ') j++;	
		options[j++] = '\0';
		array[i+1] = options+j;	
	}

	i = 0;
	while(i<numtoken) {
		if(!strcmp(array[i], "-B")) {
			if (i+1<numtoken ) {
				bsl1 = atoi(array[++i]); 	
				i++;
				continue;
			} else return FM_CONFERR;}
		if(!strcmp(array[i], "-b")) {
			if (i+1<numtoken ) {
				bsl2 = atoi(array[++i]); 
				i++;
				continue;
			} else return FM_CONFERR; }
		if(!strcmp(array[i], "-a")) {
			if (i+1<numtoken ) {
				owner = atoi(array[++i]); 
				i++;
				continue;
			} else return FM_CONFERR;}

		if(!strcmp(array[i], "-f")) {
			if (i+1<numtoken ) {
				freq = atoi(array[++i]); 
				i++;
				continue;
			} else return FM_CONFERR;}
      		return FM_CONFERR;
	}
	free(options);

	return fm_build_config (index, tc, freq, bsl1, bsl2, owner);
	
}
/* 
	Creates index from text[0..length-1]. Note that the index is an 
	opaque data type. Any build option must be passed in string 
	build_options, whose syntax depends on the index. The index must 
	always work with some default parameters if build_options is NULL. 
	The returned index is ready to be queried. 
*/
int build_index(uchar *text, uint32_t length, char *build_options, void **indexe) {
	
	int error;
	fm_index *index;
	index = (fm_index *) malloc(sizeof(fm_index));
	if(index == NULL) return FM_OUTMEM;
	
	error = parse_options(index, build_options);
	if (error < 0) return error;
		
	error = fm_build(index, text, length);
	if (error < 0) {
			free(index);
			return error;
	}
	
	*indexe = index;
	return FM_OK;
}

/* Build */
int fm_build(fm_index *index, uchar *text, uint32_t length) {

	int error;	
	index->compress = NULL;
	index->lf = NULL;
	index->loc_occ = NULL;
	index->bwt = NULL;
	index->buclist_lev1 = NULL;
	index->start_lev2 = NULL;

	index->text = text;
	index->text_size = length;
	index->compress_size = 0;

	#if TESTINFO
	Test.bucket_compr = 0; 
	Test.bucket_occ = 0;
	Test.bucket_alphasize = 0;
	Test.sbucket_alphasize = 0; 	
	#endif
	if(index->text_size < SMALLFILESIZE) index->skip = 0;
	index->compress_owner = 2;

	if((index->owner != 0) && (index->skip <= 1)) {
		int overshoot; 
		if(index->text_size < SMALLSMALLFILESIZE) overshoot = 0;
			else overshoot = init_ds_ssort(500, 2000);
				
		uchar * texts = malloc(sizeof(uchar)*(index->text_size+overshoot));
		if ( texts == NULL) return FM_OUTMEM;
		memcpy(texts, index->text, (index->text_size)*sizeof(uchar));
		if (index->owner == 1) 
				free(index->text);
		index->text = texts;
		}
	
	if(index->text_size < SMALLSMALLFILESIZE) {
		
		index->smalltext = 1;
		index->compress_size = 0;
		index->compress = malloc((4+index->text_size)*sizeof(uchar));
		if (index->compress == NULL) 
			return FM_OUTMEM;
		index->compress_size = 0;
		fm_init_bit_writer(index->compress, &index->compress_size);
		fm_uint_write(index->text_size);
		memcpy(index->compress+4,index->text, sizeof(uchar)*index->text_size);
		index->compress_size += index->text_size;	
		
		return FM_OK;
	}	


	if(index->text_size < SMALLFILESIZE) { // writes plain text

		index->smalltext = 2;
		error = build_sa(index);
		if (error) return error;
		error = build_bwt(index);
		if (error) return error;
		free(index->lf);
		return fm_bwt_compress(index);
	}
	
	index->smalltext = 0;
	
	error = select_subchar(index);
	if (error < 0) return errore(index, error);
	if(TESTINFO) fprintf(stderr, "Select char done\n");
		
	index->log2textsize = int_log2 (index->text_size - 1);
	index->int_dec_bits = int_log2 (int_log2(index->bucket_size_lev1 - index->bucket_size_lev2));
	index->mtf_seq = (uchar *) malloc (index->bucket_size_lev2 * sizeof (uchar));
	
	if (index->mtf_seq == NULL)
		return FM_OUTMEM;	
	
	/* Build suffix array */  
   	 error = build_sa(index);		
    	if ( error < 0 ) 
			return errore(index, error);
	if(TESTINFO) fprintf(stderr,"Build suffix array done\n");
	
	count_occ(index);  /* conta occorrenze */

	/* make bwt */
	error = build_bwt(index); 
	if (error < 0 )
			return errore(index, error);
	if(TESTINFO) fprintf(stderr,"Build BWT done\n");
	
	if(index->skip>1) { free(index->text); index->text = NULL;}
	else if(index->owner == 1) {
		free(index->text); 
		index->text = NULL;
		}
	
	uint32_t i;
	/* Remap bwt */
	for(i=0; i<index->text_size; i++) 
   		index->bwt[i] = index->char_map[index->bwt[i]];
	
	/* Mark position */
	error = compute_locations(index); 
	if (error < 0) 
			return errore(index, error);
	if(TESTINFO) fprintf(stderr,"Compute locations done\n");
	
	error = compute_info_superbuckets(index);
	if (error < 0 )
			return errore(index, error);	
			
	/* Compute various infos for each bucket */ 
	error = compute_info_buckets(index);
	if (error < 0 ) 
			return errore(index, error);
	
 	/* alloc memory for compress */
	if(index->skip != 1) {
			free(index->lf);
			index->lf = NULL;
		}

	uint32_t stima_len_compress = 100 + index->text_size*1.5;
	
	stima_len_compress += index->num_marked_rows*sizeof(uint32_t); 
	
	index->compress = malloc(stima_len_compress*sizeof(uchar));
	if (index->compress == NULL) 
			return errore(index, FM_OUTMEM);
	
	index->compress_size = 0;

	/* Write Prologue on mem */
	write_prologue(index);	
	if(TESTINFO) fprintf(stderr,"Write prologue done\n");
			
	/* Compress each bucket in all superbuckets */	
	for(i=0; i<index->num_bucs_lev1; i++) { /* comprimi ogni bucket */
             error = compress_superbucket(index, i);
			 if (error < 0 ) 
					return errore(index, error);	
		 }
	if(TESTINFO) fprintf(stderr,"Compress buckets done\n");
		
    index->start_positions = index->compress_size;
		 
	/* write marked chars */
	if (index->skip>0) 
			write_locations(index);
   	if(TESTINFO) fprintf(stderr,"Write locations done\n");
	uint32_t cmp = index->compress_size;
	/* write the starting position of buckets */
  	write_susp_infos(index); 
	if(TESTINFO) fprintf(stderr,"Write susp info done\n");
	index->compress_size = cmp;
	dealloc_bucketinfo(index);

		
	if(index->skip == 1) {
			free(index->lf);
			index->lf = NULL;
	}
	
	if(index->skip > 1) {
			free(index->loc_occ);
			index->loc_occ = NULL;
	}
	
	index->compress = realloc(index->compress, index->compress_size);
	dealloc(index);
	fm_use_index (index);

	#if TESTINFO
	fprintf(stderr, "<indexSize>%lu</indexSize>/n<textSize>%lu</textSize>\n", index->compress_size, index->text_size);
	fprintf(stderr, "<textAlphasize>%d</textAlphasize>\n", index->alpha_size);
	fprintf(stderr, "<prologueSize>%lu</prologueSize>\n", Test.prologue_size);
	fprintf(stderr, "<numBuckets2>%lu</numBuckets2>\n", index->num_bucs_lev2);
	fprintf(stderr, "<numBuckets1>%lu</numBuckets1>\n", index->num_bucs_lev1);
	fprintf(stderr, "Bucket pointers size %lu Kb bits x ponter %u\n", Test.bucket_pointer/1024, index->var_byte_rappr);	
    fprintf(stderr, "Buckets compressed size %lu Kb\n", Test.bucket_compr/1024); 
	fprintf(stderr, "Buckets prefix chars occ size %lu Kb\n", Test.bucket_occ/1024);
	fprintf(stderr, "Averange buckets alpha size %lu\n", Test.bucket_alphasize/index->num_bucs_lev2);
	fprintf(stderr, "Bucket bitmaps size %lu\n\n", Test.bucket_bitmap/1024);

	fprintf(stderr, "Superbuckets size %lu # Superbuckets %lu\n", index->bucket_size_lev1, index->num_bucs_lev1);
	fprintf(stderr, "Superbuckets prefix chars occ size %lu Kb\n", Test.sbucket_occ/1024);
	fprintf(stderr, "Averange superbuckets alpha size %lu\n", Test.sbucket_alphasize/index->num_bucs_lev1);
	fprintf(stderr, "Superbucket bitmaps size %lu\n\n", Test.sbucket_bitmap/1024);
	fprintf(stderr, "# marked positions %lu\nMarked positions size %lu Kb\n\n", index->num_marked_rows, Test.marked_pos/1024);
	#endif

	return FM_OK;	
}
	 

/* 
	Cerca un carattere non presente e lo inserisce ogni s->skip posizioni nel testo originale
*/
static int select_subchar(fm_index *s) {
	uint32_t i, newtextsize, pos;
	suint mappa[ALPHASIZE];
	s->subchar = 0; // inutile in questa versione
	if (s->text_size <= s->skip) s->skip=1;
	s->oldtext = NULL;
	
	if (s->skip == 1) {
	    s->num_marked_rows  = s->text_size;
		return FM_OK;
		}
	if (s->skip == 0)  {
		s->num_marked_rows = 0;
		return FM_OK;
		}

	/* Controlla se c'e' un carattere non presente nel testo */	
	for(i=0;i<ALPHASIZE;i++) 
		mappa[i] = 0;
	
	for(i=0;i<s->text_size;i++) 
		mappa[s->text[i]] = 1;

	for(i=0;i<ALPHASIZE;i++) 
		if (!mappa[i]) {
			s->specialchar = i; /* Nuovo carattere aggiunto */
			break;
	}
	
	if(i==ALPHASIZE) {
			fprintf(stderr,"256 chars I cannot insert a new chars. I mark all positions\n");
			s->num_marked_rows  = s->text_size; s->skip = 1;
			return FM_OK;
	}
	int overshoot;

	s->num_marked_rows = s->text_size/s->skip;
	newtextsize = s->text_size + s->num_marked_rows;
	overshoot = init_ds_ssort(500, 2000);
	
	uchar * text = malloc(overshoot+newtextsize);
	if (!text) return FM_OUTMEM;
	
	for(i=0,pos = 0;i<s->num_marked_rows; i++, pos += s->skip) {
		if (pos) text[pos++] = s->specialchar;
		memcpy(text+pos, s->text+pos-i, s->skip);		
		}
	uint32_t offset = s->text_size - (pos - i)-1;
	if(offset){
		if (pos) text[pos++] = s->specialchar;
		memcpy(text+pos, s->text+pos-i, offset);
	}
	
	if (s->owner == 1)  { 
		free(s->text); 
		s->oldtext = NULL;
	} else s->oldtext = s->text;

	s->text = text;
	s->text_size = newtextsize;
	return FM_OK;
	
}


/* 
   build suffix array for the text in s->text[] not remapped
   use library ds_ssort whitout overshoot ( for problem realloc 
   text )
   Input:
     s->text, s->text_size
   Ouput:
     s->lf
*/

int build_sa(fm_index *s) { 
	
  s->lf = malloc(s->text_size * sizeof(uint32_t));
  if (s->lf == NULL) 
	  	return FM_OUTMEM;

  /* compute Suffix Array with library */  
  ds_ssort(s->text, (int*) s->lf, s->text_size);
  return FM_OK;

}

/* 	
	This procedure count occurences in the text.
	Init s->alpha_size, s->bool_char_map, s->pfx_char_occ
*/

void count_occ(fm_index *s) {
	
	/* count occurences */
	uint32_t i;
	uchar curchar;

	s->alpha_size = 0;
	for ( i = 0; i < ALPHASIZE; i++) { // init occ array
		 s->bool_char_map[i] = 0;
		 s->pfx_char_occ[i] = 0;		
	}
   
	/* calcolo numero occorrenze caratteri, alpha_size e bool_char_map */
	for (i = 0; i < s->text_size; i++) {
		curchar = s->text[i];
		if ( s->bool_char_map[curchar] == 0 ) {
				s->alpha_size++;
				s->bool_char_map[curchar] = 1;
		}
		s->pfx_char_occ[curchar]++;
	}
	
	/* remap dell'alfabeto */
	suint free = 0; /* primo posto libero in s->char_map */
	for (i =0; i < ALPHASIZE; i++) {
		if ( s->bool_char_map[i] == 1 ) {
			s->char_map[i] = free;
			s->pfx_char_occ[free] = s->pfx_char_occ[i];
			free++;
		}
	}
	assert(free == s->alpha_size);
	
	/* Compute prefix sum of char occ */
	uint32_t temp, sum = 0;
    for(i=0; i<s->alpha_size; i++) {
    	temp = s->pfx_char_occ[i];
    	s->pfx_char_occ[i] = sum; 
    	sum += temp;
  	}	

}

/*
   compute BWT of input text.
   Remap alphabet when bwt is computed.
   La rimappatura potrebbe essere fatta in contemporanea con 
   la creazione del bwt ( con s->char_map[s->text[s->lf[i]-1]] )
   ma e' piu lenta di + di 1 secondo su 9.
   Input:
     s->text, s->text_size, s->lf
   Output
     s->bwt, s->bwt_eof_pos
*/ 

int build_bwt(fm_index *s) {
  
  uint32_t i;
  
  /* alloc memory for bwt */
  s->bwt = malloc(s->text_size*sizeof(uchar));
  if(s->bwt==NULL)
  			return FM_OUTMEM;
  
  s->bwt[0] = s->text[s->text_size-1];  //L[0] = Text[n-1]

  /* finche non incontro l'EOF il bwt e' piu' avanti di un elemento 
  	 dopo gli indici di sa e bwt ritornano uguali. 
  */
  
  uint32_t *sa = s->lf;		 	 // punta al primo elemento 
  uchar *bwt = &(s->bwt[1]);  	 // punta al secondo 
  for(i=0; i<s->text_size; i++) {  // non leggibile ma piu' performante 
    if(*sa !=0 ){		     	 // al posto di *bwt c'era s->bwt[j++] con j=1
      *bwt = s->text[(*sa)-1]; 	 // al posto di *safix s->lf[i]
	  bwt++; 
	} else
      s->bwt_eof_pos = i; // EOF is not written but its position remembered !	
	sa++;
	}

   return FM_OK;	
}

/* 
  	Si ricorda delle posizioni. Funzionante per multilocate 
*/
int compute_locations(fm_index *s) { 

	uint32_t i,j; /* numero posizioni marcate */
	uint32_t firstrow = s->pfx_char_occ[s->char_map[s->specialchar]];

  	if( (s->skip==0)|| (s->skip == 1)) 
    	return FM_OK;
	uint32_t real_text_size = s->text_size-s->num_marked_rows;
	/* alloc s->loc_occ */
  	s->loc_occ = malloc(sizeof(uint32_t) * s->num_marked_rows);
	if (s->loc_occ == NULL) return FM_OUTMEM;
	s->loc_row = malloc(sizeof(uint32_t) * real_text_size/s->skip);
	if (s->loc_row == NULL) return FM_OUTMEM;
		
	for(i=firstrow,j=firstrow; i<(s->num_marked_rows+firstrow); i++,j++) {
					//if(s->lf[i] == 0) { PLEASE FIX ME
					//	j++;
					//	continue;
					//	}
					s->loc_occ[i-firstrow] = s->lf[j];	
				}
	uint32_t curpos;
	uint32_t p =0;
	for(i=0;i<s->text_size;i++) { 
		if((i>=firstrow)&&(i<firstrow+s->num_marked_rows)) continue;
		if(s->lf[i]==0) continue;
		curpos = s->lf[i]-(s->lf[i]/(s->skip+1));
		if(curpos%s->skip==0) {
					p++;
					s->loc_row[curpos/s->skip-1] = i; //(i < s->bwt_eof_pos) ? i+1 :  i;
		}
	}
	
  return FM_OK;
   
}

/* 
   Init s->buclist_lev1 (list of superbuckets)
   For each superbuckets init the fields:  
   	occ[], alpha_size, bool_char_map[]
*/             

int compute_info_superbuckets(fm_index *s)
{
  uint32_t b, temp, occ, k, i;
  bucket_lev1 * sb;
  
  /* compute number of superbuckets  */
  s->num_bucs_lev1 = (s->text_size + s->bucket_size_lev1 - 1) / s->bucket_size_lev1;

  /* alloc superbuckets */
  s->buclist_lev1 = (bucket_lev1 *) malloc(s->num_bucs_lev1 * sizeof(bucket_lev1));
  if(s->buclist_lev1==NULL) return FM_OUTMEM; 
                
  /* alloc aux array for each superbucket */
  for(i=0; i< s->num_bucs_lev1; i++) {
    sb = &(s->buclist_lev1[i]);   // sb points to current superbucket

    /* Allocate space for data structures */
    sb->occ = malloc((s->alpha_size)* sizeof(uint32_t));
    if(sb->occ == NULL) 
			return FM_OUTMEM;
	
    sb->bool_char_map = malloc((s->alpha_size)*sizeof(suint));
    if(sb->bool_char_map == NULL) 
			return FM_OUTMEM;

    /* Initialize data structures */
    sb->alpha_size = 0;    
    for(k=0; k<s->alpha_size; k++){
      sb->bool_char_map[k] = 0;
	  sb->occ[k] = 0;}
  	
  }  
  
  /* scan bwt and gather information */
  
  uint32_t currentBuck = 0;  // indice superbuckets
  uint32_t dim = s->bucket_size_lev1; // per non dividere
  
  sb =  &s->buclist_lev1[0];
  for(i=0; i<s->text_size; i++) {
	
	  if ( i == dim ) {  // NON DIVIDERE sono in ordine 
		         currentBuck++;
		         dim += s->bucket_size_lev1;
	  			 sb = &(s->buclist_lev1[currentBuck]);
		         }
		          
    k = s->bwt[i];                 // current char
    		 
    if(sb->bool_char_map[k]==0) { // build char_map of current sb
      sb->bool_char_map[k] = 1; 
	  sb->occ[k] = 0;
      sb->alpha_size++;           // compute alphabet size
    }
    sb->occ[k]++;
  }

  /* compute occ in previous buckets */
  for(k=0; k<s->alpha_size;k++) {
    occ = 0;
    for(b=0; b<s->num_bucs_lev1; b++) {   //prefix sum on OCC
      temp = s->buclist_lev1[b].occ[k];
      s->buclist_lev1[b].occ[k]=occ;
	  occ += temp;
	  
	  #if TESTINFO
  	  Test.sbucket_alphasize += s->buclist_lev1[b].alpha_size;
	  Test.bucket_bitmap += (s->buclist_lev1[b].alpha_size*(s->bucket_size_lev1/s->bucket_size_lev2));
  	  #endif
    }
  }

  return FM_OK;
}

/* 
   Init s->start_lev2 which is the starting position of each bucket
   in the compressed file
*/             
int compute_info_buckets(fm_index *s)
{
  /* compute number of buckets */
  assert((s->bucket_size_lev1 % s->bucket_size_lev2) == 0);
  s->num_bucs_lev2 = (s->text_size + s->bucket_size_lev2 - 1) / s->bucket_size_lev2;

  /* alloc array for buckets starting positions */
  s->start_lev2 =  malloc((s->num_bucs_lev2)* sizeof(uint32_t));
  if(s->start_lev2==NULL) 
	  	return FM_OUTMEM;
  return FM_OK;	  
}


/* ---------------------------------------------------------
   The current format of the prologue is the following:
	8 bits	type of compression (4=Multi Table Huff 5=gamma 6=mtf2)
   32 bits 	size of input file (text)
   32 bits  position of eof in s->bwt
   16 bits  size of a super bucket divided by 1024
   16 bits  size of a bucket divided (divides the previous)
	8 bits  size-1 of the compacted alphabet of the text
	8 bits 	s->specialchar remapped eith s->char_map
   32 bits	s->skip expected # of chars between 2 marked positions 
   32 bits  occ-explicit list
   32 bits  s->start_prologue_info_sb
    8 bits  s->char_map[s->subchar]
  256 bits  s->bool_char_map: bit i is 1 iff char i occurs in the text

   	  		For each c that occurs in ter text, writes the s->pfx_char_occ[c]
      		on s->log2textsize bits
	x bits  bit_flush() in order to be aligned to byte 
		
  			For each Superbucket i 
			- s->alpha_size bits to store the Sb bitmap
			- for each char c that occurs in te text 
			  stores the sb.occ[c] (prefix sum of character occurrences) 
			  with s->log2textsize (except for the first sbucket)
	x bits  bit_flush() in order to be aligned to byte 
	
			Stores starting position of each bucket in the compressed file 
			using (s->log2textsize+7)/8 * 8 bits. This is byte aligned.
			
*/
  
void write_prologue(fm_index *s) {

  bucket_lev1 sb;
  uint32_t i,k;

  /* write file and bucket size */
  fm_init_bit_writer(s->compress, &s->compress_size);
  fm_uint_write(s->text_size);
  fm_bit_write24(8, s->type_compression);
  fm_uint_write(s->bwt_eof_pos);

  assert(s->bucket_size_lev1>>10<65536);
  assert((s->bucket_size_lev1 & 0x3ff) == 0);
  fm_bit_write24(16, s->bucket_size_lev1>>10);

  //assert(s->bucket_size_lev2>=256);
  assert(s->bucket_size_lev2<=4096);
  fm_bit_write24(16, s->bucket_size_lev2);

  /* alphabet information */
  assert(s->alpha_size>0 && s->alpha_size<=ALPHASIZE);
  fm_bit_write24(8, s->alpha_size-1);   
  
  /* write starting byte of occ-list */
  fm_bit_write24(8, s->char_map[s->specialchar]); /* carattere speciale */
  fm_bit_write(32, s->skip);
  fm_uint_write(0); /* spazio libero per occ-explicit list 20esimo byte--*/
  fm_uint_write(0); /* spazio libero per mettere inizio sb info 25esimo byte */
  fm_uint_write(0); /* 29 byte start occ*/
  fm_bit_write24(8, s->char_map[s->subchar]); /* carattere sostituito */
  
  /* boolean alphabet char map */
  for(i=0; i<ALPHASIZE; i++)
   if (s->bool_char_map[i]) {fm_bit_write24(1,1);}
   else {fm_bit_write24(1,0); }

  /* write prefix sum of char occ  Da valutare vantaggi in compressione vs tempo */
  for(i=0; i < s->alpha_size; i++) {
   	fm_bit_write(s->log2textsize, s->pfx_char_occ[i]);   
	}
  fm_bit_flush(); 
 
  s->start_prologue_info_sb = s->compress_size;
 
  /* Process superbuckets */
  for(i=0;i<s->num_bucs_lev1;i++) {
	sb = s->buclist_lev1[i];

	for(k=0; k<s->alpha_size; k++)  // boolean char_map
     	if(sb.bool_char_map[k]){ fm_bit_write24(1,1);}
     	else { fm_bit_write24(1,0);}
	fm_bit_flush();
  	if(i>0) // write prefix-occ
		{ 
		uchar *dest = s->compress+s->compress_size;
		dest = memcpy(dest, sb.occ, sizeof(uint32_t) * s->alpha_size);
	  	s->compress_size += sizeof(uint32_t) * s->alpha_size;
	  	fm_init_bit_writer(s->compress+s->compress_size, &s->compress_size);
	 }
  }

  /* leave space for storing the start positions of buckets */
  s->var_byte_rappr = ((s->log2textsize+7)/8)*8;   // it's byte-aligned
 
  for(i=0;i<s->num_bucs_lev2;i++)
  	fm_bit_write(s->var_byte_rappr,0);
  
  #if TESTINFO
  Test.bucket_pointer = (s->var_byte_rappr)*s->num_bucs_lev2/8;
  Test.sbucket_occ = (s->num_bucs_lev1-1)*(s->log2textsize*s->alpha_size)/8;
  Test.sbucket_bitmap = s->alpha_size*s->num_bucs_lev1/8;
  Test.bucket_bitmap = Test.bucket_bitmap/8;
  Test.prologue_size = s->compress_size;
  #endif

}


/* 
   compress a superbucket

   NOTE the content of s->bwt is changed (remapped) !!!! 

   The number of occurrences of each char is represented using
   integer_encode.
*/
int compress_superbucket(fm_index *s, uint32_t num)
{
  
  bucket_lev1 sb;  
  uchar *in, c, char_map[ALPHASIZE];
  uint32_t k, temp, sb_start, sb_end, start,len, bocc[ALPHASIZE], b2,i,j;
  int is_odd;

  assert(num<s->num_bucs_lev1);

  sb = s->buclist_lev1[num];             // current superbucket
  sb_start = num * s->bucket_size_lev1; // starting position of superbucket 
  sb_end = MIN(sb_start+s->bucket_size_lev1, s->text_size);    
  b2 = sb_start/s->bucket_size_lev2;    // initial level 2 bucket il numero del bucket in assoluto

  temp=0;                               // build char map for superbucket 
  for(k=0;k<s->alpha_size;k++) 
    if(sb.bool_char_map[k]) 
      char_map[k] = temp++;
	
  assert(temp==sb.alpha_size);

  for(i=0; i<sb.alpha_size;i++)              // init occ
    bocc[i]=0;

  for(start=sb_start; start<sb_end; start+=s->bucket_size_lev2, b2++) {

	s->start_lev2[b2] = s->compress_size; // start of bucket in compr file

    len = MIN(s->bucket_size_lev2, sb_end-start);    // length of bucket
    in = s->bwt + start;  // start of bucket
	
	is_odd = 0;
	if((b2%2 ==0) && (start != sb_start) && (b2 != s->num_bucs_lev2-1))
		is_odd = 1; // decide quali bucket rovesciare e non mettere occ
	
    if(start != sb_start)       // if not the first bucket write occ to file
			if(!is_odd) 
				for(k=0; k<sb.alpha_size; k++) {
				#if TESTINFO	
				Test.temp = s->compress_size;
			 	#endif
			
				fm_integer_encode(bocc[k], s->int_dec_bits); 
			
				#if TESTINFO	
				Test.bucket_occ+=(s->compress_size-Test.temp);
			 	#endif
				
	  		}
	
    for(j=0; j<len; j++) {          // update occ[] and remap
      assert(in[j]< s->alpha_size); 
      c = char_map[in[j]];          // compute remapped char
      assert(c < sb.alpha_size);          
      in[j]=c;                      // remap
      bocc[c]++;                    // bocc is used in the next bucket
    }    
	
	// ROVESCIA i bucket dispari
	if(is_odd) {
			uchar intemp[len];
			for(j=0; j<len; j++) {
				intemp[len-j-1] = in[j];
			}
			for(j=0; j<len; j++) in[j] = intemp[j];
			}
	#if TESTINFO	
	Test.temp = s->compress_size;
	#endif

	int error = compress_bucket(s, in, len, sb.alpha_size);
	
	#if TESTINFO	
	Test.bucket_compr+=(s->compress_size-Test.temp-(sb.alpha_size/8));
	#endif
	if (error < 0) return error;
}

  return FM_OK;
}

/*
   write the starting position (in the output file) of each one 
   of the s->num_bucs_lev2 buckets. These values are written at the
   end of the prologue just before the beginning of the first bucket.
   It writes also the starting position of the occurrence list
*/ 
void write_susp_infos(fm_index *s) {
  	 
  	uint32_t i, offset, unuseful = 0;
	uchar *write;

  	/* write starting position of points to positions  marked and sb occ list */
  	// warning! the constant POINTPROLOGUE depends on the structure of the prologue!!!
	write = s->compress + POINTPROLOGUE;
  	fm_init_bit_writer(write, &unuseful); // vado a scrivere dove avevo lasciato spazio
  	fm_uint_write(s->start_positions);
	fm_uint_write(s->start_prologue_info_sb); // inizio sbuckets info
	fm_uint_write(s->pos_marked_row_extr); 

  	// Warning: the offset heavily depends on the structure of prologue.
  	//          The value of start_level2[0] has been initialized in
  	//          the procedure compress_superbucket()

  	offset = s->start_lev2[0] - s->num_bucs_lev2*(s->var_byte_rappr/8);
	write = s->compress + offset;
 	fm_init_bit_writer(write, &unuseful);
  	for(i=0;i<s->num_bucs_lev2;i++)
          fm_bit_write(s->var_byte_rappr, s->start_lev2[i]);

	fm_bit_flush();
	write = s->compress + s->start_positions;
	fm_init_bit_writer(write, &s->compress_size); // ritorno a scrivere in coda alla memoria
}


/*
	Se s->skip>1 scrive nel compresso tutte le posizioni contenute in 
	s->loc_occ utilizzando s->log2textsize bits per ciascuna posizione.
    write the position of the marked chars
*/
void write_locations(fm_index *s) {

 	uint32_t i;
	fm_init_bit_buffer();
	if(s->skip == 1) { // marcamento completo file 	
		for(i=0; i<s->text_size; i++) 
			fm_bit_write(s->log2textsize, s->lf[i]);
		fm_bit_flush();
		#if TESTINFO
		Test.marked_pos = s->log2textsize*s->num_marked_rows/8;
		#endif
		return;
	}
	
	/* Marcamento sostituisci char bit log2 text_size*/
	for(i=0; i<s->num_marked_rows;i++) 
		fm_bit_write(s->log2textsize, (s->loc_occ[i]-(s->loc_occ[i]/(s->skip+1)))-1);
		
	fm_bit_flush();
	
	s->pos_marked_row_extr = s->compress_size;
	uint32_t real_text_size = s->text_size - s->num_marked_rows;
	s->log2_row = int_log2(s->text_size);
  	for(i=0; i<real_text_size/s->skip; i++) {
		fm_bit_write(s->log2_row, s->loc_row[i]);
	}
	fm_bit_flush();

	#if TESTINFO
	Test.marked_pos = s->log2textsize*s->num_marked_rows/8;
	#endif
}

/* 
   Libera la memoria allocata.  Viene usata in caso di errore 
   o alla fine della compressione.
*/
static void dealloc(fm_index *s) {
	free(s->lf);
	dealloc_bucketinfo(s);
	free(s->start_lev2);
	free(s->loc_occ);
	if ((s->owner == 0) && (s->skip !=0)) {
		if (s->subchar != s->specialchar) { 
			int i;
			for(i=0;i<s->alpha_size;i++)
				if(s->text[i] == s->subchar) s->text[i] = s->specialchar;
			}				
	} else free(s->text);

}

static void dealloc_bucketinfo(fm_index *s){
	uint32_t i;
	free(s->bwt);
	bucket_lev1 *sb;
	if(s->buclist_lev1 != NULL)
			for(i=0; i< s->num_bucs_lev1; i++) {
				sb = &(s->buclist_lev1[i]); 
    			free(sb->occ);
    			free(sb->bool_char_map);
	}
	free(s->buclist_lev1);
	s->buclist_lev1 = NULL;
	s->bwt = NULL;
}	

static int errore(fm_index *s, int error) {
	free(s->compress);
	free(s->mtf_seq);
	dealloc(s);
	return error;
}

/*
	Saves index on disk by using single or multiple files, having 
	proper extensions. 
*/
int save_index(void *indexe, char *filename) {

	fm_index *index = (fm_index *) indexe;
	FILE *outfile;
	char *outfile_name, *ext = EXT;
	
	outfile_name = (char *) malloc(strlen(filename)+strlen(ext)+1);
	if (outfile_name == NULL) return FM_OUTMEM;
	outfile_name = strcpy(outfile_name, filename);
	outfile_name = strcat(outfile_name, ext);
	
	outfile = fopen(outfile_name, "wb");
	if(outfile == NULL) { 
		free(outfile_name);
		return FM_FILEERR;
	}

    	free(outfile_name);

	uint32_t t = fwrite(index->compress, sizeof(uchar), index->compress_size, outfile);
	if(t != index->compress_size) {
		fclose(outfile);
		return FM_FILEERR;
	}
    
    fclose(outfile);
	return FM_OK;
}

int save_index_mem(void *indexe, uchar *compress){

    fm_index *index = (fm_index *) indexe;
    memcpy(compress, index->compress, sizeof(uchar)*index->compress_size);
    return FM_OK;

}

/* 
  Opens and reads a text file 
*/
int fm_read_file(char *filename, uchar **textt, uint32_t *length) {

  uchar *text;
  unsigned long t, overshoot = (uint32_t) init_ds_ssort(500, 2000); 
  FILE *infile;
  
  infile = fopen(filename, "rb"); // b is for binary: required by DOS
  if(infile == NULL) return FM_FILEERR;
  
  /* store input file length */
  if(fseek(infile,0,SEEK_END) !=0 ) return FM_FILEERR;
  *length = ftell(infile);
  
  /* alloc memory for text (the overshoot is for suffix sorting) */
  text = malloc(((*length) + overshoot)*sizeof(*text)); 
  if(text == NULL) return FM_OUTMEM;  
  
  /* read text in one sweep */
  rewind(infile);
  t = fread(text, sizeof(*text), (size_t) *length, infile);
  if(t!=*length) return FM_READERR;
  *textt = text;
  fclose(infile);
  return FM_OK;
}
