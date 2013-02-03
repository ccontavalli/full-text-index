#include "fm_common.h"
#include "ds_ssort.h"
#include "fm_errors.h"

/*
 * Compute # bits to represent u. Per calcolare log2(u) intero superiore
 * devo passargli (u-1) 
 */
int
int_log2 (int u)
{
	/*
	 * codifica con if i casi piu frequenti un if costa meno di una
	 * moltiplicazione 
	 */

	if (u < 2)
		return 1;
	if (u < 4)
		return 2;
	if (u < 8)
		return 3;	// da 4 a 7
	if (u < 16)
		return 4;
	if (u < 32)
		return 5;
	if (u < 64)
		return 6;
	if (u < 128)
		return 7;
	if (u < 256)
		return 8;
	if (u < 512)
		return 9;
	if (u < 1024)
		return 10;
	if (u < 2048)
		return 11;
	if (u < 4096)
		return 12;
	if (u < 8192)
		return 13;
	if (u < 16384)
		return 14;
	if (u < 32768)
		return 15;
	if (u < 65536)
		return 16;
	if (u < 131072)
		return 17;
	if (u < 262144)
		return 18;
	if (u < 524288)
		return 19;
	if (u < 1048576)
		return 20;
	int i = 20;
	int r = 1048575;

	while (r < u)
	{
		r = 2 * r + 1;
		i = i + 1;
	}
	return i;
}

int
int_pow2 (int u)		// compute 2^u
{
	int i, val;

	for (i = 0, val = 1; i < u; i++)
		val *= 2;

	assert (i == u);
	return val;

}
