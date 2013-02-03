/* bpe.c
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
#include "bpe.h"
#include "Treap.h"
#include "mylist.h"


#define BPE_MINIMO 1
#define BPE_MINIMOTREAP 1

/* define for debuging */
#define ID_CHEQ 576546571
#define DEBUGME1

/* These variables are used only in construction time */
static ulong NUM_MAX_RULES=3125000; //100.000.000 (*32) to calculate the exact size of the dictionary during construction time
static ulong *phd; //here is the links
static ulong *symbols_pair; 
static Treap *frec;
static ulong *data;
static ulong *Psi;
static ulong Psi0;
static ulong naux;
static ulong *bit_id; //during construction indicate if a symbol is on the tree, in the final structure indicate if the the symbol is compressed
//static ulong *in_treap; // bitmap to indicate if a position is pair in treap
static ulong *symbols_new_bit; 
static ulong symbols_new_bit_len;
static ulong *symbols_new_frec;
static ulong symbols_new_frec_val=0;
static bool verbose=false;

class mytreenode {
public:
  /** Children of the node */
  mytreenode * left, * right, *father;
  ulong value;
  mytreenode(ulong val) {value=val;left=NULL;right=NULL;father=NULL;};
  ~mytreenode() {};
};


//using namespace std;

ulong BPE::shift_it(){
  ulong  min=data[0],max=data[0];
  for(ulong i=0; i<n ; i++) {
    if (min > data[i]) min = data[i];
    if (max < data[i]) max = data[i];
  }
  if (min != 0 ) {
    for(ulong i=0; i<n ; i++) 
      data[i] = data[i]-min;
    max=max-min;
  }
  this->shift=min;
  this->max_value=max;
  this->max_assigned=max;
  return 0;
}


static int compar(const void *_a, const void *_b) { 
  ulong  a=*((ulong *) _a),b=*((ulong *) _b);
  if (data[a] != data[b]) return (data[a]-data[b]);
  else if (data[a+1] != data[b+1]) return (data[a+1]-data[b+1]);
  else return a-b;
}

void BPE::cheq() {
  ulong i,aux1,aux2;
  for (i=0;i<n-2;i++) 
    if ((i+phd[i] != n-1 ) && (bitget(bit_id,i) == 1) && (bitget(bit_id,i+phd[i]) == 1) ) {
      aux1=i;
      if (bitget(bit_id,aux1+1) == 0) aux1=aux1+1+data[aux1+1]; else aux1 = aux1+1;
      aux2=i+phd[i];
      if (bitget(bit_id,aux2+1) == 0) aux2=aux2+1+data[aux2+1]; else aux2 = aux2+1;
      if ((aux2 != n-1) && ((data[i] != data[i+phd[i]]) || (data[aux1] != data[aux2]))) {
        if (verbose) printf("Error en i= %lu, phd[i]= %lu, j=i+phd[i]=%lu, ciclo=%lu\n", i, phd[i],i+phd[i],max_assigned);
        phd[i]=n-1-i; 
        fflush(stdout);
      }
    }
}

ulong BPE::pointer() {
  ulong i,*tmp; // puntero hacia adelante
  tmp = (ulong *) malloc (sizeof(ulong)*(this->n));
  if (!tmp) return 1;
  for (i=0;i<n-1;i++) tmp[i]=i;
  qsort(tmp,n-1,sizeof(ulong),compar);

  phd = (ulong *) malloc (sizeof(ulong)*(this->n));
  if (!phd) return 1;
  for (i=0; i< n-1; i++) 
    phd[i]=0;

  /* Delete the positions k * max_phrase */
  ulong move=0;
  ulong pos=0;
  if (verbose) printf("Largo max frase = %lu\n", max_phrase);
  for (i=0;i<n-1;i++) {
    if ((tmp[i]+1) % max_phrase == 0) {
      setphd(tmp[i],n-1-tmp[i]);
      move++;
    } else {
      tmp[pos]=tmp[i];
      pos++;
    }
  }

  if (verbose) printf("Pos = %lu, move = %lu\n", pos,move);

  for (i=0;i<n-2-move;i++) {
   if ((data[tmp[i]] == data[tmp[i+1]]) && (data[tmp[i]+1] == data[tmp[i+1]+1])) 
     setphd(tmp[i],tmp[i+1]-tmp[i]);
   else
     setphd(tmp[i],n-1-tmp[i]);
  }
  setphd(tmp[n-2],n-1-tmp[n-2]);
  setphd(n-1,0);
  free(tmp);

  return 0;
}

static int compar2(const void *_a, const void *_b) { 
  ulong  a=*((ulong *) _a),b=*((ulong *) _b);
  ulong aux1,aux2;
  int a1,b1;
  a1=bitget(bit_id,a);
  b1=bitget(bit_id,b);
  if (a1+b1 == 0) return 0;
  if (a1==0) return 1;
  if (b1==0) return -1;
  if (bitget(bit_id,a+1) == 0) aux1=a+1+data[a+1]; else aux1 = a+1;
  if (bitget(bit_id,b+1) == 0) aux2=b+1+data[b+1]; else aux2 = b+1;
  if ((aux1 == naux) && (aux2 == naux)) return 0;
  if (aux1 == naux) return 1;
  if (aux2 == naux) return -1;
  if (data[a] != data[b]) return (data[a]-data[b]);
  else if (data[aux1] != data[aux2]) return (data[aux1]-data[aux2]);
  else return a-b;
}

ulong BPE::pointer2() {
  ulong i,j,*tmp; // puntero hacia adelante
  ulong aux1,aux2;
  tmp = (ulong *) malloc (sizeof(ulong)*(this->n));
  if (!tmp) return 1;
  for (i=0;i<n-1;i++) tmp[i]=i;
  qsort(tmp,n-1,sizeof(ulong),compar2);

  if (!phd) return 1;
  for (i=0; i< n-1; i++)
    phd[i]=n-1;
  for (i=0;i<n-2;i++) {
    if (bitget(bit_id,tmp[i+1]) == 0) {if (verbose) printf("Valor de corte %lu\n",i); break;}
    if (bitget(bit_id,tmp[i]+1) == 0) aux1=tmp[i]+1+data[tmp[i]+1]; else aux1 = tmp[i]+1;
    if (bitget(bit_id,tmp[i+1]+1) == 0) aux2=tmp[i+1]+1+data[tmp[i+1]+1]; else aux2 = tmp[i+1]+1;

    if ((data[tmp[i]] == data[tmp[i+1]]) && (data[aux1] == data[aux2]) && (aux2 != n-1)) 
      setphd(tmp[i],tmp[i+1]-tmp[i]);
    else
      setphd(tmp[i],n-1-tmp[i]);
  }
  for (j=i; j<n-1; j++)
    setphd(tmp[j],n-1-tmp[j]);
  setphd(n-1,0);
  free(tmp);
  cheq();
  return 0;
}

ulong BPE::prev(ulong start) {
      // returns the position of the previous 1 bit before and including start.
      // tuned to 32 bit machine

      ulong i = start >> 5;
      int offset = (start % W);
      ulong answer = start;
      ulong val = bit_id[i] << (Wminusone-offset);

      if (!val) { val = bit_id[--i]; answer -= 1+offset; }

      while (!val) { val = bit_id[--i]; answer -= W; }

      if (!(val & 0xFFFF0000)) { val <<= 16; answer -= 16; }
      if (!(val & 0xFF000000)) { val <<= 8; answer -= 8; } 

      while (!(val & 0x80000000)) { val <<= 1; answer--; }
      return answer;
}

ulong BPE::next(ulong k) {
        ulong count = k;
        ulong des,aux2;
        des=count%W; 
        aux2= bit_id[count/W] >> des;
        if (aux2 > 0) {
                if ((aux2&0xff) > 0) return count+select_tab[aux2&0xff]-1;
                else if ((aux2&0xff00) > 0) return count+8+select_tab[(aux2>>8)&0xff]-1;
                else if ((aux2&0xff0000) > 0) return count+16+select_tab[(aux2>>16)&0xff]-1;
                else {return count+24+select_tab[(aux2>>24)&0xff]-1;}
        }

        for (ulong i=count/W+1;;i++) {
                aux2=bit_id[i];
                if (aux2 > 0) {
                        if ((aux2&0xff) > 0) return i*W+select_tab[aux2&0xff]-1;
                        else if ((aux2&0xff00) > 0) return i*W+8+select_tab[(aux2>>8)&0xff]-1;
                        else if ((aux2&0xff0000) > 0) return i*W+16+select_tab[(aux2>>16)&0xff]-1;
                        else {return i*W+24+select_tab[(aux2>>24)&0xff]-1;}
                }
        }
        return n;
} 

ulong BPE::treapme(){
  ulong i, *bit_id2;

  bit_id = new ulong[((this->n)/W)+1]; 
  if (!bit_id) return 1;

  for (i=0; i< ((this->n)/W)+1 ; i++)
    bit_id[i] = (ulong) -1; // 0xFFFFFFFF; // 32 ones

  frec = new Treap(data,bit_id,n);

  bit_id2 = (ulong *) malloc (sizeof(ulong)*((this->n)/W+1));
  if (!bit_id2) return 1;
  for (i=0; i< ((this->n)/W)+1 ; i++)
    bit_id2[i] = 0;

  ulong position,count;
  for (i=0; i< n-1; i++) {
    if (bitget(bit_id2,i) == 0) {
      bitset(bit_id2,i);
      if (phd[i] > 1) count=1;
      else count = 0; 
      position=i+phd[i];
      while (position != n-1 ) {
#ifdef DEBUGME
        assert(position <= n-1);
#endif
        //if (phd[position] >= 1) 
        count++;
        bitset(bit_id2,position);
        position=position+phd[position];
      }
    } else
      count = 0; 
    if (count > BPE_MINIMOTREAP)  
      frec->insert(i,count);
  }

  /* set phd[p1] = n for pairs that not appear more than once */


  for (i=0; i< ((this->n)/W)+1 ; i++)
    bit_id2[i] = 0;

  for (i=0; i< n-1; i++) {
    if (bitget(bit_id2,i) == 0) {
      position=i+phd[i];
      if (position == n-1)
      phd[i]=n;
      while (position != n-1 ) {
        bitset(bit_id2,position);
        position=position+phd[position];
      }
    }
  }

  free(bit_id2);
  return 0;
}


inline void BPE::setphd(ulong pos, ulong value){
#ifdef DEBUGME
  //TreapNode *tp1;
  if ((pos >= ID_CHEQ) && (pos <= ID_CHEQ)){
    if (verbose) printf("Set phd %lu %lu %lu\n",pos, value, max_assigned);
    //tp1=frec->search(ID_CHEQ);
  } 
#endif

  phd[pos]=value;
}
inline void BPE::setdata(ulong pos, ulong value){
#ifdef DEBUGME
  if (pos == ID_CHEQ) {
    if (verbose) printf("Set data %lu %lu \n",pos, max_assigned);
  } 
#endif
  data[pos]=value;
}

ulong BPE::compress(double cutoff) {
  ulong i,j,p1=0,p2=0,p3=0,p,f1,f2,a1,a2,aux,aux1=0,aux2=0,aux_value,pp,q1,q2,count,pp1,pp2,auxp,anterior,num,*visit,*visit1,cuentame=0,estimado=0;
  bool list1,list2;
  ulong porcentajedeavance1=1; //, porcentajedeavance2=1;
  double paraaquinomas=0;
  TreapNode *nodo,*tp1,*tp2;
  mynodo *a;
  mylist *reemplazos;
  reemplazos = new mylist();
  nodo=frec->pop();
  if (nodo != NULL) visit = (ulong *) malloc(sizeof(ulong)*((nodo->priority/W)+1));
  else visit =NULL;
  if (nodo != NULL) visit1 = (ulong *) malloc(sizeof(ulong)*((nodo->priority/W)+1));
  else visit1 =NULL;
  symbols_new_frec = (ulong *) malloc(sizeof(ulong)*(NUM_MAX_RULES));
  for (i=0;i<NUM_MAX_RULES;i++) symbols_new_frec[i]=0;
  //ulong chucha=10000000;
  while ((nodo != NULL) && (nodo->priority > BPE_MINIMO)) {
  p=nodo->value;
  //if (chucha < nodo->priority) printf ("mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm\n");
  //  chucha=nodo->priority;
  max_assigned++;
  if (bitget(bit_id,p+1) == 0) {
    reemplazos->push(data[p],data[p+1+data[p+1]]);
    if (data[p+1+data[p+1]] > max_value)
      if (bitget(symbols_new_frec,data[p+1+data[p+1]]-max_value) == 0) {
        symbols_new_frec_val++;
        bitset(symbols_new_frec,data[p+1+data[p+1]]-max_value);
      }
  } else {
    reemplazos->push(data[p],data[p+1]);
    if (data[p+1] > max_value)
      if (bitget(symbols_new_frec,data[p+1]-max_value) == 0) {
        symbols_new_frec_val++;
        bitset(symbols_new_frec,data[p+1]-max_value);
      }
  }
  if (data[p] > max_value)
    if (bitget(symbols_new_frec,data[p]-max_value) == 0) {
      symbols_new_frec_val++;
      bitset(symbols_new_frec,data[p]-max_value);
    }
  anterior =n;
  while (p != n-1)  { 
    if ((p != 0 ) && ((p%max_phrase) != 0 )) {
      if (bitget(bit_id,p-1) == 0) p1=prev(p-1); else p1 = p-1;
      if (phd[p1] != n) {
      tp1=frec->search(p1);
      //if (tp1 == NULL) {
         //if (verbose) printf("Tipo P1  %lu %lu %lu %lu\n",p1,p1+phd[p1],phd[p1], n);
         //phd[p1]=n;
      //}

      if ((tp1 !=NULL) && (tp1->value != nodo->value) && (p1 != anterior)) {
       f1=tp1->priority -1;
       aux_value=tp1->value;
       if (f1 > BPE_MINIMOTREAP) {
        if (aux_value == p1) {
          //frec->decreaseKey(p1+phd[p1],f1);
          frec->remove(p1);
          frec->insert(p1+phd[p1],f1);
          setphd(p1,n);
          //setphd(p1,n-1-p1);
        } else { 
          a1=tp1->value;
          while (a1+phd[a1] != p1) {
            a1=a1+phd[a1];
          } 
          aux=a1+phd[a1];
          setphd(a1,phd[a1]+phd[p1]);
          setphd(aux,n-1-aux);
          //frec->decreaseKey(aux_value,f1);
          frec->remove(aux_value);
          frec->insert(aux_value,f1);
        }
       } else {
        a1=aux_value+phd[aux_value];
        setphd(aux_value,n);
        setphd(a1,n);
        frec->remove(aux_value);
       }
      } 
    }
    }  
    if (bitget(bit_id,p+1) == 0) p2=p+1+data[p+1]; else p2 = p+1;
    anterior=p2;
    if ((p2 != p+phd[p]) && (p2 != n-1))  {
    if ((bitget(bit_id,p2+1) == 0)) p3=p2+1+data[p2+1]; else p3 = p2+1;
    if ((p3 % max_phrase != 0) && (p3 != n-1)) {
      if ((phd[p2] != n)) {
      tp2=frec->search(p2);
      //if (tp2 == NULL) {
         //if (verbose) printf("Tipo P2  %lu %lu %lu %lu\n",p2,p2+phd[p2], phd[p2], n);
         //phd[p2]=n;
      //}
 
      if  ((tp2 != NULL)){ //&& (tp2->value != nodo->value)) {
       f2=tp2->priority -1;
       aux_value=tp2->value;
       if (f2 > BPE_MINIMOTREAP) {
        if (aux_value == p2) {
          //frec->decreaseKey(p2+phd[p2],f2);
          frec->remove(p2);
          frec->insert(p2+phd[p2],f2);
          setphd(p2,n);
          //setphd(p2,n-1-p2);
        } else { 
          a2=aux_value;
          while (a2+phd[a2] != p2) {       
            a2=a2+phd[a2];
          }
          aux=a2+phd[a2];
          setphd(a2,phd[a2]+phd[p2]);
          setphd(aux,n-1-aux);
          //frec->decreaseKey(aux_value,f2);
          frec->remove(aux_value);
          frec->insert(aux_value,f2);
        }
       } else {
        a2=aux_value+phd[aux_value];
        setphd(aux_value,n);
        setphd(a2,n);
        frec->remove(aux_value);
       }
      } 
    }
    }
    }
    if (p2==p+phd[p]) {aux=p2+phd[p2]; phd[p]=aux-p;phd[p2]=n;}
    p+=phd[p];//if (p2==p+phd[p]) p=p2+phd[p2]; else p+=phd[p];
    
  }
  num=0;
  p=nodo->value;
  while (p!=n-1) {
    num++;
    setdata(p,max_assigned);

    if (bitget(bit_id,p+1) == 0) p2=p+1+data[p+1]; else p2 = p+1; 
    if ((p2 != n-1) && (bitget(bit_id,p2+1) == 0)) p3=p2+1+data[p2+1]; else p3 = p2+1;
    
    bitclean(bit_id,p2);
    
    if (p2+1 == n-1) {
      setdata(p+1,n-1-(p+1));
      phd[p+1]=n-1-(p+1);
    } else {
      if (bitget(bit_id,p2+1) == 0) p3=p2+1+data[p2+1]; else p3 = p2+1;
      setdata(p+1,p3 -(p+1));
      phd[p+1]=n-1-(p+1);
    }
    p+=phd[p];
  }

  /* to avoid repetition on forward search */
  for (i=0;i<num/W+1;i++)
    visit[i]=0;

#ifdef DEBUGME
  if ((nodo->value == 1024) && (nodo->priority == 2)) 
    printf("Debugme\n");
#endif

  i=0;
  p=nodo->value;
  while (p!=n-1) {
    i++;
    if (p % max_phrase != 0) { 
      if (bitget(visit,i) != 1) {
        if (bitget(bit_id,p+1) == 0) p2=p+1+data[p+1]; else p2 = p+1; 
        if (p != 0 ) {
          if (bitget(bit_id,p-1) == 0) p1=prev(p-1); else p1 = p-1;
          q1=data[p1];
          count=1;
          pp=p+phd[p];
          j=i;
          while (pp != n-1) {
            j++;
            if (pp % max_phrase != 0) {
              if (bitget(visit,j) != 1) {
                if (bitget(bit_id,pp-1) == 0) pp1=prev(pp-1); else pp1 = pp-1;
                if (q1==data[pp1]) {count++; bitset(visit,j);}
              }
            }
            pp+=phd[pp];
          }
          if (count >BPE_MINIMOTREAP) {
            frec->insert(p1,count);
          }
        }
      }
    }
    p+=phd[p];
  }
  /* to avoid repetition on forward search */
  for (i=0;i<num/W+1;i++)
    visit[i]=0;

  i=0;
  p=nodo->value;
  while (p!=n-1) {
    i++;
    if (bitget(visit,i) != 1) {
      if (bitget(bit_id,p+1) == 0) p2=p+1+data[p+1]; else p2 = p+1;
      if ((p2 % max_phrase != 0 ) && (p2 != n-1) && (p2 != n)) {
        q1=data[p2];
        if (q1 != max_assigned) {
          count=1;
          pp=p+phd[p];
          j=i;
          while (pp != n-1) {
            j++;
            if (bitget(visit,j) != 1) {
              if (bitget(bit_id,pp+1) == 0) pp2=pp+1+data[pp+1]; else pp2 = pp+1;
              if ((pp2 % max_phrase != 0 ) && (pp2 != n) && (q1==data[pp2])) {count++; bitset(visit,j);}
            }
            pp+=phd[pp];
          }
          if (count >BPE_MINIMOTREAP) {
            frec->insert(p,count);
          }
        }
      }
    }
    p+=phd[p];
  }

  /* to avoid repetition on forward search */
  for (i=0;i<num/W+1;i++)
    visit[i]=0;
  for (i=0;i<num/W+1;i++)
    visit1[i]=0;


  ulong jj,ii;
  jj=0;

  p=nodo->value;

#ifdef DEBUGME
  if ((nodo->value == 1024) && (nodo->priority == 2)) 
    printf("Debugme\n");
#endif

  while (p!=n-1) {
    ii=jj+1;
    if (p == 0) {
      if (bitget(bit_id,p+1) == 0) p2=p+1+data[p+1]; else p2 = p+1;
      q2=data[p2]; 
      auxp=p;
      aux1=n;
      aux2=n;
      pp=p+phd[p];
      //while ((pp%max_phrase == 0) && (pp != n-1)) pp+=phd[pp];
      list2=false;
      while ((pp != n-1) && (!list2) && (p2 % max_phrase != 0)) {
        if (!list2) {
          if (bitget(bit_id,pp+1) == 0) pp2=pp+1+data[pp+1]; else pp2 = pp+1;
          //if ((pp2 % max_phrase != 0) && (pp2 != n) && (q2==data[pp2])) {aux2=pp-p;list2=true;} 
          if ((pp2 % max_phrase != 0) && (pp2 != n) && (q2==data[pp2])) {bitset(visit1,ii);aux2=pp-p;list2=true;} 
        }
        pp+=phd[pp];
        ii++;
      }
    } else {
      if (bitget(bit_id,p-1) == 0) p1=prev(p-1); else p1 = p-1;
      q1=data[p1];
      if (bitget(bit_id,p+1) == 0) p2=p+1+data[p+1]; else p2 = p+1;
      q2=data[p2]; 
      auxp=p;
      aux1=n;
      aux2=n;
      pp=p+phd[p];
      //while ((pp%max_phrase == 0) && (pp != n-1)) pp+=phd[pp];
      list1=false;
      list2=false;
      while ((pp != n-1) && (!list1 || !list2)) {
        //if ((!list1) && (pp%max_phrase != 0)) {
        if ((!list1) && (pp%max_phrase != 0) && (auxp%max_phrase != 0)) {
          if (bitget(bit_id,pp-1) == 0) pp1=prev(pp-1); else pp1 = pp-1;
          if (q1==data[pp1]) {bitset(visit,ii);aux1=pp1-p1;list1=true;}
        }
        if (p2 % max_phrase != 0) { 
          if (!list2) {
            if (bitget(bit_id,pp+1) == 0) pp2=pp+1+data[pp+1]; else pp2 = pp+1;
            if ((pp2 % max_phrase != 0) && (pp2 != n) && (q2==data[pp2])) {bitset(visit1,ii);aux2=pp-p;list2=true;} 
          }
        }
        pp+=phd[pp];
        ii++;
      }
    }
    p+=phd[p];
    if ((aux1 != n) && (auxp % max_phrase != 0)) setphd(p1,aux1); else {if (bitget(visit,jj) == 0) setphd(p1,n); else setphd(p1,n-1-p1);}
    if (aux2 != n) setphd(auxp,aux2); else {if (bitget(visit1,jj) == 0) setphd(auxp,n); else setphd(auxp,n-1-auxp);}
    //if ((aux1 != n) && (auxp % max_phrase != 0)) setphd(p1,aux1); else setphd(p1,n-1-p1);
    //if (aux2 != n) setphd(auxp,aux2); else setphd(auxp,n-1-auxp);
    jj++;
  }
#ifdef DEBUGME
  //cheq();
#endif
  //pointer2();
  estimado+=num;
  cuentame++;
  paraaquinomas=((3*(max_assigned-max_value)-symbols_new_frec_val)/W + 2*(max_assigned-max_value)-symbols_new_frec_val)/(float)n*100;
  if ((verbose) && (paraaquinomas> porcentajedeavance1/20.0)) {
  //if (cuentame++ % 10000 == 9999 ) {
    printf("it%2lu %lu, F= %lu, S= %lu,", porcentajedeavance1,
           cuentame,nodo->priority,n-estimado);
    porcentajedeavance1++;
    printf(" Dic=%lu(%.6f%%),", 2*(max_assigned-max_value) , (2*(max_assigned-max_value))/(float)n*100);
    printf(" Rep=%lu, Rul=%lu,", symbols_new_frec_val , max_assigned-max_value);
    printf(" Tira=%lu,", (3*(max_assigned-max_value)-symbols_new_frec_val)/W);
    printf(" C=%lu,", 2*(max_assigned-max_value)-symbols_new_frec_val);
    printf(" Dic=%.6f%%,", ((3*(max_assigned-max_value)-symbols_new_frec_val)/W + 2*(max_assigned-max_value)-symbols_new_frec_val)/(float)n*100 );
    printf(" TOT=%.6f%%\n", (n-estimado+(3*(max_assigned-max_value)-symbols_new_frec_val)/W + 2*(max_assigned-max_value)-symbols_new_frec_val)/(float)n*100 );
    fflush(stdout);
  }
 /*
  paraaquinomas=(2*(max_assigned-max_value))/(float)n*100;
  if ((verbose) && (paraaquinomas> porcentajedeavance2/50.0)) {
  //if (cuentame++ % 10000 == 9999 ) {
    printf("itb%2lu %lu, Frec= %lu, size = %lu, total = %lu, (%.6f%%)", porcentajedeavance2,
           cuentame,nodo->priority,n-estimado,n-estimado+2*cuentame,(n-estimado+2*cuentame)/(float)n*100);
    porcentajedeavance2++;
    printf(" Dic=%lu (%.6f%%),", 2*(max_assigned-max_value) , (2*(max_assigned-max_value))/(float)n*100);
    printf(" Rep=%lu, reglas=%lu,", symbols_new_frec_val , max_assigned-max_value);
    printf(" Tira=%lu bits (%lu),", 3*(max_assigned-max_value)-symbols_new_frec_val, (3*(max_assigned-max_value)-symbols_new_frec_val)/W);
    printf(" C=%lu,", 2*(max_assigned-max_value)-symbols_new_frec_val);
    printf(" Dic=%.6f%%,", ((3*(max_assigned-max_value)-symbols_new_frec_val)/W + 2*(max_assigned-max_value)-symbols_new_frec_val)/(float)n*100 );
    printf(" TOT=%.6f%%\n", (n-estimado+(3*(max_assigned-max_value)-symbols_new_frec_val)/W + 2*(max_assigned-max_value)-symbols_new_frec_val)/(float)n*100 );
    fflush(stdout);
  }
*/
  free(nodo);
  nodo=frec->pop();
  // To finish to some porcent of avance
  if (porcentajedeavance1/50.0 > cutoff)
    while (nodo != NULL) { free(nodo); nodo=frec->pop();}
  
  }
  if (verbose) {
    printf("itr %lu, size = %lu, total = %lu, (%.6f%%)",
           cuentame,n-estimado,n-estimado+2*cuentame,(n-estimado+2*cuentame)/(float)n*100); 
    printf(" Dic=%lu (%.6f%%),", 2*(max_assigned-max_value) , (2*(max_assigned-max_value))/(float)n*100);
    printf(" Rep=%lu, reglas=%lu,", symbols_new_frec_val , max_assigned-max_value);
    printf(" Tira=%lu bits (%lu),", 3*(max_assigned-max_value)-symbols_new_frec_val, (3*(max_assigned-max_value)-symbols_new_frec_val)/W);
    printf(" C=%lu,", 2*(max_assigned-max_value)-symbols_new_frec_val);
    printf(" Dic=%.6f%%,", ((3*(max_assigned-max_value)-symbols_new_frec_val)/W + 2*(max_assigned-max_value)-symbols_new_frec_val)/(float)n*100 );
    printf(" TOT=%.6f%%\n", (n-estimado+(3*(max_assigned-max_value)-symbols_new_frec_val)/W + 2*(max_assigned-max_value)-symbols_new_frec_val)/(float)n*100 );
    fflush(stdout);
  }
  if (nodo!=NULL) free(nodo);
  if (visit!=NULL) free(visit);
  if (visit1!=NULL) free(visit1);
  free(phd);  
  delete frec;
  j=max_assigned;
  symbols_pair = (ulong*) malloc(sizeof(ulong)*2*(max_assigned-max_value));
  while (!reemplazos->empty()) {
    a = reemplazos->pop();
    symbols_pair[2*(j-max_value-1)]=a->v1;
    symbols_pair[2*(j-max_value-1)+1]=a->v2;
    j--;
    free(a);
  }
  delete reemplazos;
  /*for (i=0;i<max_assigned-max_value; i++)
    printf("x=%lu,y=%lu\n",symbols[2*i],symbols[2*i+1]);*/
  ulong *bit_id_ver =(ulong *) malloc (sizeof(ulong)*(((this->n)/W)+1));
  for (i=0;  i< ((this->n)/W) ; i++) {
     bit_id_ver[i]=bit_id[i];
     bit_id[i]= (bit_id[i] >> 1) | ((bit_id[i+1] & 1) << 31);
  }
  bit_id_ver[(this->n)/W]=bit_id[(this->n)/W];
  bit_id[(this->n)/W]= (bit_id[(this->n)/W] >> 1) | (1<<31);

  for (i = 0 ; i < n-1 ; i++)
    if ( bitget(bit_id,i) != bitget(bit_id_ver,i+1))
    printf("%lu %lu\n", bitget(bit_id,i), bitget(bit_id_ver,i+1));
  n=n-3; //less the 3 extra symbols
  BR=new BitRankW32Int(bit_id, n, true, 20);
  this->m=BR->rank(n-1);
  symbols = (ulong*) malloc(sizeof(ulong)*this->m);
  for (i =0 ; i< m ; i++)
    symbols[i]=0;
  
  /* Valid symbols on data */
  p=0;
  for (i =0 ; i< n ; i++) 
    if (bitget(bit_id,i)==1) {
      symbols[p]=data[i+1];
      p++;
    }

  free(data);
  return 0;
}

void BPE::compress_pair_table() {
  ulong aux,j,i;
  /* Compress table of pairs */

  ulong *symbols_pair_tmp = (ulong*) malloc(sizeof(ulong)*(max_assigned-max_value));
  for (i =0 ; i< (max_assigned-max_value) ; i++) 
    symbols_pair_tmp[i]=0;

  for (i =0 ; i< (max_assigned-max_value) ; i++) {
    aux=symbols_pair[2*i];
    if (aux > max_value) {
      symbols_pair_tmp[aux-max_value-1]++;
    }
    aux=symbols_pair[2*i+1];
    if (aux > max_value) {
      symbols_pair_tmp[aux-max_value-1]++;
    }
  }
  j=0;
  for (i =0 ; i< (max_assigned-max_value); i++) {
    if (symbols_pair_tmp[i] != 0) 
      j++;
  }
 
  symbols_new_len = 2*(max_assigned-max_value)-j;
  symbols_new = (ulong*) malloc(sizeof(ulong)*(symbols_new_len));
  symbols_new_bit = (ulong*) malloc(sizeof(ulong)*((symbols_new_len+(max_assigned-max_value))/W+1));
  ulong *symbols_new_value = (ulong*) malloc(sizeof(ulong)*(max_assigned-max_value));
 
  for (i =0 ; i<((symbols_new_len+(max_assigned-max_value))/W+1);i++)
    symbols_new_bit[i]=0;
  for (i =0 ; i<symbols_new_len;i++)
    symbols_new[i]=0;
  for (i =0 ; i<(max_assigned-max_value);i++)
    symbols_new_value[i]=0;
  j=1;
  ulong k1=0;
  for (i =0 ; i< (max_assigned-max_value) ; i++) {
    if (symbols_pair_tmp[i] == 0) {
      symbols_new_value[i]=j; bitset(symbols_new_bit,j-1); j++;
      new_value(symbols_pair,symbols_new_value,&k1,&j,i);
    }
  }
  symbols_new_bit_len = j;
  ulong *symbols_new_bit_aux = new ulong [(symbols_new_bit_len/W+1)];

  for (i =0 ; i<symbols_new_bit_len/W+1;i++) {
    symbols_new_bit_aux[i]= symbols_new_bit[i];
  }
  free(symbols_new_bit);
  symbols_new_bit=symbols_new_bit_aux;

  /* Solo para verificar */
  //for (i =0 ; i<(max_assigned-max_value);i++)
    //if (symbols_new_value[i]==0) printf("s[%lu]=%lu, frec=%lu\n",i,symbols_new_value[i], symbols_pair_tmp[i]);
  
  free(symbols_pair_tmp);
  ulong cuentame=0,cuentame2=0,distancia=0;
  double maximo=0;
  double maxima_dist=0;
  this->nbits=bits(symbols_new_bit_len+max_value+1);
  ulong *symbols_aux = (ulong *) malloc(sizeof(ulong)*((m/W)*nbits+((m%W)*nbits)/W+1));// sizeof(ulong)*((m*nbits)/W+1));
  for (i =0 ; i< m ; i++) {
    aux = symbols[i];
    if (aux > max_value) {
      SetField(symbols_aux,nbits,i,symbols_new_value[aux-max_value-1]+max_value);
      //symbols[i] = symbols_new_value[aux-max_value-1]+max_value;
      if (maxima_dist < BR->select(i+2)-BR->select(i+1)) maxima_dist = BR->select(i+2)-BR->select(i+1);
      distancia=BR->select(i+2)-BR->select(i+1);
      maximo+=distancia*distancia;
      cuentame2++;
    } else {
      SetField(symbols_aux,nbits,i,aux);
      maximo++;
      cuentame++;
    }
  }
  free(symbols);
  symbols=symbols_aux;

  /* Compact symbols_new */
  /*
  symbols_aux = (ulong *) malloc(sizeof(ulong)*((symbols_new_len*nbits)/W+1));
  for (i =0 ; i< symbols_new_len ; i++) {
    SetField(symbols_aux,nbits,i,symbols_new[i]);
  }
  free(symbols_new);
  symbols_new=symbols_aux;
  */
            
  free(symbols_new_value); 
  free(symbols_pair); 
  BRR=new BitRankW32Int(symbols_new_bit, symbols_new_bit_len , true, 20); 
  if (verbose) {
    printf("Sin Comprimir=%lu, comprimido=%lu\n",cuentame,cuentame2); 
    printf("Costo prom descompr=%f,Costo max descompr=%f\n",maximo/(float)n, maxima_dist);
    printf("Original=%lu\n",n); 
    printf("Simpolos finales=%lu (%.6f%%)\n",m,m/(float)n*100);
    printf("Conjunto de pares=%lu (%.6f%%) hubo %lu reemplazos\n",2*(max_assigned-max_value),(2*(max_assigned-max_value))/(float)n*100,(max_assigned-max_value));
    aux=symbols_new_bit_len/W+1;
    printf("Conjunto de pares2=%lu+%lu (%.6f%%) ahorro  %lu\n",symbols_new_len,aux,(symbols_new_len+aux)/(float)n*100,2*(max_assigned-max_value)-symbols_new_len-aux);
    printf("Estructura para rank para descomprimir=%lu (%.6f%%) tira de bits+5%% extra \n",((this->n)/W)+1+(((this->n)/W)+1)*5/100,(((this->n)/W)+1+(((this->n)/W)+1)*5/100)/(float)n*100);
    printf("Sumas simbolos+pares=%lu (%.6f%%)\n",m+2*(max_assigned-max_value),(m+2*(max_assigned-max_value))/(float)n*100);
    printf("Sumas simbolos+pares2=%lu (%.6f%%)\n",m+symbols_new_len+aux,(m+symbols_new_len+aux)/(float)n*100);
  }
}

double BPE::avg_cost() {
  ulong i ,distancia=0,aux;
  double maximo=0;
  double maxima_dist=0;

  for (i =0 ; i< m ; i++) {
    aux =  GetField(symbols,nbits,i);
    if (aux > max_value) {
      if (maxima_dist < BR->select(i+2)-BR->select(i+1)) maxima_dist = BR->select(i+2)-BR->select(i+1);
      distancia=BR->select(i+2)-BR->select(i+1);
      maximo+=distancia*distancia;
    } else {
      maximo++;
    }
  }
  return maximo/(double)n ;
}

void  BPE::new_value(ulong *symbols_pair,ulong *symbols_new_value,ulong *k1,ulong *j,ulong pos) {
  ulong izq,der;
  izq=symbols_pair[2*pos];
  der=symbols_pair[2*pos+1];

  if (izq>max_value) {
    izq=izq-max_value-1;
    if (symbols_new_value[izq] == 0) {
      symbols_new_value[izq]=*j; bitset(symbols_new_bit,*j-1); (*j)++;
      new_value(symbols_pair,symbols_new_value,k1,j,izq);
    } else {
      symbols_new[*k1]=symbols_new_value[izq]+max_value; (*j)++; (*k1)++;
    }
  } else {
    symbols_new[*k1]=izq;(*j)++;(*k1)++;
  }

  if (der>max_value) {
    der=der-max_value-1;
    if (symbols_new_value[der] == 0) {
      symbols_new_value[der]=*j; bitset(symbols_new_bit,*j-1); (*j)++;
      new_value(symbols_pair,symbols_new_value,k1,j,der);
    } else {
      symbols_new[*k1]=symbols_new_value[der]+max_value; (*j)++; (*k1)++;
    }
  } else {
    symbols_new[*k1]=der;(*j)++;(*k1)++;
  }

}


BPE::BPE(ulong *a, ulong _n, ulong _max_phrase, double _cutoff, bool _verbose) {
  verbose=_verbose;
  data=a;
  NUM_MAX_RULES=(_n/2)/32;
  this->max_phrase = _max_phrase;
  if (verbose) printf("Largo max frase = %lu\n", max_phrase);
  if (verbose) printf("Tamaño max diccionario = %f\n", _cutoff);
  this->n=_n;
  naux=_n-1;
  if (verbose) { printf("Shift:\n"); fflush(stdout); }
  shift_it();
  if (verbose) { printf("Punteros:\n"); fflush(stdout); }
  pointer();
  if (verbose) { printf("Ini Treap:\n"); fflush(stdout); }
  treapme();
  if (verbose) { printf("Compress:\n"); fflush(stdout); }
  compress(_cutoff);
  if (verbose) { printf("Compress pair table:\n"); fflush(stdout); }
  compress_pair_table();
}



ulong *BPE::dispair(ulong i, ulong len) {
  //if (i==31036)
  //  printf("breakme\n");
  ulong pos1,pos2,pos3,pos4,pos5,pos6,end,j,*info=NULL,aux;
  pos1=BR->prev(i);
  pos2=BR->next(i+1);
  end=min(n-1,i+len-1);
  if (i>end) return info;
  aux=BR->rank(pos1)-1;
  ulong *base=case1(aux,pos2-pos1,i-pos1);
  if (end <= pos2-1) {
    info = (ulong*) malloc(sizeof(ulong)*(end-pos1+1+1+1)); // An extra symbol at the end for decompresion
    info[0] = end-i+1; //number of symbols
    for (j=1+i-pos1; j <= end-pos1+1; j++) 
       info[j-i+pos1]=base[j-1];
    free(base);
    return info;
  } 
  info = (ulong*) malloc(sizeof(ulong)*(end-pos1+1+1+1));  // An extra symbol at the end for decompresion
  info[0] = end-i+1; //number of symbols
  for (j=1+i-pos1; j <= pos2-1-pos1+1; j++) 
    info[j-i+pos1]=base[j-1];
  free(base);

  pos3=BR->prev(end);
  pos4=BR->next(end+1);

  aux=BR->rank(pos3)-1;
  base=case2(aux,pos4-pos3);

  for (j=pos3; j <=end ; j++) 
    info[j-i+1]=base[j-pos3];
  free(base);
  if (pos2 == pos3) 
    return info;

  pos5=pos2;  
  pos6=BR->prev(pos3-1);
  ulong aux1,aux2;
  aux1=BR->rank(pos5)-1;
  aux2=BR->rank(pos6)-1;
  caso3(aux1,aux2,pos2-i+1,pos3-i,info);
  return info;

}

ulong *BPE::dispair2(ulong i, ulong len) {
  ulong aux1,aux2,*info;
  aux1=BR->rank(i)-1;
  aux2=BR->rank(i+len-1)-1;
  assert(BR->IsBitSet(i+len));
  info = (ulong*) malloc(sizeof(ulong)*(len+2)); //one symbol at the beginning (length) and one at the end for decompresion
  info[0]=len;
  caso3(aux1,aux2,1,len,info);
  return info;
}


void BPE::caso3(ulong r1, ulong r2, ulong start, ulong end, ulong *info) {
  ulong j,i,aux,k,aux2,aux1,rank_aux;
  i=end;
  j=r2;
  while (j+1>=r1+1) {
    aux=GetField(symbols,nbits,j); //symbols[j];
    k=start;
    j--;
    if (aux <=max_value) {
      info[i]=aux+shift;
      i--; 
    } else {
      k=start;
      aux1=aux-max_value-1;
      assert(BRR->IsBitSet(aux1));
      aux2=1;
      rank_aux=aux1-BRR->rank(aux1)+1;
      while (aux2!=0) {
        if (BRR->IsBitSet(aux1))
          aux2++;
        else {
          aux2--;
          info[k]=symbols_new[rank_aux];
          rank_aux++;
          k++;
        }
        aux1++;
      }
    }
    while (k > start) {
      aux=info[k-1];
      if (aux <=max_value) {
        info[i]=aux+shift;
        i--;k--; 
      } else {
        aux1=aux-max_value-1;
        assert(BRR->IsBitSet(aux1));
        aux2=1;
        rank_aux=aux1-BRR->rank(aux1)+1;
        while (aux2!=0) {
          if (BRR->IsBitSet(aux1))
            aux2++;
          else {
            aux2--;
            info[k-1]=symbols_new[rank_aux];
            rank_aux++;
            k++; 
          }
          aux1++;
        }
        k--;
      }
    }
  }
}


ulong *BPE::case2(ulong pos1, ulong pos2) {
  ulong *info = (ulong*) malloc(sizeof(ulong)*pos2);
  ulong i,aux,k,aux2,aux1,rank_aux;
  i=pos2-1;
    aux=GetField(symbols,nbits,pos1); //symbols[pos1];
    k=0;
    if (aux <=max_value) {
      info[i]=aux+shift;
      i--; 
    } else {
      k=0;
      aux1=aux-max_value-1;
      assert(BRR->IsBitSet(aux1));
      aux2=1;
      rank_aux=aux1-BRR->rank(aux1)+1;
      while (aux2!=0) {
        if (BRR->IsBitSet(aux1))
          aux2++;
        else {
          aux2--;
          info[k]=symbols_new[rank_aux];
          rank_aux++;
          k++;
        }
        aux1++;
      }
    }
    while (k > 0) {
      aux=info[k-1];
      if (aux <=max_value) {
        info[i]=aux+shift;
        i--;k--; 
      } else {
        aux1=aux-max_value-1;
        assert(BRR->IsBitSet(aux1));
        aux2=1;
        rank_aux=aux1-BRR->rank(aux1)+1;
        while (aux2!=0) {
          if (BRR->IsBitSet(aux1))
            aux2++;
          else {
            aux2--;
            info[k-1]=symbols_new[rank_aux];
            rank_aux++;
            k++; 
          }
          aux1++;
        }
        k--;
      }
    }
  return info;
}

ulong *BPE::case1(ulong pos1, ulong pos2, ulong cut) {
  ulong *info = (ulong*) malloc(sizeof(ulong)*pos2);
  ulong i,aux,k,aux2,aux1,rank_aux;
  i=pos2-1;
    aux=GetField(symbols,nbits,pos1); //symbols[pos1];
    k=0;
    if (aux <=max_value) {
      info[i]=aux+shift;
      if (cut == i) return info;
      i--; 
    } else {
      k=0;
      aux1=aux-max_value-1;
      assert(BRR->IsBitSet(aux1));
      aux2=1;
      rank_aux=aux1-BRR->rank(aux1)+1;
      while (aux2!=0) {
        if (BRR->IsBitSet(aux1))
          aux2++;
        else {
          aux2--;
          info[k]=symbols_new[rank_aux];
          rank_aux++;
          k++;
        }
        aux1++;
      }
    }
    while (k > 0) {
      aux=info[k-1];
      if (aux <=max_value) {
        info[i]=aux+shift;
        if (cut == i) return info;
        i--;k--; 
      } else {
        aux1=aux-max_value-1;
        assert(BRR->IsBitSet(aux1));
        aux2=1;
        rank_aux=aux1-BRR->rank(aux1)+1;
        while (aux2!=0) {
          if (BRR->IsBitSet(aux1))
            aux2++;
          else {
            aux2--;
            info[k-1]=symbols_new[rank_aux];
            rank_aux++;
            k++; 
          }
          aux1++;
        }
        k--;
      }
    }
  return info;
}

void BPE::des1(ulong pos1, ulong pos2, ulong *info) {
  //ulong *info = (ulong*) malloc(sizeof(ulong)*pos2);
  ulong i,aux,k,aux2,aux1,rank_aux;
  i=pos2-1;
    aux=GetField(symbols,nbits,pos1); //symbols[pos1];
    k=0;
    if (aux <=max_value) {
      info[i]=aux+shift;
      i--; 
    } else {
      k=0; 
      aux1=aux-max_value-1;
      assert(BRR->IsBitSet(aux1));
      aux2=1;
      rank_aux=aux1-BRR->rank(aux1)+1;
      while (aux2!=0) {
        if (BRR->IsBitSet(aux1))
          aux2++;
        else {
          aux2--;
          info[k]=symbols_new[rank_aux];
          rank_aux++; 
          k++;
        }
        aux1++;
      }
    }
    while (k > 0) {
      aux=info[k-1];
      if (aux <=max_value) {
        info[i]=aux+shift;
        i--;k--; 
      } else {
        aux1=aux-max_value-1;
        assert(BRR->IsBitSet(aux1));
        aux2=1;
        rank_aux=aux1-BRR->rank(aux1)+1;
        while (aux2!=0) {
          if (BRR->IsBitSet(aux1))
            aux2++;
          else {
            aux2--;
            info[k-1]=symbols_new[rank_aux];
            rank_aux++;
            k++;
          }
          aux1++;
        }
        k--;
      }
    }
}

ulong *BPE::dispairall() {
  ulong *info = (ulong*) malloc(sizeof(ulong)*this->n);
  ulong j,i,aux,k,aux2,aux1,rank_aux;
  i=n-1;
  j=m-1;
  while (j+1>0) {
    aux=GetField(symbols,nbits,j); //symbols[j];
    k=0;
    j--;
    if (aux <=max_value) {
      info[i]=aux+shift;
      i--; 
    } else {
      k=0;
      aux1=aux-max_value-1;
      assert(BRR->IsBitSet(aux1));
      aux2=1;
      rank_aux=aux1-BRR->rank(aux1)+1;
      while (aux2!=0) {
        if (BRR->IsBitSet(aux1))
          aux2++;
        else {
          aux2--;
          info[k]=symbols_new[rank_aux];
          rank_aux++;
          k++;
        }
        aux1++;
      }
    }
    while (k > 0) {
      aux=info[k-1];
      if (aux <=max_value) {
        info[i]=aux+shift;
        i--;k--; 
      } else {
        aux1=aux-max_value-1;
        assert(BRR->IsBitSet(aux1));
        aux2=1;
        rank_aux=aux1-BRR->rank(aux1)+1;
        while (aux2!=0) {
          if (BRR->IsBitSet(aux1))
            aux2++;
          else {
            aux2--;
            info[k-1]=symbols_new[rank_aux];
            rank_aux++;
            k++; 
          }
          aux1++;
        }
        k--;
      }
    }
  }
  return info;
}

int BPE::save(FILE *f) {
  if (f == NULL) return 20;
  if (fwrite (&n,sizeof(ulong),1,f) != 1) return 21;
  if (fwrite (&m,sizeof(ulong),1,f) != 1) return 21;
  if (fwrite (&max_value,sizeof(ulong),1,f) != 1) return 21;
  if (fwrite (&max_assigned,sizeof(ulong),1,f) != 1) return 21;
  if (fwrite (&shift,sizeof(ulong),1,f) != 1) return 21;
  if (fwrite (&nbits,sizeof(ulong),1,f) != 1) return 21;
  if (fwrite (symbols,sizeof(ulong),((m/W)*nbits+((m%W)*nbits)/W+1),f) != ((m/W)*nbits+((m%W)*nbits)/W+1)) return 21;
//  if (fwrite (symbols,sizeof(ulong),((m*nbits)/W+1),f) != ((m*nbits)/W+1)) return 21;
  if (fwrite (&symbols_new_len,sizeof(ulong),1,f) != 1) return 21;
  //if (fwrite (symbols_new,sizeof(ulong),(symbols_new_len*nbits)/W+1,f) != (symbols_new_len*nbits)/W+1) return 21;
  for (ulong i=0;i < symbols_new_len; i++) 
    if (fwrite (symbols_new+i,sizeof(ulong),1,f) != 1) return 21;
  //if (fwrite (symbols_new,sizeof(ulong),symbols_new_len,f) != symbols_new_len) return 21;
  if (BR->save(f) !=0) return 21;
  if (BRR->save(f) !=0) return 21;
  return 0;
}

int BPE::load(FILE *f) {
  int error;
  if (f == NULL) return 23;
  if (fread (&n,sizeof(ulong),1,f) != 1) return 25;
  if (fread (&m,sizeof(ulong),1,f) != 1) return 25;
  if (fread (&max_value,sizeof(ulong),1,f) != 1) return 25;
  if (fread (&max_assigned,sizeof(ulong),1,f) != 1) return 25;
  if (fread (&shift,sizeof(ulong),1,f) != 1) return 25;
  if (fread (&nbits,sizeof(ulong),1,f) != 1) return 25;
  symbols = (ulong *) malloc(sizeof(ulong)*((m/W)*nbits+((m%W)*nbits)/W+1));
  if (!symbols) return 1;
  if (fread (symbols,sizeof(ulong),((m/W)*nbits+((m%W)*nbits)/W+1),f) != ((m/W)*nbits+((m%W)*nbits)/W+1)) return 25;

  //symbols = (ulong *) malloc(sizeof(ulong)*((m*nbits)/W+1)); 
  //if (!symbols) return 1;
  //if (fread (symbols,sizeof(ulong),((m*nbits)/W+1),f) != ((m*nbits)/W+1)) return 25;
  if (fread (&symbols_new_len,sizeof(ulong),1,f) != 1) return 25;
  //symbols_new = (ulong *) malloc(sizeof(ulong)*(symbols_new_len*nbits)/W+1); 
  symbols_new = (ulong *) malloc(sizeof(ulong)*symbols_new_len); 
  if (!symbols_new) return 1;
  //if (fread (symbols_new,sizeof(ulong),(symbols_new_len*nbits)/W+1,f) != (symbols_new_len*nbits)/W+1) return 25;
  if (fread (symbols_new,sizeof(ulong),symbols_new_len,f) != symbols_new_len) return 25;
  BR = new BitRankW32Int(f,&error);
  if (error !=0) return error;
  BRR = new BitRankW32Int(f,&error);
  if (error !=0) return error;
  return 0;
}

BPE::BPE(FILE *f, int *error) {
  *error = BPE::load(f);
}

BPE::~BPE() {
  free(symbols);
  free(symbols_new);
  delete BR;
  delete BRR;
}

ulong BPE::SpaceRequirement(){
  ulong size=0;
  size+=BR->SpaceRequirement();
  size+=BRR->SpaceRequirement();
  size+=(m/8)*nbits+((m%8)*nbits)/8; //m*nbits/8; // Compressed symbols;
  size+=symbols_new_len*(W/8); //*nbits/8; // table to decompress de new codes
  size+=sizeof(BPE);
  return size;
}

BPE::BPE(ulong *sa_diff,ulong *_Psi,ulong _ini,ulong length, ulong _max_phrase, double _cutoff, ulong s, ulong delta, ulong gamma, bool _verbose){
  /* Global variables */
  verbose=_verbose;
  data=sa_diff;
  Psi=_Psi;
  Psi0=_ini;
  data=sa_diff;
  naux=length-1;
  NUM_MAX_RULES=(length/2)/32;
  this->max_phrase = _max_phrase;
  if (verbose) printf("Largo max frase = %lu\n", max_phrase);
  if (verbose) printf("Tamaño max diccionario = %f\n", _cutoff);

  this->n=length;
  if (verbose) { printf("Shift:\n"); fflush(stdout); }
  shift_it();
  if (verbose) { printf("Inicio variables:\n"); fflush(stdout); }
  ini_psi();   
  if (verbose) { printf("Compress:\n"); fflush(stdout); }
  compress_psi2(_cutoff,s,delta,gamma);
  if (verbose) { printf("Compress pair table:\n"); fflush(stdout); }
  compress_pair_table();
}

int BPE::ini_psi(){
  bit_id = new ulong[((this->n)/W)+2];
  if (!bit_id) return 1;
  for (ulong i=0; i< ((this->n)/W)+2 ; i++)
    bit_id[i] = (ulong) -1; //0xFFFFFFFF; // 32 ones
  return 0;
}

int BPE::getpar (ulong pos, ulong *a, ulong *b, ulong *bpos) {
  if (pos>=this->n-2) return 0;
  if (bitget(bit_id,pos) == 0) return 0;
  assert(bitget(bit_id,pos) == 1);
  *bpos=next(pos+1);
  if ((*bpos % max_phrase) == 0) return 0;
  if (*bpos>=this->n-1) return 0;
  *a=data[pos];
  *b=data[*bpos];
   return 1;
}


void BPE::compress_psi(){
  ulong pos,a,b,bpos,pos1,bpos1,na,nb;
  ulong it;
  ulong LEN=this->n;
  mynodo *elem;
  mylist *reemplazos;
  reemplazos = new mylist();
  bool salir; ulong vuelta;

  pos = Psi0;
  while (!getpar(pos,&a,&b,&bpos)) pos = Psi[pos];
  while (1) { 
    pos1 = pos; bpos1 = bpos;
    salir=true; vuelta = pos;
    while (1) {
      pos = Psi[pos];
      if ((pos==vuelta) && salir) break;
      while (!getpar(pos,&na,&nb,&bpos)) pos = Psi[pos]; 
      if (Psi[pos1]!=pos) Psi[pos1]=pos;
      if ((a == na) && (b == nb) && (bpos1 != pos) && (bpos != pos1) ) {
        max_assigned++;
        data[pos1] = max_assigned; bitclean(bit_id,bpos1); LEN--;
        data[pos]  = max_assigned; bitclean(bit_id,bpos); LEN--;
        salir = false;
        reemplazos->push(a,b);
        if (!((max_assigned-max_value-1) % 100000)) {
          it = max_assigned-max_value-1;
          if (verbose) printf ("it = %lu, size = %lu, total = %lu (%f%%)\n",it,LEN,LEN+2*it,
                  (LEN+2*it)/(float)this->n*100);
                  fflush(stdout);
        }
        break;
      } 
      pos1 = pos; bpos1 = bpos; a = na; b = nb;
    }
    if (salir) break;
    while (1) {
      pos1=pos;
      pos = Psi[pos];
      while (!getpar(pos,&na,&nb,&bpos)) pos = Psi[pos];
      if (Psi[pos1]!=pos) Psi[pos1]=pos;
      if ((a != na) || (b != nb) ) break;
      data[pos] = max_assigned; bitclean(bit_id,bpos); LEN--; salir=false;
    }
    a = na; b = nb;
  }
  
  it = max_assigned-max_value-1;
  if (verbose) printf ("it = %lu, size = %lu, total = %lu (%f%%)\n",it, LEN,LEN+2*it,
          (LEN+2*it)/(float)this->n*100);
          fflush(stdout);
  free(Psi);
  ulong j=max_assigned,i,p;
  symbols_pair = (ulong*) malloc(sizeof(ulong)*2*(max_assigned-max_value));
  while (!reemplazos->empty()) {
    elem = reemplazos->pop();
    symbols_pair[2*(j-max_value-1)]=elem->v1;
    symbols_pair[2*(j-max_value-1)+1]=elem->v2;
    j--;
    free(elem);
  }
  assert(j==max_value);
  delete reemplazos;


  BR=new BitRankW32Int(bit_id, n, true, 20);
  this->m=BR->rank(n-1);
  symbols = (ulong*) malloc(sizeof(ulong)*this->m);
  for (i =0 ; i< m ; i++)
    symbols[i]=0;

  /* Valid symbols on data */
  p=0;
  for (i =0 ; i< n ; i++)
    if (bitget(bit_id,i)==1) {
      symbols[p]=data[i];
      p++;
    }

  free(data);

}


void BPE::compress_psi2(double cutoff,ulong s,ulong delta, ulong gamma){
  ulong pos,a,b,bpos,pos1,bpos1,na,nb;
  ulong it;
  ulong it2=0;
  ulong LEN=this->n;
  mynodo *elem;
  mylist *reemplazos;
  reemplazos = new mylist();
  bool salir; ulong vuelta;


  double paraaquinomas=0,porcentajedeavance1=1;
  symbols_new_frec = (ulong *) malloc(sizeof(ulong)*(NUM_MAX_RULES));
  for (ulong i=0;i<NUM_MAX_RULES;i++) symbols_new_frec[i]=0;

  pos = Psi0;
  ulong threshold=0, frecuencia=0;
  ulong iter=0;
  while (!getpar(pos,&a,&b,&bpos)) pos = Psi[pos];
  while (1) {
    pos1 = pos; bpos1 = bpos;
    salir=true; vuelta = pos;
    while (1) {
      pos = Psi[pos];
      if ((pos==vuelta) && salir) break;
      while (!getpar(pos,&na,&nb,&bpos)) pos = Psi[pos];
      iter++;
      if (Psi[pos1]!=pos) Psi[pos1]=pos;
      if ((a == na) && (b == nb) && (bpos1 != pos) && (bpos != pos1) ) {
        salir = false;
        break;
      }
      pos1 = pos; bpos1 = bpos; a = na; b = nb;
    }
    if (salir) break;
    frecuencia=2;
    while (1) {
      pos1= pos;
      pos = Psi[pos];
      while (!getpar(pos,&na,&nb,&bpos)) pos = Psi[pos]; 
      iter++;
      if (Psi[pos1]!=pos) Psi[pos1]=pos;
      if ((a != na) || (b != nb) ) break;
      frecuencia++;
      salir=false;
    }
    if (frecuencia > threshold) threshold=frecuencia;
    a = na; b = nb;
    if (iter>3*this->n) break;
  }

  printf ("threshold = %lu, delta=1-1/%lu, gamma=%lu\n", threshold,delta,gamma);
  pos = Psi0;
  ulong contador=0;
  ulong first_pos,last_pos,first_bpos,vuelta2=-1;
  while (!getpar(pos,&a,&b,&bpos)) pos = Psi[pos];
  //bool reemplazo=false;
  //bool reemplazo2=false;
  iter = 0;
  while (1) { 
    pos1 = pos; bpos1 = bpos;
    salir=true; vuelta = pos;
    while (1) {
      pos = Psi[pos];
      if ((pos==vuelta) && salir) break;
      //if ((pos==vuelta2)) {reemplazo=false;reemplazo2=true;}
      //if ((pos==vuelta2) && reemplazo2) {
       // threshold=min(threshold-1,threshold-threshold/20);
        //reemplazo2=false;
        //printf ("threshold = %lu\n", threshold);
      //}
      while (!getpar(pos,&na,&nb,&bpos)) pos = Psi[pos]; 
      iter ++;
      if (Psi[pos1]!=pos) Psi[pos1]=pos;
      if ((a == na) && (b == nb) && (bpos1 != pos) && (bpos != pos1) ) {
        //max_assigned++;
        //data[pos1] = max_assigned; bitclean(bit_id,bpos1); LEN--;
        //data[pos]  = max_assigned; bitclean(bit_id,bpos); LEN--;
        salir = false;
        //reemplazos->push(a,b);
        //if (!((max_assigned-max_value-1) % 100000)) {
          //it = max_assigned-max_value-1;
          //if (verbose) printf ("it = %lu, size = %lu, total = %lu (%f%%)\n",it,LEN,LEN+2*it,
                  //(LEN+2*it)/(float)this->n*100);
                  //fflush(stdout);
        //}
        break;
      } 
      pos1 = pos; bpos1 = bpos; a = na; b = nb;
    }
    if (salir) break;
    first_pos=pos1; first_bpos=bpos1;
    pos=pos1;
    frecuencia=1;
    while (1) {
      pos1=pos;
      pos = Psi[pos];
      while (!getpar(pos,&na,&nb,&bpos)) pos = Psi[pos];
      iter++;
      if (Psi[pos1]!=pos) Psi[pos1]=pos;
      if ((a != na) || (b != nb) ) break;
      frecuencia++;
      salir=false;
    }
    last_pos=pos;
    pos=first_pos;
    if (threshold <= frecuencia) {
      //printf("Frecuencia %lu t %lu\n",frecuencia,threshold);
      //iter=0;
      contador+=frecuencia;
      //reemplazo = true;
      max_assigned++;
      if (a > max_value)
        if (bitget(symbols_new_frec,a-max_value) == 0) {
          symbols_new_frec_val++;
          bitset(symbols_new_frec,a-max_value);
        }
      if (b > max_value)
        if (bitget(symbols_new_frec,b-max_value) == 0) {
          symbols_new_frec_val++;
          bitset(symbols_new_frec,b-max_value);
        }

      data[first_pos] = max_assigned; bitclean(bit_id,first_bpos); LEN--;
      salir = false;
      reemplazos->push(a,b);
      paraaquinomas=((3*(max_assigned-max_value)-symbols_new_frec_val)/W + 2*(max_assigned-max_value)-symbols_new_frec_val)/(float)n*100;
      //if (!((max_assigned-max_value-1) % 50000)) {
      if (paraaquinomas> porcentajedeavance1/20.0) {
        it = max_assigned-max_value-1;
        porcentajedeavance1++;
        if (verbose) printf ("it = %lu, size = %lu, total = %lu (%f%%), Compress Dic = %.6f%%\n",it,LEN,LEN+2*it,
                (LEN+2*it)/(float)this->n*100, paraaquinomas);
                fflush(stdout);
      }
      pos=first_pos;
      while (1) {
        pos1=pos;
        pos = Psi[pos];
        while (!getpar(pos,&na,&nb,&bpos)) pos = Psi[pos];
        if (Psi[pos1]!=pos) Psi[pos1]=pos;
        if ((a != na) || (b != nb) ) break;
        //if ( (bpos1 != pos) && (bpos != pos1)) {
          data[pos] = max_assigned; bitclean(bit_id,bpos); LEN--; salir=false;
        //}
      }
      vuelta2=pos;
    } else {
      pos=last_pos;
    }
 
    if (iter>LEN+threshold+100) {
      if (contador <= (LEN/(s*bits(n+1))) ) {
        if (threshold ==2) it2++;
        threshold=max(min(threshold-1,max(threshold-threshold/delta,gamma)),2);
        printf ("threshold = %lu, contador=%lu, LEN=%lu, logn=%d, LEN/(%lu*logn)=%lu\n", threshold,contador,LEN,bits(n+1),s,(LEN/(s*bits(n+1))));
      } 
      contador=0;
      iter=0;
    } 
    a = na; b = nb;
    if ((paraaquinomas > cutoff) || (it2 >= gamma))  break;



  }
  
  it = max_assigned-max_value-1;
  if (verbose) printf ("it = %lu, size = %lu, total = %lu (%f%%)\n",it, LEN,LEN+2*it,
          (LEN+2*it)/(float)this->n*100);
          fflush(stdout);
  free(Psi);
  ulong j=max_assigned,i,p;
  symbols_pair = (ulong*) malloc(sizeof(ulong)*2*(max_assigned-max_value));
  while (!reemplazos->empty()) {
    elem = reemplazos->pop();
    symbols_pair[2*(j-max_value-1)]=elem->v1;
    symbols_pair[2*(j-max_value-1)+1]=elem->v2;
    j--;
    free(elem);
  }
  assert(j==max_value);
  delete reemplazos;


  BR=new BitRankW32Int(bit_id, n, true, 20);
  this->m=BR->rank(n-1);
  symbols = (ulong*) malloc(sizeof(ulong)*this->m);
  for (i =0 ; i< m ; i++)
    symbols[i]=0;

  /* Valid symbols on data */
  p=0;
  for (i =0 ; i< n ; i++)
    if (bitget(bit_id,i)==1) {
      symbols[p]=data[i];
      p++;
    }

  free(data);

}
