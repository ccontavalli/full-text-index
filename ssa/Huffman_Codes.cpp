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
#include <cmath>

//
// This routine does a quick count of all the
// characters in the input text.  
//


void count_chars(uchar *text, ulong n, TCodeEntry *counts )
{
    ulong i;
    ulong  min_sym =(ulong) ceil(((double) n)*0.000000205); //((1+Sqrt(5))/2)^(-32) min number of symbols, so no code longer than 32 bits
    min_sym =(ulong) ceil(((double) (n+(min_sym-1)*size_uchar))*0.000000205); // all the symbols fails (upper bound)

    for (i = 0 ; i < size_uchar ; i++ )
        counts[ i ].count = 0;
    for (i=0; i<n; i++)
        counts[text[i]].count++; 
    
    for (i = 0 ; i < size_uchar ; i++ )  
        if ((counts[ i ].count > 0) && (counts[ i ].count < min_sym)) 
            counts[ i ].count = min_sym;
}

TCodeEntry *makecodetable(uchar *text, ulong n)
{
    TCodeEntry *result = new TCodeEntry[ size_uchar ];
    
    count_chars( text, n, result );
/*    ulong count=0;
    for (int i=0;i<256; i++) {
       count += result[i].count;
       if (result[i].count>0)
          printf("%c,%d: %d\n",i , i,result[i].count);
    }
    printf("count=%d\n",count);*/
    priority_queue< node, vector< node >, greater<node> > q;
//
// First I push all the leaf nodes into the queue
//
    for ( ulong i = 0 ; i < size_uchar ; i++ )
        if ( result[ i ].count )
            q.push(node( i, result[ i ].count ) );
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
// Now I compute and return the codetable
//
    //cout << "Char  Symbol   Code" << endl;
    //q.top().traverse();
    q.top().maketable(0u,0u, result);
    q.top().deleteme();
    q.pop();
    return result;
}

int save_codetable(FILE *f,TCodeEntry *codetable) {
  if (f == NULL) return 20;
  for (int i=0; i<size_uchar;i++) {
    if (codetable[i].save(f) != 0) return 21;
  }
  return 0;
}


int load_codetable(FILE *f,TCodeEntry **codetable) {
  if (f == NULL) return 23;
   TCodeEntry *result = new TCodeEntry[ size_uchar ];
  for (int i=0; i<size_uchar;i++) {
    if (result[i].load(f) != 0) return 25;
  }
  (*codetable)=result;
  return 0;
}

