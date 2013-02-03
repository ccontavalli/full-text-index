/* bitarray.cpp
   Copyright (C) 2005, Rodrigo Gonzalez, all rights reserved.

   New RANK, SELECT, SELECT-NEXT and SPARSE RANK implementations.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/

#include "bitarray.h"
#include "assert.h"
#include "math.h"
#include <sys/types.h>

/////////////
//Rank(B,i)//
/////////////
//This Class use a superblock size of 256 bits
//and a block size of 32 bits also
//we use a 32 bit to represent Rs and
//we use a 8 bit to represent Rb

// The bitarray goes form 0..n-1
BitRankF::BitRankF(ulong *bitarray, ulong n, bool owner) {
  data=bitarray;
  this->owner = owner;
  this->n=n;
  b=32; // b is a word
  factor=8; // 8 word in 256 bits
  s=b*factor;
  ulong aux=(n+1)%W;
  if (aux != 0)
    integers = (n+1)/W+1;
  else
    integers = (n+1)/W;
  BuildRank();
}

BitRankF::~BitRankF() {
  delete [] Rs;
  delete [] Rb;
  if (owner) delete [] data;
}

ulong BitRankF::SpaceRequirementInBits() {
  return (owner?n:0)+(n/s)*sizeof(ulong)*8 +(n/b)*sizeof(uchar)*8+sizeof(BitRankF)*8; 
}
//Build the rank (blocks and superblocks)
void BitRankF::BuildRank(){
  ulong num_sblock = n/s;
  ulong num_block = n/b;
  Rs = new ulong[num_sblock+1];//+1 we add the 0 pos
  Rb = new uchar[num_block+1];//+1 we add the 0 pos
  ulong j;
  Rs[0]=0;
  for (j=1;j<=num_sblock;j++)
    Rs[j]=BuildRankSub((j-1)*factor,factor)+Rs[j-1];

  Rb[0]=0;
  for (ulong k=1;k<=num_block;k++) {
    j = k / factor;
    Rb[k]=BuildRankSub(j*factor,k%factor);
  }
}

int BitRankF::save(FILE *f) {
  if (f == NULL) return 20;
  if (fwrite (&n,sizeof(ulong),1,f) != 1) return 21;
  if (fwrite (data,sizeof(ulong),n/W+1,f) != n/W+1) return 21;
  if (fwrite (Rs,sizeof(ulong),n/s+1,f) != n/s+1) return 21;
  if (fwrite (Rb,sizeof(uchar),n/b+1,f) != n/b+1) return 21;
  return 0;
}

int BitRankF::load(FILE *f) {
  if (f == NULL) return 23;
  if (fread (&n,sizeof(ulong),1,f) != 1) return 25;
  b=32; // b is a word
  factor=8; // 8 word in 256 bits
  s=b*factor;
  ulong aux=(n+1)%W;
  if (aux != 0)
    integers = (n+1)/W+1;
  else
    integers = (n+1)/W;
  data= new ulong[n/W+1];
  if (!data) return 1;
  if (fread (data,sizeof(ulong),n/W+1,f) != n/W+1) return 25;
  this->owner = true;
  Rs= new ulong[n/s+1];
  if (!Rs) return 1;
  if (fread (Rs,sizeof(ulong),n/s+1,f) != n/s+1) return 25;
  Rb= new uchar[n/b+1];
  if (!Rb) return 1;
  if (fread (Rb,sizeof(uchar),n/b+1,f) != n/b+1) return 25;
  return 0;
}

BitRankF::BitRankF(FILE *f, int *error) {
  *error = BitRankF::load(f);
}

ulong BitRankF::BuildRankSub(ulong ini,ulong bloques){
  ulong rank=0,aux;
  for(ulong i=ini;i<ini+bloques;i++) {
    if (i < integers) {
      aux=data[i];
      rank+=popcount(aux);
    }
  }
  return rank; //return the numbers of 1's in the interval
}


//this rank ask from 0 to n-1
ulong BitRankF::rank(ulong i) {
  ++i; // the following gives sum of 1s before i
  return Rs[i>>8]+Rb[i>>5]
    +popcount(data[i>>5] & ((1<<(i & mask31))-1));
}

ulong BitRankF::prev(ulong start) {
      // returns the position of the previous 1 bit before and including start.
      // tuned to 32 bit machine

      ulong i = start >> 5;
      int offset = (start % W);
      ulong answer = start;
      ulong val = data[i] << (Wminusone-offset);

      if (!val) { val = data[--i]; answer -= 1+offset; }

      while (!val) { val = data[--i]; answer -= W; }

      if (!(val & 0xFFFF0000)) { val <<= 16; answer -= 16; }
      if (!(val & 0xFF000000)) { val <<= 8; answer -= 8; }

      while (!(val & 0x80000000)) { val <<= 1; answer--; }
      return answer;
}

ulong BitRankF::select(ulong x) {
  // returns i such that x=rank(i) && rank(i-1)<x or n if that i not exist
  // first binary search over first level rank structure
  // then sequential search using popcount over a int
  // then sequential search using popcount over a char
  // then sequential search bit a bit

  //binary search over first level rank structure
  ulong l=0, r=n/s;
  ulong mid=(l+r)/2;
  ulong rankmid = Rs[mid];
  while (l<=r) {
    if (rankmid<x)
      l = mid+1;
    else
      r = mid-1;
    mid = (l+r)/2;
    rankmid = Rs[mid];
  }
  //sequential search using popcount over a int
  ulong left;
  left=mid*factor;
  x-=rankmid;
  ulong j=data[left];
  ulong ones = popcount(j);
  while (ones < x) {
    x-=ones;left++;
    if (left > integers) return n;
    j = data[left];
    ones = popcount(j);
  }
  //sequential search using popcount over a char
  left=left*b;
  rankmid = popcount8(j);
  if (rankmid < x) {
    j=j>>8;
    x-=rankmid;
    left+=8;
    rankmid = popcount8(j);
    if (rankmid < x) {
      j=j>>8;
      x-=rankmid;
      left+=8;
      rankmid = popcount8(j);
      if (rankmid < x) {
        j=j>>8;
        x-=rankmid;
        left+=8;
      }
    }
  }

  // then sequential search bit a bit
        while (x>0) {
    if  (j&1) x--;
    j=j>>1;
    left++;
  }
  return left-1;
}

bool BitRankF::IsBitSet(ulong i) {
  return (1u << (i % W)) & data[i/W];
}




/* Implementation of  Bitselect Next */
BitSelectNext::BitSelectNext(ulong *bit, ulong n, bool owner) {
        this->owner = owner;
	this->datos=bit;
	this->n=n;
	ulong aux=(n+1)%W;
	if (aux != 0)
		integers = (n+1)/W+1;
	else 
		integers = (n+1)/W;
}

//Select Next
//this selectnext ask from 1 to n
ulong BitSelectNext::select_next(ulong k) {
	ulong count = k-1;
	ulong des,aux2;
	des=count%W;
	aux2= datos[count/W] >> des;
	if (aux2 > 0) {
		if ((aux2&0xff) > 0) return count+select_tab[aux2&0xff];
		else if ((aux2&0xff00) > 0) return count+8+select_tab[(aux2>>8)&0xff];
		else if ((aux2&0xff0000) > 0) return count+16+select_tab[(aux2>>16)&0xff];
		else {return count+24+select_tab[(aux2>>24)&0xff];}
	}
	
	for (ulong i=count/W+1;i<integers;i++) {
		aux2=datos[i];
		if (aux2 > 0) {
			if ((aux2&0xff) > 0) return i*W+select_tab[aux2&0xff];
			else if ((aux2&0xff00) > 0) return i*W+8+select_tab[(aux2>>8)&0xff];
			else if ((aux2&0xff0000) > 0) return i*W+16+select_tab[(aux2>>16)&0xff];
			else {return i*W+24+select_tab[(aux2>>24)&0xff];}
		}
	}
	return n+1;
}


BitSelectNext::~BitSelectNext() {
  if (owner) delete datos;
}
// The bitarray have positions from 0..n-1
BitRankFSparse::BitRankFSparse(ulong *bitarray, ulong n){
  ulong ones=0;
  ulong i,j,*sblockbit,*blockbit;
  assert(bitget(bitarray,n-1));
  for (i=0;i<n;i++) 
    if (bitget(bitarray,i)) ones++;
  L=(ulong)ceil(sqrt(n/ones));
  sblockbit=new ulong[(n/L)/W+1];
  ulong aux=0;
  for (i=0;i<(n/L)/W+1;i++) sblockbit[i] = 0;
  for (i=0;i*L<n;i++){
    assert(i*L<n);
    for (j=i*L;j<min(n,(i+1)*L);j++) {
      if (bitget(bitarray,j)) { bitset(sblockbit,i); aux++; break; }
    }
    //printf("maximo j revisado=%d\n", j);
    
  }
  sblock = new BitRankF(sblockbit,n/L,true);
  blockbit=new ulong[(L*sblock->rank(n/L))/W+1];
  for (i=0;i<(L*sblock->rank(n/L))/W+1;i++) blockbit[i] = 0;
  ulong loc=0;
  for (i=0;i*L<n;i++){
    assert(i*L<n);
    if (bitget(sblockbit,i)) {
      for (j=i*L;j<min(n,(i+1)*L);j++) {
	if (bitget(bitarray,j)) bitset(blockbit,j-i*L+loc);
      }
      loc+=L;
    //  printf("maximo j revisado=%d\n", j);
    }
  }
  block = new BitRankF(blockbit,L*sblock->rank(n/L),true); 

  //printf("largo de bloque %lu\n", L);
  //printf("numero de bloques activos%lu %lu\n", aux, sblock-> rank(n/L));

} 

BitRankFSparse::~BitRankFSparse() {
  delete sblock;
  delete block;
}

bool BitRankFSparse::IsBitSet(ulong i){
  ulong numblock=i/L;
  if (sblock->IsBitSet(numblock) == false) return false;
  else {
    ulong one_blocks=sblock->rank(numblock);
    ulong zero_blocks=numblock-one_blocks+1;
    //printf("mmm i=%d, zero_blocks=%d, L=%d, res=%d \n",i,zero_blocks,L, i-zero_blocks*L);
    return block->IsBitSet(i-zero_blocks*L);
  }
}

ulong BitRankFSparse::rank(ulong i){
  ulong numblock=i/L;
  ulong one_blocks=sblock->rank(numblock);
  if (sblock->IsBitSet(numblock) == false) {
    return block->rank(one_blocks*L-1);
  } else {
    ulong zero_blocks=numblock-one_blocks+1;
    return block->rank(i-zero_blocks*L);
  }
}

ulong BitRankFSparse::select(ulong i){
  ulong pos1=block->select(i);
  ulong one_blocks=pos1/L+1;
  ulong pos2=sblock->select(one_blocks);
  ulong zero_blocks=pos2-one_blocks+1;
  return pos1+zero_blocks*L;
}

ulong BitRankFSparse::SpaceRequirementInBits(){
  return block->SpaceRequirementInBits()+sblock->SpaceRequirementInBits()+sizeof(BitRankFSparse)*8;
}

ulong BitRankFSparse::prev(ulong start) {
      // returns the position of the previous 1 bit before and including start.
      // tuned to 32 bit machine
  ulong temp,zero_blocks;
  ulong numblock=start/L+1;
  ulong one_blocks=sblock->rank(numblock-1);
  zero_blocks=numblock-one_blocks;
  if (sblock->IsBitSet(numblock-1) == false) {
    temp = block->prev(one_blocks*L-1);
    numblock = sblock->prev(numblock-1)+1;
    zero_blocks=numblock-(one_blocks-1)-1;
    //printf("Caso1 ");
    return temp+zero_blocks*L;
  } else {
    temp = block->prev(start-zero_blocks*L);
    if (temp < (one_blocks-1)*L) {
      numblock = sblock->prev(numblock-2)+1;
      zero_blocks=numblock-(one_blocks-1);
    //printf("Caso2 ");
      return temp+zero_blocks*L;
    } else {
      zero_blocks=numblock-(one_blocks-1)-1;
    //printf("Caso3 ");
      return temp+zero_blocks*L;
    }
  }
}

