/*
 * Main Search Functions Rossano Venturini 
 */

#include "fm_index.h"
#include "fm_search.h"
#include "fm_mng_bits.h"	/* Functions to manage bits */
#include "fm_occurences.h"

static int allocated, used;	/* var usate dalla count multipla */

int count_row_mu (fm_index * index, uchar * pattern, uint32_t len,
		  uint32_t sp, uint32_t ep);

int fm_multi_count (fm_index * index, uchar * pattern,
		    uint32_t len, multi_count ** list);

int multi_locate (uint32_t sp, uint32_t element, uint32_t * positions, fm_index *);

/*
 * Writes in numocc the number of occurrences of pattern[0..length-1] in
 * index. It also allocates occ (which must be freed by the caller) and
 * writes the locations of the numocc occurrences in occ, in arbitrary
 * order. 
 */
int
locate (void *indexe, uchar * pattern, uint32_t length, uint32_t ** occ,
	uint32_t * numocc)
{

	multi_count *groups;
	int i, num_groups = 0, state;
	uint32_t *occs = NULL;
	fm_index * index = (fm_index *) indexe;
	
	*numocc = 0;
	
	if(index->smalltext)  //uses Boyer-Moore algorithm
		return fm_boyermoore(index, pattern, length, occ, numocc);
	
	/* count */
	num_groups = fm_multi_count (index, pattern, length, &groups);

	if (num_groups <= 0)
		return num_groups;

	for (i = 0; i < num_groups; i++)
		*numocc += groups[i].elements;

	occs = *occ = (uint32_t *) malloc (sizeof (uint32_t) * (*numocc));
	if (*occ == NULL)
		return FM_OUTMEM;

	for (i = 0; i < num_groups; i++)
	{
		state = multi_locate (groups[i].first_row, groups[i].elements,
				      occs, index);
		if (state < 0)
		{
			free (*occ);
			return state;
		}
		occs += groups[i].elements;
	}

	free (groups);
	return FM_OK;

}


/*
 * Writes in numocc the number of occurrences of pattern[0..length-1] in
 * index. 
 */
int
count (void *indexe, uchar * pattern, uint32_t length, uint32_t * numocc)
{
	fm_index * index = (fm_index *) indexe;
	multi_count *groups;
	int i, num_groups = 0;

	*numocc = 0;

	if(index->smalltext) { //uses Boyer-Moore algorithm
		uint32_t *occ;
		int error = fm_boyermoore(index, pattern, length, &occ, numocc);
		if(*numocc>0) free(occ);
		return error;		
	}
	
	num_groups = fm_multi_count (index, pattern, length, &groups);

	if (num_groups <= 0)
		return num_groups;

	for (i = 0; i < num_groups; i++)
		*numocc += groups[i].elements;

	free (groups);
	return FM_OK;

}


static multi_count *lista;

#define ADD_LIST(_first_row, _elements) {\
	if ((used+1) == allocated) {\
		allocated += 5;\
		lista = realloc(lista, sizeof(multi_count)*allocated);\
		if (lista == NULL) return FM_OUTMEM;\
		}\
	lista[used].first_row = _first_row;\
	lista[used++].elements = _elements;\
}


/*
 * Count ricorsiva per la multilocate. se nel pattern c'e' una occorenza
 * di subchar allora devo dividere la ricerca con due rami distinti. 
 */
int
count_row_mu (fm_index * index, uchar * pattern, uint32_t len, uint32_t sp,
	      uint32_t ep)
{
	uchar chars_in[index->alpha_size];
	uint32_t occsp[index->alpha_size], occep[index->alpha_size];
	int num_char, i, find = 0;
	uchar c;
	uint32_t ssp, sep;
	/*
	 * Versione semplice - possibile fare meglio 
	 */
	while ((sp <= ep) && (len > 0))
	{
		c = pattern[--len];
		find = 0;
		if(sp == 0) 
			num_char = occ_all (0, EOF_shift (ep), occsp, occep, chars_in, index);
		else 
			num_char = occ_all (EOF_shift (sp - 1), EOF_shift (ep), occsp, occep, chars_in, index);
        ep = 0;	sp = 1;
		for(i=0; i<num_char; i++) {
			if ((index->skip >1) && (chars_in[i] == index->specialchar))
			{
				ssp = index->bwt_occ[index->specialchar] + occsp[index->specialchar]; 
				sep = ssp +(occep[index->specialchar] - occsp[index->specialchar]) - 1;
				assert((ssp<index->text_size)&&(sep<index->text_size));
				count_row_mu (index, pattern, len+1, ssp, sep);		
				if (find==1) break;
				find = 1;
			}
			if (chars_in[i] == c) {
				sp = index->bwt_occ[c] + occsp[c]; 
				ep = sp + (occep[c] - occsp[c]) - 1;
				assert((sp<index->text_size)&&(ep<index->text_size));
				if (find==1) break;
				find = 1;
			}
		}
	}
	
		
	/*
	 * return number of occurrences 
	 */
	if (ep < sp)
		return FM_OK;	/* nessuna occorrenza */
	ADD_LIST (sp, ep - sp + 1);	/* Aggiunge gruppo trovato alla lista */

	return FM_OK;
}


/*
 * Count per la ricerca con marcatura per multi locate il problema con
 * questo tipo di marcatura e' che si devono considerare due rami di
 * ricerca ogni volta che si incontra il carattere sostituito nel
 * pattern. Questa divisione peggiora le prestazioni della count. La
 * ricerca sui diversi cammini e' implementata con una chiamata ricorsiva
 * non molto efficiente. 
 */
int
fm_multi_count (fm_index * index, uchar * pattern, uint32_t len,
		multi_count ** list)
{

	uint32_t sp, ep, i, j;
	uchar c;

	allocated = len;
	used = 0;

	lista = malloc (allocated * sizeof (multi_count));
	if (lista == NULL)
		return FM_OUTMEM;

	/*
	 * remap pattern 
	 */
	assert (len > 0);

	for (i = 0; i < len; i++)
	{
		if (index->bool_char_map[pattern[i]] == 0)
			{
				for (j = 0; j <= i; j++) 
					pattern[j] = index->inv_char_map[pattern[j]];
				free(lista);
				return 0;	/* char not in file */
			}
			
		pattern[i] = index->char_map[pattern[i]];	/* remap char */
		if((index->skip >1)&&(pattern[i]==index->specialchar))
			{
				for (j = 0; j <= i; j++) 
					pattern[j] = index->inv_char_map[pattern[j]];
				free(lista);
				return 0;	/* char not in file */
			}
			
	}

	/* get initial sp and ep values */
	c = pattern[len - 1];
	sp = index->bwt_occ[c];
	if (c == index->alpha_size - 1)
		ep = index->text_size - 1;
	else
		ep = index->bwt_occ[c + 1]- 1;
	
	count_row_mu (index, pattern, len - 1, 	sp, ep);	// ricerca per il carattere c

	#if 0
	if (used = 0)
		lista = realloc (lista, sizeof (multi_count) * used);
	else
		free (lista);
	#endif
	
	if(used == 0) {
		free(lista); lista = NULL;
	}
	
	/* inverse remap pattern  */
	for (i = 0; i < len; i++) 
		pattern[i] = index->inv_char_map[pattern[i]];
	
	*list = lista;

	return used;
}


/*
 * Restituisce la posizione nel testo memorizzata sul compresso in i-esima 
 * posizione. 
 */
static inline void
get_pos (uint32_t first_row, uint32_t element, suint step, uint32_t * pos,
	 	 fm_index * s)
{
	int offset, skipbits;
	uint32_t postext, i;
	offset = first_row * s->log2textsize;
	fm_init_bit_reader (s->start_prologue_occ + (offset >> 3));
	skipbits = offset % 8;	// bits are to be skipped 
	if (skipbits)
	{
		fm_bit_read24 (skipbits, skipbits);
	}

	for (i = 0; i < element; i++, pos++)
	{			// cerca tutto il gruppo
		postext = fm_bit_read (s->log2textsize);	// read text pos 
		*pos = postext + step;
	}

}


/*
 * Multilocate 
 */
int
multi_locate (uint32_t sp, uint32_t element, uint32_t * positions, fm_index * index)
{

	if(element == 0) return FM_OK;
	if(index->skip == 0) return FM_NOMARKEDCHAR;
	if(index->skip == 1) {
		get_pos (sp, element, 0, positions, index);
		return element;
	}

	uint32_t curr_row, used, recurs, elements, diff;
	uint32_t *elem_array;	// contiene il numero di elementi del sottogruppo
	uint32_t occsp[index->alpha_size];
	uint32_t occep[index->alpha_size];
	suint *step_array, step;	// come sopra ma passi
	uchar chars_in[index->alpha_size];
	int j, state, num_char;
	/*
	 * per singola locate 
	 */
	uint32_t occ_sb[index->alpha_size], occ_b[index->alpha_size];
	suint localstep;
	uchar c, c_sb, cb;

	/* Caso in cui l'ultimo carattere e' s->specialchar 
	   ovvero le riga sp e sp+element sono compresse tra  
	   index->bwt_occ[index->specialchar]  e  
       index->bwt_occ[index->specialchar+1] */
	if ((sp >= index->occcharinf) && (sp+element-1 < index->occcharsup)) {
		get_pos (sp - index->occcharinf, element, 0, positions, index);
		return element;
	}
	
	step_array = malloc (sizeof (uint32_t) * element);
	elem_array = malloc (sizeof (uint32_t) * element);
	if ((step_array ==NULL) || (elem_array == NULL))
		return FM_OUTMEM;
	
	used = 0;
	recurs = element - 1;
	elem_array[recurs] = element;
	positions[recurs] = sp;
	step_array[recurs] = 0;

	while (recurs < element)
	{
		elements = elem_array[recurs];
		curr_row = positions[recurs];
		step = step_array[recurs++] + 1;

		if ((index->bwt_eof_pos >= curr_row) &&
		    (index->bwt_eof_pos < curr_row + elements))
			positions[used++] = step - 1;

		num_char =
			occ_all (EOF_shift (curr_row - 1),
				 EOF_shift (curr_row + elements - 1), occsp,
				 occep, chars_in, index);

		for (j = 0; j < num_char; j++)
		{
			cb = chars_in[j];
			diff = occep[cb] - occsp[cb];
			if (cb == index->specialchar)
			{	
				get_pos (occsp[cb], diff, step, positions + used, index);	
				/* pos of * + step  +1 */
				used += diff;
				continue;
			}

			if (diff == 1)
			{
				curr_row = occsp[cb] + index->bwt_occ[cb];
				assert(curr_row<index->text_size);
				c_sb = cb;
				localstep = step;
				while (c_sb != index->specialchar)
				{
					if (curr_row == index->bwt_eof_pos)
					{
						positions[used++] = localstep;
						break;
					}
					state = get_info_sb (EOF_shift
							     (curr_row),
							     occ_sb, index);
					if (state < 0)
						return FM_SEARCHERR;
					state = get_info_b (NULL_CHAR,
							    EOF_shift
							    (curr_row), occ_b,
							    WHAT_CHAR_IS,
							    index);
					c = state;
					if (state < 0)
						return FM_SEARCHERR;
					c_sb = index->inv_map_sb[c];
					curr_row =
						index->bwt_occ[c_sb] +
						occ_sb[c_sb] + occ_b[c] - 1;
					localstep++;
				}
				if (curr_row == index->bwt_eof_pos)
					continue;
				get_pos (curr_row - index->occcharinf, 1, localstep,
					 positions + used, index);
				used++;
				continue;
			}

			elem_array[--recurs] = diff;	/* simula ricorsione */
			positions[recurs] = occsp[cb] + index->bwt_occ[cb];
			assert(occsp[cb] + index->bwt_occ[cb]<index->text_size);
			step_array[recurs] = step;
		}
	}

	if (used != element) 
		return FM_GENERR;
	free (elem_array);
	free (step_array);
	
	return FM_OK;
}


/* Boyer-Moore algorithm to support small files */

	
static void preBmBc(uchar *x, int m, int bmBc[]) {
   int i;
 
   for (i = 0; i < ALPHASIZE; ++i)
      bmBc[i] = m;
   for (i = 0; i < m - 1; ++i)
      bmBc[x[i]] = m - i - 1;
}
 
 
static void suffixes(uchar *x, int m, int *suff) {
   int f, g, i;
   f = 0;	
   suff[m - 1] = m;
   g = m - 1;
   for (i = m - 2; i >= 0; --i) {
      if (i > g && suff[i + m - 1 - f] < i - g)
         suff[i] = suff[i + m - 1 - f];
      else {
         if (i < g)
            g = i;
         f = i;
         while (g >= 0 && x[g] == x[g + m - 1 - f])
            --g;
         suff[i] = f - g;
      }
   }
}
 
static void preBmGs(uchar *x, int m, int bmGs[]) {
   int i, j, suff[m];
 
   suffixes(x, m, suff);
 
   for (i = 0; i < m; ++i)
      bmGs[i] = m;
   j = 0;
   for (i = m - 1; i >= -1; --i)
      if (i == -1 || suff[i] == i + 1)
         for (; j < m - 1 - i; ++j)
            if (bmGs[j] == m)
               bmGs[j] = m - 1 - i;
   for (i = 0; i <= m - 2; ++i)
      bmGs[m - 1 - suff[i]] = m - 1 - i;
}

int fm_boyermoore(fm_index *s, uchar * pattern, uint32_t length, uint32_t ** occ, uint32_t * numocc) {
   
   uint32_t j;
   int i, bmGs[length], bmBc[ALPHASIZE];
   uint32_t alloc = 10;
   *numocc = 0;
   *occ = malloc(sizeof(uint32_t)*alloc);
   if(*occ == NULL) 
	return FM_OUTMEM;   
	
   /* Preprocessing */
   preBmGs(pattern, (int) length, bmGs);
   preBmBc(pattern, (int) length, bmBc);
 
   /* Searching */
   j = 0;
   while (j <=  s->text_size - length) {
      for (i = length - 1; i >= 0 && pattern[i] == s->text[i + j]; --i);
      if (i < 0) {
		 (*numocc)++;
		 if(*numocc == alloc) {
				alloc = MIN(alloc*2,s->text_size);
			 	*occ = realloc(*occ, sizeof(uint32_t)*alloc);
   				if(*occ == NULL) 
						return FM_OUTMEM;   
			}
		 (*occ)[*numocc-1] = j;
         j += bmGs[0];
      }
      else
         j += MAX((uint32_t) bmGs[i], bmBc[s->text[i + j]] - length + 1 + i);
   }
   if(*numocc>0) occ = realloc(*occ, sizeof(uint32_t)*(*numocc));
   else occ = NULL;
   return FM_OK;
}
