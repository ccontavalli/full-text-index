/* Huffman_Codes.h
   Copyright (C) 2005, Rodrigo Gonzalez, all rights reserved.

   A compressed form of a Huffman code

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

#ifndef Huffman_Codes_h
#define Huffman_Codes_h

#include "basic.h"

class THuffman_Codes{
private:
// firts one is max bits, second the number of characters, then the characters 
   ulong *codes;
   uchar *characters;
public:
   THuffman_Codes(uchar *text, ulong n);
//   THuffman_Codes(TCodeEntry *result);
   ulong SpaceRequirementInBits();
   ulong get_code(uchar c);
   bool exist(uchar c);
   ~THuffman_Codes();
   int save(FILE *f);
   int load(FILE *f);
   THuffman_Codes(FILE *f, int *error);
};

#endif
