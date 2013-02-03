#include "fm_mng_bits.h"

/*
 * Funzioni per la scrittura di bit in memoria 
 */

/*
 * Mem e' il puntatore al primo byte da scrivere e pos_mem riporta il
 * numero di byte scritti. Non Inizializza Buffer. 
 */

void
fm_init_bit_writer (uchar * mem, uint32_t * pos_mem)
{
	__MemAddress = mem;
	__Num_Bytes = pos_mem;
	fm_init_bit_buffer ();
}

void
fm_init_bit_reader (uchar * mem)
{
	__MemAddress = mem;
	__pos_read = 0;
	__Num_Bytes = &__pos_read;
	fm_init_bit_buffer ();
}

/*
 * -----------------------------------------------------------------------------
 * Funzioni per leggere/scrivere piu' di 24 bits usando le funzioni fondamentali
 * ----------------------------------------------------------------------------- 
 */

// ****** Write in Bit_buffer n bits taken from vv (possibly n > 24) 
void
fm_bit_write (int n, uint32_t vv)
{
	uint32_t v = (uint32_t) vv;

	assert (n <= 32);
	if (n > 24)
	{
		fm_bit_write24 ((n - 24), (v >> 24 & 0xffL));
		fm_bit_write24 (24, (v & 0xffffffL));
	}
	else
	{
		fm_bit_write24 (n, v);
	}
}

// ****** Read n bits from Bit_buffer 
int
fm_bit_read (int n)
{
	uint32_t u = 0;
	int i;
	assert (n <= 32);
	if (n > 24)
	{
		fm_bit_read24 ((n - 24), i);
		u = i << 24;
		fm_bit_read24 (24, i);
		u |= i;
		return ((int) u);
	}
	else
	{
		fm_bit_read24 (n, i);
		return i;
	}
}

void fm_bit_write24(int bits, uint32_t num) {
					
  	assert(__Bit_buffer_size<8); 
  	assert(bits>0 && bits<=24);
  	assert( num < 1u << bits );	
	__Bit_buffer_size += (bits); 
  	__Bit_buffer |= (num << (32 - __Bit_buffer_size));
	while (__Bit_buffer_size >= 8) {
		*__MemAddress = (__Bit_buffer>>24);
		__MemAddress++;
    	(*__Num_Bytes)++;
    	__Bit_buffer <<= 8;
    	__Bit_buffer_size -= 8;
		} 
}

/*
 * -----------------------------------------------------------------------------
 * Funzioni per leggere/scrivere piu' di 4 Bytes usando le funzioni fondamentali
 * ----------------------------------------------------------------------------- 
 */

// ****** Write in Bit_buffer four bytes 
void
fm_uint_write (uint32_t uu)
{
	uint32_t u = (uint32_t) uu;
	fm_bit_write24 (8, ((u >> 24) & 0xffL));
	fm_bit_write24 (8, ((u >> 16) & 0xffL));
	fm_bit_write24 (8, ((u >> 8) & 0xffL));
	fm_bit_write24 (8, (u & 0xffL));

}

// ***** Return 32 bits taken from Bit_buffer
uint32_t
fm_uint_read (void)
{

	uint32_t u;
	int i;
	fm_bit_read24 (8, i);
	u = i << 24;
	fm_bit_read24 (8, i);
	u |= i << 16;
	fm_bit_read24 (8, i);
	u |= i << 8;
	fm_bit_read24 (8, i);
	u |= i;
	return ((int) u);
}

/*
 * -----------------------------------------------------------------------------
 * Funzione Bit Flush.
 * ----------------------------------------------------------------------------- 
 */

// ***** Complete with zeroes the first byte of Bit_buffer 
// ***** This way, the content of Bit_buffer is entirely flushed out

void
fm_bit_flush (void)
{
	if (__Bit_buffer_size != 0)
		fm_bit_write24 ((8 - (__Bit_buffer_size % 8)), 0);	// pad with zero !
}

#if 0
void
elias_gamma (unsigned int j)
{
	j++;
	if (j < 2)
	{
		fm_bit_write24 (1, 1);
		return;
	}
	if (j < 4)
	{
		fm_bit_write24 (3, j);
		return;
	}
	if (j < 8)
	{
		fm_bit_write24 (5, j);
		return;
	}
	if (j < 16)
	{
		fm_bit_write24 (7, j);
		return;
	}
	if (j < 32)
	{
		fm_bit_write24 (9, j);
		return;
	}
	if (j < 64)
	{
		fm_bit_write24 (11, j);
		return;
	}
	if (j < 128)
	{
		fm_bit_write24 (13, j);
		return;
	}
	if (j < 256)
	{
		fm_bit_write24 (15, j);
		return;
	}
	if (j < 512)
	{
		fm_bit_write24 (17, j);
		return;
	}
	if (j < 1024)
	{
		fm_bit_write24 (19, j);
		return;
	}
	if (j < 2048)
	{
		fm_bit_write24 (21, j);
		return;
	}
	if (j < 4096)
	{
		fm_bit_write24 (23, j);
		return;
	}
	if (j < 8192)
	{
		fm_bit_write (25, j);
		return;
	}
	if (j < 16384)
	{
		fm_bit_write (27, j);
		return;
	}
	if (j < 32768)
	{
		fm_bit_write (29, j);
		return;
	}
	if (j < 65536)
	{
		fm_bit_write (31, j);
		return;
	}
}

unsigned int
elias_gamma_decode ()
{
	int bits, bit, number;
	number = 1;
	bits = 0;

	fm_bit_read24 (1, bit);
	while (!bit)
	{
		number *= 2;
		fm_bit_read24 (1, bit);
		bits++;
	}
	if (bits)
	{
		fm_bit_read24 (bits, bit);	// corretto con Buckets di dimensione
		// <4096
		number += bit - 1;
	}
	else
		return 0;

	return number;
}

#endif 

uint32_t
fm_integer_decode (unsigned short int headbits)
{
	int k, i;
	fm_bit_read24 (headbits, i);
	/*
	 * casi base speciali 
	 */

	if (i == 0)
		return 0;
	if (i == 1)
		return 1;
	if (i == 2)
	{
		fm_bit_read24 (1, i);
		k = 2 + i;
		return k;
	}
	/*
	 * i indica il numero di bits che seguono num = 2^i + k 
	 */
	i--;
	/*
	 * bit_read24 e' corretta fino a superbuckets di dimensione inferiore
	 * a 16384*1024. E' comunque meglio utilizzare questa perche molto
	 * piu' veloce 
	 */
	fm_bit_read24 (i, k);
	return ((1 << i) + k);
}
