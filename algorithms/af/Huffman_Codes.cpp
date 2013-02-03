/* Huffman_Codes.cpp
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

#include "Huffman_Codes.h"
#include <assert.h>
#include <deque>      // for deque
#include <iostream>   // for cout, endl
#include <queue>      // for priority_queue
#include <string>     // for string
#include <vector>     // for vector#include <iostream>
#include <cmath>     // for vector#include <iostream>

using namespace std;
// The TCodeEntry class is used like a auxiliary
// class to THuffman_Codes Class
class TCodeEntry {
public:
   ulong bits;
   ulong code;
   TCodeEntry() {bits=0;code=0u;}
   int size_in_bits() { return sizeof(ulong)*8*2;}
};

static TCodeEntry *makecodetable(uchar *text, ulong n);


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
    const node *child0;
    const node *child1;
//
// Construct a new leaf node for character c
//
public:
    int weight;
    uchar value;
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
    node( const node* c0, const node *c1 ) {
        value = 0;
        weight = c0->weight + c1->weight;
        child0 = c0;
        child1 = c1;
    }

//
// Build the code table from node
//
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

// The comparison operators used to order the
// priority queue.

    bool operator>( const node &a ) const {
        return weight > a.weight;
    }
                   
};

static TCodeEntry *makecodetable(uchar *text, ulong n)
{
    TCodeEntry *result = new TCodeEntry[size_uchar];
    ulong *counts= new ulong[size_uchar];
    ulong i;
    ulong  min_sym =(ulong) ceil(((double) n)*0.000000205); //((1+Sqrt(5))/2)^(-32) min number of symbols, so no code longer than 32 bits
    min_sym =(ulong) ceil(((double) (n+(min_sym-1)*size_uchar))*0.000000205); // all the symbols fails (upper bound)

//    
// Count of all the characters in the input text.
//
    for (i = 0 ; i < size_uchar ; i++ )
        counts[i] = 0;
    for (i=0; i<n; i++)
        counts[text[i]]++; 

    for (i = 0 ; i < size_uchar ; i++ )
        if ((counts[i] > 0) && (counts[i] < min_sym))
            counts[i] = min_sym;

//
// First I push all the leaf nodes into the queue
//
    priority_queue< node, vector< node >, greater<node> > q;
    for (i = 0 ; i < size_uchar ; i++ )
        if ( counts[i] ) {
           q.push(node( i, counts[i]) );
	}

    delete [] counts;

//
// This loop removes the two smallest nodes from the
// queue.  It creates a new internal node that has
// those two nodes as children. The new internal node
// is then inserted into the priority queue.  When there
// is only one node in the priority queue, the tree
// is complete.
//
    while ( q.size() > 1 ) {
            node *child0 = new node( q.top() );
            q.pop();
            node *child1 = new node( q.top() );
            q.pop();
            q.push( node( child0, child1 ) );
    }
//
// Now I compute the codetable
//
  //  q2.top()->maketable(0u,0u, result);
    q.top().maketable(0u,0u, result);
    q.top().deleteme();
    q.pop();
	    

//
// finally return the result
//
    return result;
}

// The definition needed by THuffman_Codes class 

THuffman_Codes::THuffman_Codes(uchar *text, ulong n) {
   TCodeEntry *result;
   result = makecodetable(text,n);
   ulong max=0,size=0,i;
   for (i=0;i<size_uchar;i++) {
      if (result[i].bits != 0) size++;
      if (max < result[i].bits) max = result[i].bits;
   }
   characters = new uchar[size+2];
   codes = new ulong[(max*size)/W+1];
   characters[0]=max;
   characters[1]=size-1;
   size=2;
   for (i=0;i<size_uchar;i++) {
      if (result[i].bits != 0) {
         characters[size]=i;
         SetField(codes,characters[0],size-2,result[i].code);
         size++;
      }
   }
   delete [] result;      
}

ulong THuffman_Codes::SpaceRequirementInBits(){
   return (characters[1]+3)*sizeof(uchar)*8 // Characters
          +(characters[0]*(characters[1]+1)/W+1)*W // codes
          +2*W; //pointers
}

ulong THuffman_Codes::get_code(uchar c) {
   int i;
   for (i=2;i<characters[1]+3;i++) 
      if (characters[i] == c) break; 
   return GetField(codes,characters[0], i-2);
}
      
bool THuffman_Codes::exist(uchar c) {
   bool belong=false;
   for (int i=2;i<characters[1]+3;i++) {
      if (characters[i] == c) { belong = true; break; }
   }
   return belong;
}
      
THuffman_Codes::~THuffman_Codes() {
   delete [] characters;
   delete [] codes;
}

int THuffman_Codes::save(FILE *f) {
  if (f == NULL) return 20;
  if (fwrite (characters,sizeof(uchar),1,f) != 1) return 21; //max
  if (fwrite (characters+1,sizeof(uchar),1,f) != 1) return 21; //size
  if (fwrite (characters+2,sizeof(uchar),characters[1]+1,f) != (characters[1]+1)) return 21;
  if (fwrite (codes,sizeof(ulong),(characters[0]*(characters[1]+1))/W+1,f) != (size_t)(characters[0]*(characters[1]+1))/W+1) return 21;
  return 0;
}
int THuffman_Codes::load(FILE *f) {
  if (f == NULL) return 23;
  uchar max,size;
  if (fread (&max,sizeof(uchar),1,f) != 1) return 25; //max
  if (fread (&size,sizeof(uchar),1,f) != 1) return 25; //size
  characters = new uchar[size+1+2];
  codes = new ulong[(max*(size+1))/W+1];
  characters[0]=max;
  characters[1]=size;
  if (fread (characters+2,sizeof(uchar),size+1,f) !=  (size+1)) return 25;
  if (fread (codes,sizeof(ulong),(max*(size+1))/W+1,f) != (size_t)(max*(size+1))/W+1) return 25;
  return 0;
}

THuffman_Codes::THuffman_Codes(FILE *f, int *error) {
    *error = load(f);
}
  
