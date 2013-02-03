#include "bitrankw32int.h"
#include "assert.h"
#include "math.h"
#include <sys/types.h>


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

BitRankW32Int::BitRankW32Int( ulong *bitarray, ulong _n, bool owner, ulong _factor){
  data=bitarray;
  this->owner = owner;
  this->n=_n;
  ulong lgn=bits(n-1);
  this->factor=_factor;
  if (_factor==0) this->factor=lgn;
  else this->factor=_factor;
  b=32;
  s=b*this->factor;
  integers = n/W+1;
  BuildRank();
}

BitRankW32Int::~BitRankW32Int() {
  delete [] Rs;
  if (owner) delete [] data;
}

//Metodo que realiza la busqueda d
void BitRankW32Int::BuildRank(){
  ulong num_sblock = n/s;
  Rs = new ulong[num_sblock+5];// +1 pues sumo la pos cero
  for(ulong i=0;i<num_sblock+5;i++)
    Rs[i]=0;
  ulong j;
  Rs[0]=0;
  for (j=1;j<=num_sblock;j++) {
    Rs[j]=Rs[j-1];
    Rs[j]+=BuildRankSub((j-1)*factor,factor);
  }
}

ulong BitRankW32Int::BuildRankSub(ulong ini,ulong bloques){
  ulong rank=0,aux;
  for(ulong i=ini;i<ini+bloques;i++) {
    if (i < integers) {
      aux=data[i];
      rank+=popcount(aux);
    }
  }
  return rank; //retorna el numero de 1's del intervalo

}

ulong BitRankW32Int::rank(ulong i) {
  ++i;
  ulong resp=Rs[i/s];
  ulong aux=(i/s)*factor;
  for (ulong a=aux;a<i/W;a++)
    resp+=popcount(data[a]);
  resp+=popcount(data[i/W]  & ((1<<(i & mask31))-1));
  return resp;
}

bool BitRankW32Int::IsBitSet(ulong i) {
  return (1u << (i % W)) & data[i/W];
}

int BitRankW32Int::save(FILE *f) {
  if (f == NULL) return 20;
  if (fwrite (&n,sizeof(ulong),1,f) != 1) return 21;
  if (fwrite (&factor,sizeof(ulong),1,f) != 1) return 21;
  if (fwrite (data,sizeof(ulong),n/W+1,f) != n/W+1) return 21;
  if (fwrite (Rs,sizeof(ulong),n/s+1,f) != n/s+1) return 21;
  return 0;
}

int BitRankW32Int::load(FILE *f) {
  if (f == NULL) return 23;
  if (fread (&n,sizeof(ulong),1,f) != 1) return 25;
  b=32; // b is a word
  if (fread (&factor,sizeof(ulong),1,f) != 1) return 25;
  s=b*factor;
  ulong aux=(n+1)%W;
  if (aux != 0)
    integers = (n+1)/W+1;
  else
    integers = (n+1)/W;
  data= new ulong[n/W+1];
  if (!data) return 1;
  if (fread (data,sizeof(ulong),n/W+1,f) != n/W+1) return 25;
  this->owner = true;
  Rs= new ulong[n/s+1];
  if (!Rs) return 1;
  if (fread (Rs,sizeof(ulong),n/s+1,f) != n/s+1) return 25;
  return 0;
}

BitRankW32Int::BitRankW32Int(FILE *f, int *error) {
  *error = BitRankW32Int::load(f);
}

ulong BitRankW32Int::SpaceRequirementInBits() {
  return (owner?n:0)+(n/s)*sizeof(ulong)*8 +sizeof(BitRankW32Int)*8; 
}

ulong BitRankW32Int::SpaceRequirement() {
  return (owner?n:0)/8+(n/s)*sizeof(ulong)+sizeof(BitRankW32Int); 
}

ulong BitRankW32Int::prev2(ulong start) {
      // returns the position of the previous 1 bit before and including start.
      // tuned to 32 bit machine

      ulong i = start >> 5;
      int offset = (start % W);
      ulong answer = start;
      ulong val = data[i] << (Wminusone-offset);

      if (!val) { val = data[--i]; answer -= 1+offset; }

      while (!val) { val = data[--i]; answer -= W; }

      if (!(val & 0xFFFF0000)) { val <<= 16; answer -= 16; }
      if (!(val & 0xFF000000)) { val <<= 8; answer -= 8; }

      while (!(val & 0x80000000)) { val <<= 1; answer--; }
      return answer;
}

ulong BitRankW32Int::prev(ulong start) {
      // returns the position of the previous 1 bit before and including start.
      // tuned to 32 bit machine
  
      ulong i = start >> 5;
      int offset = (start % W);
      ulong aux2 = data[i] & (-1u >> (31-offset));
  
      if (aux2 > 0) {
                if ((aux2&0xFF000000) > 0) return i*W+23+prev_tab[(aux2>>24)&0xFF];
                else if ((aux2&0xFF0000) > 0) return i*W+15+prev_tab[(aux2>>16)&0xFF];
                else if ((aux2&0xFF00) > 0) return i*W+7+prev_tab[(aux2>>8)&0xFF];
                else  return i*W+prev_tab[aux2&0xFF]-1;
      }
      for (ulong k=i-1;;k--) {
         aux2=data[k];
         if (aux2 > 0) {
                if ((aux2&0xFF000000) > 0) return k*W+23+prev_tab[(aux2>>24)&0xFF];
                else if ((aux2&0xFF0000) > 0) return k*W+15+prev_tab[(aux2>>16)&0xFF];
                else if ((aux2&0xFF00) > 0) return k*W+7+prev_tab[(aux2>>8)&0xFF];
                else  return k*W+prev_tab[aux2&0xFF]-1;
         }
      }
      return 0;
} 

ulong BitRankW32Int::next(ulong k) {
        ulong count = k;
        ulong des,aux2;
        des=count%W; 
        aux2= data[count/W] >> des;
        if (aux2 > 0) {
                if ((aux2&0xff) > 0) return count+select_tab[aux2&0xff]-1;
                else if ((aux2&0xff00) > 0) return count+8+select_tab[(aux2>>8)&0xff]-1;
                else if ((aux2&0xff0000) > 0) return count+16+select_tab[(aux2>>16)&0xff]-1;
                else {return count+24+select_tab[(aux2>>24)&0xff]-1;}
        }

        for (ulong i=count/W+1;i<integers;i++) {
                aux2=data[i];
                if (aux2 > 0) {
                        if ((aux2&0xff) > 0) return i*W+select_tab[aux2&0xff]-1;
                        else if ((aux2&0xff00) > 0) return i*W+8+select_tab[(aux2>>8)&0xff]-1;
                        else if ((aux2&0xff0000) > 0) return i*W+16+select_tab[(aux2>>16)&0xff]-1;
                        else {return i*W+24+select_tab[(aux2>>24)&0xff]-1;}
                }
        }
        return n;
} 

ulong BitRankW32Int::select(ulong x) {
  // returns i such that x=rank(i) && rank(i-1)<x or n if that i not exist
  // first binary search over first level rank structure
  // then sequential search using popcount over a int
  // then sequential search using popcount over a char
  // then sequential search bit a bit

  //binary search over first level rank structure
  ulong l=0, r=n/s;
  ulong mid=(l+r)/2;
  ulong rankmid = Rs[mid];
  while (l<=r) {
    if (rankmid<x)
      l = mid+1;
    else
      r = mid-1;
    mid = (l+r)/2;
    rankmid = Rs[mid];
  }
  //sequential search using popcount over a int
  ulong left;
  left=mid*factor;
  x-=rankmid;
        ulong j=data[left];
        ulong ones = popcount(j);
        while (ones < x) {
    x-=ones;left++;
    if (left > integers) return n;
          j = data[left];
      ones = popcount(j);
        }
  //sequential search using popcount over a char
  left=left*b;
  rankmid = popcount8(j);
  if (rankmid < x) {
    j=j>>8;
    x-=rankmid;
    left+=8;
    rankmid = popcount8(j);
    if (rankmid < x) {
      j=j>>8;
      x-=rankmid;
      left+=8;
      rankmid = popcount8(j);
      if (rankmid < x) {
        j=j>>8;
        x-=rankmid;
        left+=8;
      }
    }
  }

  // then sequential search bit a bit
        while (x>0) {
    if  (j&1) x--;
    j=j>>1;
    left++;
  }
  return left-1;
}
