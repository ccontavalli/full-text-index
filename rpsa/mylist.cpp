/* mylist.cpp
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

#include "mylist.h"

mylist::mylist() {
  head=NULL;
}

mylist::~mylist() {
  mynodo *aux;
  while (head != NULL) {
    aux=head;
    head=head->next;
    free(aux);
  }
}

bool mylist::empty() {
  return (head == NULL);
}

mynodo *mylist::pop() {
  mynodo *aux;
  if (head == NULL) return NULL;
  else {
    aux=head;
    head=head->next;
    return aux;
  }
}

void mylist::push(ulong v1, ulong v2) {
  mynodo *aux;
  aux = (mynodo *) malloc (sizeof(mynodo));
  aux->v1=v1; aux->v2=v2;
  aux->next=head;
  head=aux;
}

