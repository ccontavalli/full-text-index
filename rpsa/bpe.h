/* bpe.h
   Copyright (C) 2007, Rodrigo Gonzalez, all rights reserved.

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

#ifndef bpe_h
#define bpe_h

#include "basic.h"
#include "bitrankw32int.h"

class BPE {
private:
  ulong *symbols; //here is the compressed data
  ulong m; //length of symbols
  BitRankW32Int *BRR; //bitmap to indicate the structure of symbols_new
  ulong *symbols_new; // table to decompress de new codes
  ulong  symbols_new_len; // len of symbols_new
  ulong n;
  ulong nbits;
  ulong max_phrase; //length of max phrase
  ulong shift;
  ulong max_value;
  ulong max_assigned;
  ulong shift_it();
  ulong pointer();
  ulong pointer2();
  ulong treapme();
  void setphd(ulong pos, ulong value);
  void setdata(ulong pos, ulong value);
  void cheq();
  void new_value(ulong *symbols_pair,ulong *symbols_new_value,ulong *k1,ulong *j,ulong pos);
  ulong compress(double cutoff);
  ulong *case1(ulong pos1, ulong pos2, ulong cut);
  ulong *case2(ulong pos1, ulong pos2);
  void caso3(ulong r1, ulong r2, ulong start, ulong end, ulong *info);
  void compress_pair_table();
  ulong prev(ulong start);
  ulong next(ulong start);
  /* BPE with PSI */
  int getpar (ulong pos, ulong *a, ulong *b, ulong *bpos);
  int ini_psi();
  void compress_psi();
public:
  double avg_cost(); // returns the average cost to decompress one symbol
  void des1(ulong pos1, ulong pos2, ulong *info);
  BitRankW32Int *BR; //indicate if the the symbol is compressed
  BPE(ulong *array, ulong n,ulong _max_phrase, double _cutoff, bool _verbose);
  BPE(ulong *sa_diff,ulong *Psi,ulong ini,ulong length, bool _verbose);
  ~BPE(); //destructor
  ulong * dispair(ulong i, ulong len); 
  ulong * dispair2(ulong i, ulong len); 
  ulong * dispairall(); 
  ulong SpaceRequirement();
  /*load-save functions*/
  int save(FILE *f);
  int load(FILE *f);
  BPE(FILE *f, int *error); 
};

#endif
