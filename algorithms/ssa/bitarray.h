/* bitarray.h
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

#ifndef bitarray_h
#define bitarray_h

#include "basic.h"

class BitRankF {
private:
  ulong *data; //here is the bit-array
  bool owner;
  ulong n,integers;
  ulong factor,b,s;
  ulong *Rs; //superblock array
  uchar *Rb; //block array
  ulong BuildRankSub(ulong ini,ulong fin); //internal use of BuildRank
  void BuildRank(); //crea indice para rank
public:
  BitRankF(ulong *bitarray, ulong n, bool owner);
  ~BitRankF(); //destructor
  bool IsBitSet(ulong i);
  ulong rank(ulong i); //Rank from 0 to n-1
  ulong rank2(ulong i); //Rank from 0 to n-1
  ulong prev(ulong start); // gives the largest index i<=start such that IsBitSet(i)=true
  ulong select(ulong x); // gives the position of the x:th 1.
  ulong SpaceRequirementInBits();
  /*load-save functions*/
  int save(FILE *f);
  int load(FILE *f);
  BitRankF(FILE *f, int *error); 
};


class BitSelectNext {
private:
	ulong *datos; //arreglo de bits
	bool owner;
	ulong n;
	ulong integers;
public:
	// Crea arreglo segun numero de bits, semilla aleatoria y probabilidad
	BitSelectNext(ulong *bit, ulong n, bool owner); 
	~BitSelectNext(); //destructor
	ulong select_next(ulong i); // select_next
};

class BitRankFSparse {
private:
  BitRankF *block;
  BitRankF *sblock;
  ulong L;
public:
  BitRankFSparse(ulong *bitarray, ulong n);
  ~BitRankFSparse(); //destructor
  bool IsBitSet(ulong i);
  ulong rank(ulong i); //Rank from 0 to n-1
  ulong prev(ulong start); // gives the largest index i<=start such that IsBitSet(i)=true
  ulong select(ulong x); // gives the position of the x:th 1.
  ulong SpaceRequirementInBits();
};

#endif
