#include "fm_index.h"

/* Variabili sono qui solo per poter usare bit_read24 
   e bit_write24 come macro */ 

uint32_t * __Num_Bytes;      /* numero byte letti/scritti */
uchar * __MemAddress;   /* indirizzo della memoria dove scrivere */
int __Bit_buffer_size;  /* number of unread/unwritten bits in Bit_buffer */
uint32_t __pos_read;
uint32_t __Bit_buffer;    



void fm_init_bit_writer(uchar *, uint32_t *);
void fm_init_bit_reader(uchar *);
void fm_bit_flush( void );


/* -----------------------------------------------------------------------------
   Funzioni per leggere/scrivere piu' di 24 bits usando le funzioni fondamentali
   ----------------------------------------------------------------------------- */

void fm_bit_write(int, uint32_t); // (numero bits, intero da codificare) 
void fm_bit_write24(int bits, uint32_t num);
inline void fm_bit_write24inline(int bits, uint32_t num);
int fm_bit_read(int);


/* -----------------------------------------------------------------------------
   Funzioni per leggere/scrivere piu' di 4 Bytes usando le funzioni fondamentali 
   ----------------------------------------------------------------------------- */

void fm_uint_write(uint32_t);
uint32_t fm_uint_read(void);

/* -----------------------------------------------------------------------------
   Encode/Decode di interi con codifica Elias Gamma. 
   n e' codificato con log2(n)-1 bit a 0  + bin(n) 
   -----------------------------------------------------------------------------*/
 /*  
__inline__ void fm_elias_gamma(unsigned int);
__inline__ unsigned int fm_elias_gamma_decode( void );
*/

/* M A C R O  */

#define fm_init_bit_buffer() {__Bit_buffer = 0; __Bit_buffer_size = 0;}

/* -----------------------------------------------------------------------------
   Funzioni per leggere/scrivere meno di 24 bits.
   L'accesso fuori da memoria allocata non e' gestito a questo livello ma e' 
   compito del chiamante. 
   Implementate come macro per migliorare le prestazione.
   ----------------------------------------------------------------------------- */

/* Uso Corretto write:
   BitsDaWrite
   if ( PosizioneinMemori + (log2(BitsDaWrite)+1) < BytesDisponibili ) 
   				ALLORA REALLOCA e RIFAI init_Byte_write;
   bit_write(BitsDaWrite,IntDaCodificare);
*/
							   
/* (numero bits da leggere, int che conterra' il risultato) */
#define fm_bit_read24(__n,__result)	{					\
	uint32_t __t,__u;										\
	assert(__Bit_buffer_size<8);						\
	assert(__n>0 && __n<=24);							\
	/* --- read groups of 8 bits until size>= n --- */ 	\
  	while(__Bit_buffer_size < __n) {					\
		__t = (uint32_t) *__MemAddress;					\
		__MemAddress++; (*__Num_Bytes)++;				\
    	__Bit_buffer |= (__t << (24-__Bit_buffer_size));\
    	__Bit_buffer_size += 8;}						\
  	/* ---- write n top bits in u ---- */				\
  	__u = __Bit_buffer >> (32-__n);						\
  	/* ---- update buffer ---- */						\
  	__Bit_buffer <<= __n;								\
  	__Bit_buffer_size -= __n;							\
  	__result = ((int)__u);}					   

	
#define BIT_READ(__bitz, __result) {					\
  uint32_t __ur = 0;										\
  int __ir;												\
  assert(__bitz <= 32);									\
  if (__bitz > 24){										\
	fm_bit_read24((__bitz-24), __ir);					\
    __ur =  __ir<<24;									\
	fm_bit_read24(24,__ir);								\
    __ur |= __ir;										\
    __result = __ur;									\
  } else {												\
    fm_bit_read24(__bitz,__ir);							\
	__result = __ir;									\
  }}
	
									
/* -----------------------------------------------------------------------------
   Codifica di Interi con una parte fissa ed una variabile. 
   Questa codifica non so se esisteva ma ha prestazioni buone rispetto a write7x8
   Anche queste come macro perche' molto utilizzate in ogni parte di FM-Index
   ------------------------------------------------------------------------------
   */

#define fm_integer_encode(__numero, __log2log2maxvalue) {{		\
	uint32_t  __k;										\
	uint32_t __i = 0;									\
	assert(__log2log2maxvalue<6);							\
	switch ( __numero ){									\
		/* primi 4 casi speciali */							\
		case 0: fm_bit_write24(__log2log2maxvalue,0); 			\
				break;										\
		case 1: fm_bit_write24(__log2log2maxvalue,1); 			\
				break;										\
		case 2: fm_bit_write24((__log2log2maxvalue+1),4);		\
				break;										\
		case 3: fm_bit_write24((__log2log2maxvalue+1),5); 		\
				break;										\
	/*	Altri casi. Si calcola i = log2(occ) __numero		\
		di bit per rappresentare occ. si calcola  k=		\
		occ-2^i cioe' la distanza dalla potenza di due		\ 
		inferiore. Scrive i su log2log2BuckSize bits		\
		e k su i-1 bits indica __numero in piu' alla		\
	 	potenza di due precedente 						*/	\
		default: {											\
			uint32_t __pow = 1;									\
			/* 	calcola distanza dalla potenza di due 		\	
			 	precedente i indica l'esponente di 			\
				tale potenza */ 							\
			__i = 3;										\
			__pow = 4;										\
			while(__pow <= __numero){						\
    			__pow= __pow<<1;							\
    			__i = __i+1;								\
  			}												\
			__i--; __pow = __pow>>1;						\
			__k = __numero - __pow; 						\
			fm_bit_write24(__log2log2maxvalue,__i);			\
			fm_bit_write24(__i-1,__k);							\
			}}}}
			
uint32_t fm_integer_decode(unsigned short int);
