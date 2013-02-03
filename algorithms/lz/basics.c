
// Basics

// #include "basics.h" included later to avoid macro recursion for malloc
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>

	// Memory management

#ifdef MEMCTRL
int account = 1;
int currmem = 0;
int newmem;
int maxmem = 0;
#endif

void *Malloc(int n)
 { 
    void *p;
    if (n == 0) return NULL;
#ifndef MEMCTRL
    p = (void*) malloc(n);
#else
    p = (void*) (malloc(n+sizeof(int))+sizeof(int));
#endif
    if (p == NULL) {
       fprintf(stderr,"Could not allocate %i bytes\n",n);
       exit(1);
    }
#ifdef MEMCTRL
    *(((int*)p)-1) = n;
    if (account) {
       newmem = currmem+n;
       if (newmem > maxmem) maxmem = newmem;
       if (currmem/1024 != newmem/1024)
          printf("Memory: %i Kb, maximum: %i Kb\n",newmem/1024,maxmem/1024);
       currmem = newmem
    }
#endif
    return p;
 }

void Free(void *p)
 { 
#ifndef MEMCTRL
    if (p) free(p);
#else
    if (!p) return;
    if (account) {
       newmem = currmem - *(((int*)p)-1);
       free((void*)(((int)p)-sizeof(int)));
       if (currmem/1024 != newmem/1024)
          printf("Memory: %i Kb, maximum: %i Kb\n",newmem/1024,maxmem/1024);
       currmem = newmem;
    }
#endif
 }

void *Realloc(void *p, int n)
 { 
    if (p == NULL) return Malloc (n);
    if (n == 0) { Free(p); return NULL; }
#ifndef MEMCTRL
    p = (void*) realloc (p,n);
#else
    if (account)
       newmem = currmem - *(((int*)p)-1);
    p = (void*) (realloc ((void*)(((int)p)-sizeof(int)),n+sizeof(int))+sizeof(int));
    *(((int*)p)-1) = n;
#endif
    if (p == NULL) {
       fprintf(stderr,"Could not allocate %i bytes\n",n);
       exit(1);
    }
#ifdef MEMCTRL
    if (account) {
       newmem = newmem + n;
       if (newmem > maxmem) maxmem = newmem;
       if (currmem/1024 != newmem/1024)
          printf("Memory: %i Kb, maximum: %i Kb\n",newmem/1024,maxmem/1024);
       currmem = newmem;
    }
#endif
    return p;
 }

#include "basics.h"

        // bits needed to represent a number between 0 and n

uint bits(uint n)
 { 
    uint b = 0;
    while (n)
    { b++; n >>= 1; }
    return b;
 }

        // returns e[p..p+len-1], assuming len <= W

uint bitget(uint *e, uint p, register uint len)
 { 
    register uint i=p/W, j=p-W*i, answ;
    if (j+len <= W)
       answ = (e[i] << (W-j-len)) >> (W-len);
    else {
       answ = e[i] >> j;
       answ = answ | ((e[i+1]<<(W-j-len))>>(W-len));
    }
    return answ;
 }

  	// writes e[p..p+len-1] = s, len <= W

void bitput(register uint *e, register uint p, 
            register uint len, register uint s)
 { 
    e += p >> bitsW; p &= (1<<bitsW)-1;
    if (len == W) {
       *e |= (*e & ((1<<p)-1)) | (s << p);
       if (!p) return;
       e++;
       *e = (*e & ~((1<<p)-1)) | (s >> (W-p));
    }
    else { 
       if (p+len <= W) {
          *e = (*e & ~(((1<<len)-1)<<p)) | (s << p);
          return;
       }
       *e = (*e & ((1<<p)-1)) | (s << p);
       e++; len -= W-p;
       *e = (*e & ~((1<<len)-1)) | (s >> (W-p));
    }
 }
