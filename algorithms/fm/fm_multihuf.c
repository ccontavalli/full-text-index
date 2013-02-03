/*
 * >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
 bwt-based
 * compression and indexing
 
 hufbzip.c
 compression and decompression
 * using multiple huffman tables
 as in the bzip2 compressors.
 
 P.
 * Ferragina & G. Manzini, 10 June 2000
 * >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> 
 */  
#include "fm_common.h"
#include <stdlib.h>
#include "fm_mng_bits.h"
	

#define BZ_RUNA 0
#define BZ_RUNB 1
#define BZ_N_GROUPS 6
#define BZ_G_SIZE   50
#define BZ_N_ITERS  4
#define BZ_LESSER_ICOST  0
#define BZ_GREATER_ICOST 15
#define BZ_MAX_ALPHA_SIZE 258
#define BZ_MAX_CODE_LEN    23

extern int __Fm_Verbose;


#define True 1
typedef unsigned short uint16;


	/*
	 * -------- arrays used by multihuf ------------ 
	 */ 
	uchar huf_len[BZ_N_GROUPS][BZ_MAX_ALPHA_SIZE];	// coding and
							// decoding
int huf_code[BZ_N_GROUPS][BZ_MAX_ALPHA_SIZE];	// coding
int rfreq[BZ_N_GROUPS][BZ_MAX_ALPHA_SIZE];	// coding
int mtf_freq[BZ_MAX_ALPHA_SIZE];	// coding

uchar huf_minLens[BZ_N_GROUPS];	// decoding
int huf_limit[BZ_N_GROUPS][BZ_MAX_CODE_LEN];	// decoding
int huf_base[BZ_N_GROUPS][BZ_MAX_CODE_LEN];	// decoding
int huf_perm[BZ_N_GROUPS][BZ_MAX_ALPHA_SIZE];	// decoding

	/*
	 ********************************************************************
	 rle+compression of a string using Huffman with multiple tables 
	 input
	 int   len         size of mtf sequence
	 uchar *in         input mtf sequence
	 int   alpha_size   size of the alphabet
	 output
	 the compressed string is written in the output file 
	 ******************************************************************* */ 
int fm_multihuf_compr (uchar * in, int len, int alpha_size) 
{
	
void hbMakeCodeLengths (uchar * len, int *freq,
				 int alphaSize, int maxLen);
	
void hbAssignCodes (int *code, uchar * len, int minL,
			     int maxL, int asize);
	
int v, t, i, j, gs, ge, totc, bt, bc, iter;
	
int nSelectors, minLen, maxLen, new_len;
	
int nGroups;
	
uint16 cost[BZ_N_GROUPS];
	
int fave[BZ_N_GROUPS];
	
uint16 * mtfv;
	
uchar * selector;
	

mtfv = (uint16 *) malloc ((len + 1) * sizeof (uint16));
	
if (mtfv == NULL)
		return FM_OUTMEM;
	

		// encode sequences of 0's using 1-2 coding
		new_len = 0;
	
	{
		
int c, z = 0;
		
for (i = 0; i < len; i++)
		{
			
c = in[i];
			
assert (c < alpha_size);
			
if (c == 0)
				z++;
			
			else
			{
				
					/*
					 * ----- check if there are pending zeores ---- 
					 */ 
					if (z > 0)
				{	// 1-2 encoding 
					z++;	// write z+1 in binary least sign bit
					// first 
					while (z > 1)
					{
						
mtfv[new_len++] =
							(z & 1) ? BZ_RUNB :
							BZ_RUNA;
						
z = z >> 1;
					
}
					
z = 0;
				
}
				
mtfv[new_len++] = (uint16) c + 1;
			
}
		
}
		
			// ---- there could be some pending zeroes
			if (z > 0)
		{
			
z++;	// write z+1 in binary least sign bit
			// first 
			while (z > 1)
			{
				
mtfv[new_len++] =
					(z & 1) ? BZ_RUNB : BZ_RUNA;
				
z = z >> 1;
			
}
		
}
	
}
	
mtfv[new_len++] = alpha_size + 1;	// end of block
	alpha_size += 2;	// 2 new symbols have been used
	if (__Fm_Verbose > 2)
		
fprintf (stderr, "block size after MTF & 1-2 coding: %d\n",
			  new_len);
	

		// init mtf_freq[]
		for (i = 0; i < alpha_size; i++)
		mtf_freq[i] = 0;
	
for (i = 0; i < new_len; i++)
		mtf_freq[mtfv[i]]++;
	
		// init huf_len[][]
		for (t = 0; t < BZ_N_GROUPS; t++)
		
for (v = 0; v < alpha_size; v++)
			
huf_len[t][v] = BZ_GREATER_ICOST;
	
		// alloc selector[]
		selector =
		(uchar *) malloc ((1 + new_len / BZ_G_SIZE) * sizeof (uchar));
	
if (selector == NULL)
		return FM_OUTMEM;
	

   /*--- Decide how many coding tables to use ---*/ 
		assert (new_len > 0);
	
if (new_len < 200)
		nGroups = 2;
	else
	
if (new_len < 600)
		nGroups = 3;
	else
	
if (new_len < 1200)
		nGroups = 4;
	else
	
if (new_len < 2400)
		nGroups = 5;
	else
		
nGroups = 6;
	

   /*--- Generate an initial set of coding tables ---*/ 
		// each table uses BZ_LESSER_ICOST for a group of consecutive 
		// chars (gs to ge) and BZ_GREATER_ICOST for the others chars
	{
		
int nPart, remF, tFreq, aFreq;
		

nPart = nGroups;
		
remF = new_len;
		
gs = 0;
		
while (nPart > 0)
		{
			
tFreq = remF / nPart;
			
ge = gs - 1;
			
aFreq = 0;
			
while (aFreq < tFreq && ge < alpha_size - 1)
			{
				
ge++;
				
aFreq += mtf_freq[ge];
			
}
			
if (ge > gs 
			     &&nPart != nGroups && nPart != 1 
			     &&((nGroups - nPart) % 2 == 1))
			{
				
aFreq -= mtf_freq[ge];
				
ge--;
			
}
			
if (__Fm_Verbose > 2)
				
fprintf (stderr,
					  "      initial group %d, [%d .. %d], "
					  
 "has %d syms (%4.1f%%)\n", 
nPart,
					  gs, ge, aFreq,
					  
(100.0 * (float) aFreq) /
					  (float) (new_len));
			
for (v = 0; v < alpha_size; v++)
				
if (v >= gs && v <= ge)
					
huf_len[nPart - 1][v] =
						BZ_LESSER_ICOST;
				else
					
huf_len[nPart - 1][v] =
						BZ_GREATER_ICOST;
			
nPart--;
			
gs = ge + 1;
			
remF -= aFreq;
		
}
	
}
	

   /*--- 
      Iterate up to BZ_N_ITERS times to improve the tables.
   ---*/ 
		for (iter = 0; iter < BZ_N_ITERS; iter++)
	{
		
for (t = 0; t < nGroups; t++)
			fave[t] = 0;
		
for (t = 0; t < nGroups; t++)
			
for (v = 0; v < alpha_size; v++)
				
rfreq[t][v] = 0;
		
nSelectors = 0;
		
totc = 0;
		
gs = 0;
		
while (True)
		{
			
				/*
				 * Set group start & end marks. -- 
				 */ 
				if (gs >= new_len)
				break;
			
ge = gs + BZ_G_SIZE - 1;	// size is at most BZ_G_SIZE
			if (ge >= new_len)
				ge = new_len - 1;
			
	 /*-- 
            Calculate the cost of this group as coded
            by each of the coding tables.
         --*/ 
				for (t = 0; t < nGroups; t++)
				cost[t] = 0;
			
if (nGroups == 6)
			{
				
register uint16 cost0,
					cost1, cost2, cost3, cost4, cost5;
				
cost0 = cost1 = cost2 = cost3 = cost4 =
					cost5 = 0;
				
for (i = gs; i <= ge; i++)
				{
					
uint16 icv = mtfv[i];
					
cost0 += huf_len[0][icv];
					
cost1 += huf_len[1][icv];
					
cost2 += huf_len[2][icv];
					
cost3 += huf_len[3][icv];
					
cost4 += huf_len[4][icv];
					
cost5 += huf_len[5][icv];
				
}
				
cost[0] = cost0;
				cost[1] = cost1;
				cost[2] = cost2;
				
cost[3] = cost3;
				cost[4] = cost4;
				cost[5] = cost5;
			
}
			else
			{
				
for (i = gs; i <= ge; i++)
				{
					
uint16 icv = mtfv[i];
					
for (t = 0; t < nGroups; t++)
						cost[t] += huf_len[t][icv];
				
}
			
}
			
	 /*-- 
            Find the coding table which is best for this group,
            and record its identity in the selector table.
         --*/ 
				bc = 999999999;
			bt = -1;
			
for (t = 0; t < nGroups; t++)
				
if (cost[t] < bc)
				{
					bc = cost[t];
					bt = t;
				};
			
totc += bc;
			
fave[bt]++;
			
selector[nSelectors++] = bt;
			
	 /*-- 
            Increment the symbol frequencies for the selected table.
          --*/ 
				for (i = gs; i <= ge; i++)
				
rfreq[bt][mtfv[i]]++;
			
gs = ge + 1;	// consider next group 
		}
		
if (__Fm_Verbose > 2)
		{
			
fprintf (stderr,
				  "      pass %d: size is %d, grp uses are ",
				  
iter + 1, totc / 8);
			
for (t = 0; t < nGroups; t++)
				
fprintf (stderr, "%d ", fave[t]);
			
fprintf (stderr, "\n");
		
}
		
      /*--
        Recompute the tables based on the accumulated frequencies.
      --*/ 
			for (t = 0; t < nGroups; t++)
			
hbMakeCodeLengths (&(huf_len[t][0]), &(rfreq[t][0]),
					    alpha_size, 20);
	
}
	

   /*--- Assign actual codes for the tables. --*/ 
		for (t = 0; t < nGroups; t++)
	{
		
minLen = 32;
		
maxLen = 0;
		
for (i = 0; i < alpha_size; i++)
		{
			
if (huf_len[t][i] > maxLen)
				maxLen = huf_len[t][i];
			
if (huf_len[t][i] < minLen)
				minLen = huf_len[t][i];
		
}
		
assert (!(maxLen > 20));
		
assert (!(minLen < 1));
		
hbAssignCodes (&(huf_code[t][0]), &(huf_len[t][0]), 
minLen,
				maxLen, alpha_size);
	
}
	

		// write coding tables (i.e codeword length).
		assert (nGroups < 8);
	
fm_bit_write (3, nGroups);
	
for (t = 0; t < nGroups; t++)
	{
		
int curr = huf_len[t][0];
		
fm_bit_write (5, curr);
		
for (i = 0; i < alpha_size; i++)
		{
			
while (curr < huf_len[t][i])
			{
				fm_bit_write (2, 2);
				curr++;	/* 10 */
			};
			
while (curr > huf_len[t][i])
			{
				fm_bit_write (2, 3);
				curr--;	/* 11 */
			};
			
fm_bit_write (1, 0);
		
}
	
}
	

   /*--- write selectors and compressed data ---*/ 
	{
		
int sel = 0;
		
uchar pos[BZ_N_GROUPS], ll_i, tmp2, tmp;
		
for (i = 0; i < nGroups; i++)
			pos[i] = i;
		
gs = 0;
		
while (True)
		{
			
if (gs >= new_len)
				break;
			
ge = gs + BZ_G_SIZE - 1;
			
if (ge >= new_len)
				ge = new_len - 1;	// establish group boundaries
			assert (selector[sel] < nGroups);
			
			{
				
ll_i = selector[sel];	// get mtf rank for selector
				j = 0;
				
tmp = pos[j];
				
while (ll_i != tmp)
				{
					
j++;
					
tmp2 = tmp;
					tmp = pos[j];
					pos[j] = tmp2;
				
};
				
pos[0] = tmp;
				
fm_bit_write (j + 1, 1);	// write selector mtf rank in
				// unary 
			}
			
for (i = gs; i <= ge; i++)
			{
				
assert (mtfv[i] < alpha_size);
				
fm_bit_write (huf_len[selector[sel]][mtfv[i]],
					    
huf_code[selector[sel]][mtfv
								     [i]]);
			
}
			
gs = ge + 1;
			
sel++;
		
}
		
assert (sel == nSelectors);
	
}
	
free (selector);
	
free (mtfv);
	
return FM_OK;

}



	/*
	 *********************************************************
	 decode a unary code read from Infile. the output is the
	 # of zeroes we see before we see a 1 
	 1 --> 0
	 01 --> 1
	 001 --> 2  
	 etc.
	 ********************************************************* */ 
	 __inline__ static  int
decode_unary (void) 
{
	
int t, i = 0;
	

	do
	{
		
fm_bit_read24 (1, t);
		
if (t != 0)
			break;
		
i++;
	
}
	while (1);
	
return i;

}




	/*
	 ********************************************************************
	 this procedures reads a bucket from file decodes it and writes 
	 it to dest[] (which should be of the appropriate size).
	 The decoding stops when an EOB is encountered or >= limit bytes
	 have been decoded. that is, when >= limit chars have been written
	 to dest the decompression terminates and the procedure returns.
	 Note that more than limit chars can be
	 decoded, so dest() should be large enough to contain the 
	 complete bucket. the procedure returns the number of chars written to
	 dest (which can be less than limit (if a EOB is encountered)) 
	 ******************************************************************** */ 
	int
fm_multihuf_decompr (uchar * dest, int alpha_size, int limit) 
{
	
void hbCreateDecodeTables (int *limit, int *base, int *perm,
				    uchar * length, 
int minLen,
				    int maxLen, int alphaSize);
	

//O__inline__ int decode_unary (void);
	
int t, i, j, minLen, maxLen, len, nGroups;
	

alpha_size += 2;	// we temporarily use a larger alphabet
	
		// get number of groups
		fm_bit_read24 (3, nGroups);
	
  /*--- get the coding tables ---*/ 
	{
		
int curr, uc;
		

for (t = 0; t < nGroups; t++)
		{
			
fm_bit_read24 (5, curr);
			
for (i = 0; i < alpha_size; i++)
			{
				
while (True)
				{
					
if (curr < 1 || curr > 20)
						
return FM_DECERR;
					
fm_bit_read24 (1, uc);
					
if (uc == 0)
						break;
					
fm_bit_read24 (1, uc);
					
if (uc == 0)
						curr++;
					else
						curr--;
				
}
				
huf_len[t][i] = curr;
			
}
		
}
		

    /*--- Create the Huffman decoding tables ---*/ 
			for (t = 0; t < nGroups; t++)
		{
			
minLen = 32;
			
maxLen = 0;
			
for (i = 0; i < alpha_size; i++)
			{
				
if (huf_len[t][i] > maxLen)
					maxLen = huf_len[t][i];
				
if (huf_len[t][i] < minLen)
					minLen = huf_len[t][i];
			
}
			
hbCreateDecodeTables (
&(huf_limit[t][0]),
					       
&(huf_base[t][0]),
					       
&(huf_perm[t][0]),
					       
&(huf_len[t][0]), 
minLen,
					       maxLen, alpha_size 
);
			
huf_minLens[t] = minLen;
		
}
	
}
	

   /*------- uncompress data -------*/ 
	{
		
int rle_sofar, run, next, rank, gSel, to_be_read;
		
int zn, zj, zvec, *gLimit, *gPerm, *gBase;
		
uchar pos[BZ_N_GROUPS], gMinlen = 0;
		

gLimit = gPerm = gBase = NULL;	// to avoid annoying
		// compiler warnings
		for (i = 0; i < nGroups; i++)
			pos[i] = i;
		
len = 0;
		rle_sofar = 0;
		
to_be_read = 0;
		
while (True)
		{
			
if (to_be_read == 0)
			{
				
to_be_read = BZ_G_SIZE;
				
rank = decode_unary ();	// get mtf rank of new group
				assert (rank < nGroups);
				
gSel = pos[rank];
				
for (j = rank; j > 0; j--)
					pos[j] = pos[j - 1];
				
pos[0] = (uchar) gSel;
				
					// get tables for this group
					gMinlen = huf_minLens[gSel];
				
gLimit = &(huf_limit[gSel][0]);
				
gPerm = &(huf_perm[gSel][0]);
				
gBase = &(huf_base[gSel][0]);
			
}
			
to_be_read--;
			
				// get next huffman encoded char
				zn = gMinlen;
			
				// zvec = bit_read(zn);
				fm_bit_read24 (zn, zvec);
			
while (zvec > gLimit[zn])
			{
				
zn++;
				
					// zj=bit_read(1);
					fm_bit_read24 (1, zj);
				
zvec = (zvec << 1) | zj;
			
};
			
next = gPerm[zvec - gBase[zn]];
			
				// decode next
				assert (next < alpha_size);
			
if (next == alpha_size - 1)
				break;	// end of bucket
			if (next == BZ_RUNA)
			{	// 0 of a 1-2 encoding
				run = 1 << rle_sofar;
				
for (j = 0; j < run; j++)
					dest[len++] = 0;
				
rle_sofar++;
			
}
			
			else if (next == BZ_RUNB)
			{	// 1 of a 1-2 encoding 
				run = 2 << rle_sofar;
				
for (j = 0; j < run; j++)
					dest[len++] = 0;
				
rle_sofar++;
			
}
			
			else
			{
				
dest[len++] = next - 1;
				
rle_sofar = 0;
			
}
			
if (len >= limit)
				return len;	// only line added to stop when >= limit
		}		// chars have been decoded
	}
	
return len;

}




	/*
	 ************************************************************
	 uncompress the bucket which starts at the current position
	 of infile. the bucket is "len" bytes long and should
	 be written in array dest[]
	 ************************************************************ */ 
	int
fm_uncompress_bucket_multihuf (uchar * dest, int len, int alpha_size)
{
	

int fm_multihuf_decompr (uchar *, int, int);
	
int k, j, i, aux_len, rank, local_alpha_size, next;
	
uchar mtf[256], inv_local_map[256];
	

		/*
		 * ---------- read local boolean map and compute inverse map
		 * ------ 
		 */ 
		local_alpha_size = 0;
	
for (k = 0; k < alpha_size; k++)
		
if (fm_bit_read (1))
			
inv_local_map[local_alpha_size++] = k;
	

if (local_alpha_size == 1)
	{			// use this when you have only one 
		// char in bucket
		for (i = 0; i < len; i++)
			dest[i] = inv_local_map[0];
		
return FM_OK;
	
}
	

		/*
		 * Set initial MTF 
		 */ 
		for (k = 0; k < local_alpha_size; k++)
	{
		
mtf[k] = k;
	
}
	

		/*
		 * ------- decode multiple huffman codes ----- 
		 */ 
		aux_len = fm_multihuf_decompr (dest, local_alpha_size, len);
	
if (aux_len != len)
		
return FM_DECERR;
	

		/*
		 * ------ decode *inplace* mtf_seq -------------------- 
		 */ 
		for (j = 0; j < len; j++)
	{
		
rank = dest[j];
		
assert (rank < local_alpha_size);
		
next = mtf[rank];	// decode mtf rank 
		for (i = rank; i > 0; i--)	// update mtf list
			mtf[i] = mtf[i - 1];
		
mtf[0] = next;	// update mtf[0]
		dest[j] = inv_local_map[next];	// apply invamp
		assert (dest[j] < alpha_size);
	
}
	
return FM_OK;

}
