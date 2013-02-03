/* Huffman_Codes.h
   Copyright (C) 2005, Rodrigo Gonzalez, all rights reserved.

   This code is adapted from http://www.dogma.net/markn/articles/pq_stl/priority.htm

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

#ifndef maketable_h
#define maketable_h

#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <stack>
#include <string>
#include <queue>
#include "basic.h"
using namespace std;


// To append codes

//ulong SetBit(ulong x, ulong pos, ulong bit) {
//      return x | (bit << pos);
//}

// codetable := new TCodeEntry[256]

class TCodeEntry {
  public:
    ulong count;
    ulong bits;
    ulong code;
    TCodeEntry() {count=0;bits=0;code=0u;};
    int save(FILE *f) {
      if (f == NULL) return 20;
      if (fwrite (&count,sizeof(ulong),1,f) != 1) return 21;
      if (fwrite (&bits,sizeof(ulong),1,f) != 1) return 21;
      if (fwrite (&code,sizeof(ulong),1,f) != 1) return 21;
      return 0;
    };
    int load(FILE *f) {
      if (f == NULL) return 23;
      if (fread (&count,sizeof(ulong),1,f) != 1) return 25;
      if (fread (&bits,sizeof(ulong),1,f) != 1) return 25;
      if (fread (&code,sizeof(ulong),1,f) != 1) return 25;
      return 0;
    };
};

//
// The node class is used to represent both leaf
// and internal nodes. leaf nodes have 0s in the
// child pointers, and their value member corresponds
// to the character they encode.  internal nodes
// don't have anything meaningful in their value
// member, but their child pointers point to
// other nodes.
//

class node {
private:
    int weight;
    uchar value;
    const node *child0;
    const node *child1;
//
// Construct a new leaf node for character c
//
public:
    node( uchar c = 0, int i = -1 ) {
        value = c;
        weight = i;
        child0 = 0;
        child1 = 0;
    }
//
// Construct a new internal node that has
// children c1 and c2.
//
    node( const node* c0,  const node *c1 ) {
        value = 0;
        weight = c0->weight + c1->weight;
        child0 = c0;
        child1 = c1;
    }

//
// The comparison operators used to order the
// priority queue.
//
    bool operator>( const node &a ) const {
        return weight > a.weight;
    }
//
// The traverse member function is used to
// print out the code for a given node.  It
// is designed to print the entire tree if
// called for the root node.
//

    void traverse( string code = "" )  const
    {
        if ( child0 ) {
            child0->traverse( code + "0" );
            child1->traverse( code + "1" );
        } else {
            cout << " " << value << " " << (int) value << "      ";
            cout << setw( 2 ) << weight;
            cout << "     " << code << endl;
        }
    }
    void maketable( ulong code, ulong bits, TCodeEntry *codetable ) const
    {
        if ( child0 ) {
            child0->maketable( code | (0 << bits) , bits+1, codetable );
            child1->maketable( code | (1 << bits) , bits+1, codetable );
        } 
        else {
            codetable[value].code = code;    
            codetable[value].bits = bits;
        }
    }
    void deleteme() const {
        if ( child0 ) {
            if (child0->child0)
                child0->deleteme();
            if (child1->child0)
                child1->deleteme();
            delete child0;
            delete child1;
	}
    }
};

TCodeEntry *makecodetable(uchar *text, ulong n);
int save_codetable(FILE *f,TCodeEntry *codetable);
int load_codetable(FILE *f,TCodeEntry **codetable);
#endif
