/* HuffAlphabetRank.h
   Copyright (C) 2005, Rodrigo Gonzalez, all rights reserved.

   This is a adaptation from a version programmed by Veli Makinen.
   http://www.cs.helsinki.fi/u/vmakinen/

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

#ifndef HuffAlphabetRank_h
#define HuffAlphabetRank_h

#include "basic.h"
#include "bitrankw32int.h"
#include "Huffman_Codes.h"

class THuffAlphabetRank {
// using fixed 0...255 alphabet
private:
   struct Atree {
      THuffAlphabetRank *left;
      THuffAlphabetRank *right;
   };
   union Anode {
      Atree children;
      uchar ch;
   };
   BitRankW32Int *bitrank;
   Anode node;
public:
   THuffAlphabetRank(uchar *s, ulong n, THuffman_Codes *codetable, ulong level, ulong factor);
   ulong rank(int c, int i, THuffman_Codes *codetable); // returns the number of characters c before and including position i
   bool IsCharAtPos(int c, int i, THuffman_Codes *codetable);
   int charAtPos(int i);
   uchar charAtPos2(ulong i, ulong *rank);
   ulong SpaceRequirementInBits();
   bool Test(uchar *s, ulong n, THuffman_Codes *codetable);
   ~THuffAlphabetRank();
   int save(FILE *f);
   int load(FILE *f);
   THuffAlphabetRank(FILE *f, int *error);
};

#endif
