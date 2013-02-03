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

#ifndef BitRankW32Int_h
#define BitRankW32Int_h

#include "basic.h"
/////////////
//Rank(B,i)// 
/////////////
//_factor = 0  => s=W*lgn
//_factor = P  => s=W*P
//Is interesting to notice
//factor=2 => overhead 50%
//factor=3 => overhead 33%
//factor=4 => overhead 25%
//factor=20=> overhead 5%

class BitRankW32Int {
private:
	ulong *data;
  bool owner;
	ulong n,integers;
  ulong factor,b,s;
  ulong *Rs; //superblock array

	ulong BuildRankSub(ulong ini,ulong fin); //uso interno para contruir el indice rank
	void BuildRank(); //crea indice para rank
public:
	BitRankW32Int(ulong *bitarray, ulong n, bool owner, ulong factor);
	~BitRankW32Int(); //destructor
  bool IsBitSet(ulong i);
	ulong rank(ulong i); //Nivel 1 bin, nivel 2 sec-pop y nivel 3 sec-bit

  ulong prev(ulong start); // gives the largest index i<=start such that IsBitSet(i)=true
  ulong select(ulong x); // gives the position of the x:th 1.
  ulong SpaceRequirementInBits();
  /*load-save functions*/
  int save(FILE *f);
  int load(FILE *f);
  BitRankW32Int(FILE *f, int *error); 
};

#endif

