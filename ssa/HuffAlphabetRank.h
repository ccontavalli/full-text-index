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
   TCodeEntry *codetable;
   BitRankW32Int *bitrank;
   uchar ch;
   bool leaf;
   THuffAlphabetRank *left;
   THuffAlphabetRank *right;
public:
   THuffAlphabetRank(uchar *s, ulong n, TCodeEntry *codetable, ulong level, ulong factor);
   ulong rank(uchar c, ulong i); // returns the number of characters c before and including position i
   bool IsCharAtPos(uchar c, ulong i);
   uchar charAtPos(ulong i);
   uchar charAtPos2(ulong i, ulong *rank);
   ulong SpaceRequirementInBits();
   bool Test(uchar *s, ulong n);
   ~THuffAlphabetRank();
   int save(FILE *f);
   int load(FILE *f, TCodeEntry *_codetable);
   THuffAlphabetRank(FILE *f, TCodeEntry *_codetable, int *error);
};

#endif
