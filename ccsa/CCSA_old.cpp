/* CCSA.cpp
   Copyright (C) 2005, Rodrigo Gonzalez, all rights reserved.

   This file contains an implementation of a Compressed Compact Suffix Array (CCSA)
   Based on a version programmed by Veli Makinen.
   http://www.cs.helsinki.fi/u/vmakinen/software/
   For more information, see [MN2004a].

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
/*
   [MN2004a] M\"akinen, V. and Navarro, G.  Compressed compact suffix arrays. In Proc. 15th
             Annual Symposium on Combinatorial Pattern Matching (CPM), LNCS v. 3109 (2004), pp.
             420-433.

   This library uses a suffix array construction algorithm developed by
   Giovanni Manzini (Universit\uffff del Piemonte Orientale) and
   Paolo Ferragina (Universit\uffff di Pisa).
   You can find more info at:
   http://roquefort.di.unipi.it/~ferrax/SuffixArray/
*/


//---------------------------------------------------------------------------
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/times.h>
#include <assert.h>
#include <string.h>
#include "ds/ds_ssort.h"
#include "basic.h"
#include "bitarray.h"
//---------------------------------------------------------------------------

/* Three function to variables to manage parameters */
static bool is_delimeter(char *delimiters, char c) {
   int i=0,len_delimiters=strlen(delimiters);
   bool is=false;
   for (i=0;i<len_delimiters;i++)
     if (c == delimiters[i]) is=true;
   return is;
}

static void parse_parameters(char *options, int *num_parameters, char ***parameters, char *delimiters) {
  int i=0,j=0,temp=0,num=0, len_options=strlen(options);
  char *options_temp;
  while  (i<len_options) {
    while ((i<len_options) && is_delimeter(delimiters,options[i])) i++;
    temp=i;
    while ((i<len_options) && !is_delimeter(delimiters,options[i])) i++;
    if (i!=temp) num++;
  }
  (*parameters) = (char **) malloc(num*sizeof(char * ));
  i=0;
  while  (i<len_options) {
    while ((i<len_options) && is_delimeter(delimiters,options[i])) i++;
    temp=i;
    while ((i<len_options) && !is_delimeter(delimiters,options[i])) i++;
    if (i!=temp) {
      (*parameters)[j]=(char *) malloc((i-temp+1)*sizeof(char));
      options_temp = options+temp;
      strncpy((*parameters)[j], options_temp, i-temp);
      ((*parameters)[j])[i-temp] = '\0';
      j++;
    }
  }
  *num_parameters = num;
}

static void free_parameters(int num_parameters,char ***parameters) {
  int i=0;
  for (i=0; i<num_parameters;i++)
    free((*parameters)[i]);
  free((*parameters));
}

//---------------------------------------------------------------------------

class TSA {
  private:
    ulong posbits;
    ulong *pos;
    ulong *B;
    ulong n;
    ulong bitsn;
    int remap0(uchar *text, ulong n) {
      int i;
      ulong j;
      ulong Freq_old[size_uchar];
      for(i=0;i<size_uchar;i++)
        Freq_old[i]=0;
      for(j=0;j<n;j++) {
        Freq_old[text[j]]++;
      }
      i=-1;
      // remap alphabet
      if (Freq_old[0]==0) return 0; //test if some character of T is zero
      for(j=1;j<size_uchar;j++)
        if(Freq_old[j]==0) {
          i=j;
          break;
        }
      if (i == -1 ) return i;
      // remap text
      for(j=0;j<n;j++)
        if (text[j] == 0) text[j] = i;
      return i;
    }
  public:
    TSA(uchar *text, uchar **text_aux, ulong n, int *error,bool free_text, uchar *map0){
      this->n=n;
      ulong *sa;
      int overshoot = init_ds_ssort(500, 2000);
      *text_aux= (uchar *) malloc (sizeof(uchar)*(this->n+overshoot));
      if (!*text_aux) {*error=1;return;}
      for (ulong i=0;i<this->n-1;i++) (*text_aux)[i]=text[i];
      (*text_aux)[n-1] = 0;
      if (free_text) free(text);
      /* Remap 0 if exist */
      *map0=remap0((*text_aux),this->n-1); // the text contain a 0 at the end

      /* Make suffix array */
      sa= (ulong *) malloc (sizeof(ulong)*(this->n));
      if (!sa) {*error =1 ; return;}
      ds_ssort( (*text_aux), sa, this->n);
      bitsn = bits(n);
      CreatePosArray(bitsn,n);
      for (ulong i=0; i<n; i++) {
      SetPos(i,sa[i]);
      SetB(i,0);
      }
      free(sa);
      *error=0;
    }
    TSA(ulong bits, ulong n){
      this->n = n; posbits = bits; pos = new ulong[n*posbits/W+2];
      CreateBooleanArray(n);
    };
    void CreatePosArray(ulong bits, ulong n) {
    	this->n = n; posbits = bits; pos = new ulong[n*posbits/W+2];
      CreateBooleanArray(n);
    };
    void CreateBooleanArray(ulong n){
    	B = new ulong[n/W+2];
    };
    void DeletePosArray() {delete [] pos;};
    void DeleteBooleanArray() {delete [] B;};
    void SetPos(ulong index, ulong x){
    	SetField(pos,posbits,index,x);
    };
    void SetB(ulong index, ulong x){
    	SetField(B,1,index,x);
    };
    ulong GetPos(ulong index) {
    	return GetField(pos,posbits,index);
    };
    bool GetB(ulong index){
    	return GetField(B,1,index);
    };
    ~TSA() {
    	DeletePosArray();
    	DeleteBooleanArray();
    };
};

/**********************************************************************************************************
* Compressed Compact Suffix Array implementation                                                         *
**********************************************************************************************************/

class TCCSA {
  private:
    //Variables//
    ulong n;
    ulong bitsn;
    ulong posbits;
    ulong Htbits;
    ulong *pos;
    ulong *Ht;
    ulong length;
    ulong *chars;
    BitRankF *charrank;
    BitRankF *posrank;
    BitRankF *samplerank;
    ulong *sample;
    ulong samplebits;
    ulong samplerate; // suffixes 0,samplerate,2*samplerate,...., are sampled
    ulong samplen;
    uchar map0;

    //Functions//
    uchar *pattern0(uchar *pattern, ulong m) {
      uchar *pat0;
      pat0 = (uchar *) malloc (sizeof(uchar)*m);
      for (ulong j=0;j<m;j++) {
        if (pattern[j] == 0) pat0[j]=map0;
        else pat0[j]=pattern[j];
      }
      return pat0;
    }

    void CreateHtArray(ulong bitss, ulong samplenht){
    	Htbits = bitss; Ht = new ulong[(samplenht*bitss)/W+1];
    };
    void DeleteHtArray() {delete Ht;};
    void SetHt(ulong index, ulong x){
      SetField(Ht,Htbits,index,x);
    };
    ulong GetHt(ulong index){
      return GetField(Ht,Htbits,index);
    };
    void CreatePosArray(ulong bits, ulong n){
    	posbits = bits; pos = new ulong[(n*bits)/W+1];
    };
    void DeletePosArray() {delete pos;};
    void SetPos(ulong index, ulong x){
    	SetField(pos,posbits,index,x);
    };
    ulong GetPos(ulong index) {
    	return GetField(pos,posbits,index);
    };
    inline ulong GetLen(ulong index){
    	return posrank->select(index+2)-posrank->select(index+1);
    };
    void SetSample(bool *B) {
    ulong i;
    ulong *Binbits = new ulong[n/W+1];
    for (i=0;i<n/W+1;i++)
      Binbits[i]=0;
    for (i=0;i<n;i++)
    SetField(Binbits,1,i,B[i]);
    samplerank = new BitRankF(Binbits,n,true);
    };
    void CreateRankForChars(uchar *text, ulong n) {
      ulong i,j=0,sumlen=0,sumchars=0;
      chars = new ulong[size_uchar]; // chars[c] tells how many times character c repeats
      uchar *charatindex = new uchar[length]; // charatindex[i] tells which char is at entry i
      for (i=0;i<size_uchar; i++) chars[i]=0;
      for (i=0;i<n;i++)
        chars[text[i]]++;
      for (i=0;i<length;i++) {
        sumlen += GetLen(i);
        while (sumlen>sumchars+chars[j])
          sumchars += chars[j++];
        charatindex[i]=j;
      }
      bool *B=new bool[length];
      j=0; B[0]=1;
      chars[j++]=charatindex[0]; // chars[j] tells which is the jth distinct char
      for (i=1;i<length;i++) {
        if (charatindex[i]!=charatindex[i-1]) {
          B[i] = 1;
          chars[j++] = charatindex[i];
        } else
          B[i]=0;
      }
      delete [] charatindex;
      ulong *Binbits = new ulong[length/W+1];
      for (i=0;i<length;i++)
        SetField(Binbits,1,i,B[i]);
      charrank = new BitRankF(Binbits,length,true);
      //charrank->Test();
      delete [] B;
    };
    void CreateRankForPos(bool *B) {
      ulong i;
      ulong *Binbits = new ulong[(n+1)/W+2];
      for (i=0;i<(n+1)/W+2;i++)
        Binbits[i]=0;
      for (i=0;i<=n;i++)
        SetField(Binbits,1,i,B[i]);
      posrank = new BitRankF(Binbits,n+1,true);
      // caller can delete B
    };
    inline bool IsSampled(ulong x,ulong offset) {
      return samplerank->IsBitSet(posrank->select(x+1)+offset);
    };
    inline ulong GetTextPos(ulong i) {
      ulong depth=0;
      ulong x;
      while (!samplerank->IsBitSet(i)) {
        x = posrank->rank(i)-1;
        i = GetPos(x)+i-posrank->prev(i); //+posrank->select(x+1);
        depth++;
      }
      return GetField(sample,samplebits,samplerank->rank(i)-1)-depth;
    };
    void ShowPath(ulong i, ulong len) {
      int x,offset; ulong j;  uchar c;
      x = posrank->rank(i)-1; // CCSA position
      offset = i-posrank->prev(i); // offset = i-posrank->select(x+1);
      for (j=0;j<len;j++) {
        c = chars[charrank->rank(x)-1]; //CharAtIndex(x)
        if (c=='\n') printf("%c",'/');
        else printf("%c",c);
        i = GetPos(x)+offset; // suffix array position
        x = posrank->rank(i)-1; // CCSA position
        offset = i -posrank->prev(i);
      }
      printf("\n");
    };
    inline uchar CharAtIndex(ulong i) {
      return chars[charrank->rank(i)-1];
    };
    inline int CompareIndex(uchar *pattern, ulong m, ulong i) { // i is suffix array position
      // compares pattern[0...m-1] with the suffix
      int x,offset; ulong j; uchar c;
      x = posrank->rank(i)-1; // CCSA position
      offset = i-posrank->prev(i); // offset = i-posrank->select(x+1);
      for (j=0;j<m;j++) {
        c = chars[charrank->rank(x)-1]; //CharAtIndex(x)
        if (pattern[j]>c) return j+1;
        else if (pattern[j]<c) return -(j+1);
        /* c==pattern[j] */
        i = GetPos(x)+offset; // suffix array position
        x = posrank->rank(i)-1; // CCSA position
        offset = i -posrank->prev(i);
      }
      return 0;  // match
    };

    inline ulong ReportOccurrenceRegion(ulong left, ulong right, ulong depth) {
      // sa indexes left and right
      ulong r=right, prevsample=samplerank->prev(right), prevlink=posrank->prev(right);
      ulong x,y,i;
      while (prevsample >= left || prevlink >= left) {
        if (prevsample < prevlink) {
          // recursively report area prevlink...r
          x = posrank->rank(prevlink)-1;
          i = GetPos(x);
          ReportOccurrenceRegion(i,i+r-prevlink, depth+1);
          r = prevlink -1;
          prevlink = posrank->prev(r);
        } else {
          // recursively report area prevsample+1...r
          // report suffix of index prevsample
          if (prevsample<r) {
            i = prevsample+1;
            x = posrank->rank(i)-1;
            i = GetPos(x) + i- prevlink;
            ReportOccurrenceRegion(i,i+r-prevsample-1, depth+1);
          }
          y = GetField(sample,samplebits,samplerank->rank(prevsample)-1)-depth;
          r = prevsample -1;
          prevsample = samplerank->prev(r);
        }
      }
      if (r >= left) {
        // recursively report area left...r
        x = posrank->rank(left)-1;
        i = GetPos(x) + left-posrank->prev(left);
        ReportOccurrenceRegion(i,i+r-left, depth+1);
      }
      return 0;
    };

    inline ulong ReportOccurrence(ulong i, ulong *path, ulong *pathdepth) {
      ulong depth=0;
      ulong x;
      while (*pathdepth > depth && !samplerank->IsBitSet(i) && !posrank->IsBitSet(i))
        i = ++(path[depth++]);
      while (!samplerank->IsBitSet(i)) {
        x = posrank->rank(i)-1;
        i = GetPos(x)+i-posrank->prev(i); //posrank->select(x+1);
        path[depth++]=i;
      }
      *pathdepth = depth;
      return GetField(sample,samplebits,samplerank->rank(i)-1)-depth;
    };

    void ReportOccurrences(ulong l, ulong r) {
      ulong *path = new ulong[samplerate+1];
      ulong pathdepth = 0;
      ulong i, x;
      for (i=l;i<=r;i++) {
        x = ReportOccurrence(i,path,&pathdepth);
      }
      delete path;
    };

    void ObtainOccurrences(ulong l, ulong r, ulong **occ, ulong matches) {
      ulong *path = new ulong[samplerate+1];
      ulong pathdepth = 0;
      ulong i,j=0,x;
      *occ = (ulong *) malloc(matches*sizeof(ulong));
      for (i=l;i<=r;i++) {
        x = ReportOccurrence(i,path,&pathdepth);
        //ShowPath(i,30);
        (*occ)[j]=x;
        j++;
      }
      delete [] path;
    };
    ulong MarkLinksToSuffixArray(TSA *SArray, TSA *DArray, uchar *text, ulong n) {
      /* This is for making compressed compact suffix arrays */
      ulong newsize=n;
      ulong i,j,x,index;
      // Make DArray the inverse of SArray
      for (i=0; i<n; i++) {
        DArray->SetPos(SArray->GetPos(i),i);
        DArray->SetB(i,0);
        SArray->SetB(i,0);
      }
      // Swap SArray and DArray
      for (i=0; i<n; i++) {
        x = DArray->GetPos(i);
        DArray->SetPos(i,SArray->GetPos(i));
        SArray->SetPos(i,x);
      }
      i=0;
      /* Mark links */
      while (i<n) {
        x = SArray->GetPos((DArray->GetPos(i)+1) % n);
        DArray->SetB(i,0);
        ulong textpos=DArray->GetPos(i);
        DArray->SetPos(i,x);
        j = 1;
        while (x+j <n && i+j <n &&
               SArray->GetPos(((DArray->GetPos(i+j)+1) % n)) == x+j &&
               text[DArray->GetPos(i+j)] == text[textpos]) {// restrict a link to contain only one character
          DArray->SetB(i+j,1);
          DArray->SetPos(i+j,x+j);
          newsize--;
          j++;
        }
        i = i+j;
      }
      // SArray := inverse of SArray
      for (i=0; i<n; i++) {
        j = SArray->GetPos(i);
        x = i;
        while (SArray->GetB(j)==0) {
          index = j;
          j = SArray->GetPos(index);
          SArray->SetPos(index,x);
          SArray->SetB(index,1);
          x = index;
        }
      }
      return newsize;
    }

    void FromMarkedSAToCCSA(TSA *SArray, TCCSA *DArray, ulong n){
      ulong i=0;
      ulong j=0;
      bool *B = new bool[n+1];
      while (i<n) {
        DArray->SetPos(j++,SArray->GetPos(i));
        B[i++]=1;
        while (i<n && SArray->GetB(i))
          B[i++]=0;
      }
      B[i]=1;
      DArray->CreateRankForPos(B);
      delete [] B;
    }
  public:
    TCCSA(uchar *text, ulong _n, ulong _samplerate, bool free_text, int *error) {
      this->n=_n+1;
      uchar *text_aux;
      TSA *SA = new TSA(text,&text_aux,n,error,free_text, &map0);
      if (*error != 0) return;
      ulong newsize=n,i;
      this->samplerate=_samplerate;//bits(n);
      this->samplebits=bits(n);
      this->samplen = n/samplerate+1;
      this->sample = new ulong[(samplen*samplebits)/W+2];
      bool *B = new bool[n];
      ulong rank=0;
      /* Make sample of suffixes for reporting */
      CreateHtArray(samplebits,samplen);
      for (i=0;i<n;i++)
        if (SA->GetPos(i) % samplerate == 0 || SA->GetPos(i) == n-1) {
          SetField(sample,samplebits,rank,SA->GetPos(i));
          SetHt(SA->GetPos(i)/samplerate,i);
          B[i]=1;
          rank++;
        } else
          B[i]=0;
      SetSample(B);
      delete [] B;
      bitsn = bits(n);
      TSA *SATemp = new TSA(bitsn,n);
      newsize = MarkLinksToSuffixArray(SA, SATemp, text_aux, n);
      delete SA;
      length = newsize;
      CreatePosArray(bitsn,newsize+1);
      FromMarkedSAToCCSA(SATemp,this,n);
      delete SATemp;
      CreateRankForChars(text_aux,n);
      free(text_aux);
      *error = 0;
    }
    int count(uchar *pattern, ulong m) {
      uchar *pat=pattern;
      if (map0 != 0 )
        pat = pattern0(pattern,m);
      ulong L;
      ulong R,i,Llimit,Rlimit=n;
      ulong matches=0;
      // search for left limit
      L = 0;//-1;
      R = n;
      i = (L+R)/2;
      while (L<R-1) {
        if (CompareIndex(pat,m,i) <= 0)
          R = i;
        else
          L = i;
        i = (R+L)/2;
      }
      Llimit=R;
      // search for right limit
      L = Llimit-1;
      R = n;
      i = (L+R)/2;
      if (Llimit <  n) {
        while (L < R-1) {
          if (CompareIndex(pat,m,i) >= 0)
            L = i;
          else
            R = i;
          i = (R+L)/2;
          }
        Rlimit=L;
      }
      if (map0 != 0) free(pat);
      if (Llimit ==  n || Llimit > Rlimit)
        return 0;
      matches=Rlimit-Llimit+1;
      return matches; // counting
    };

    int locate(uchar *pattern, ulong m, ulong **occ) {
      uchar *pat=pattern;
      if (map0 != 0 )
        pat = pattern0(pattern,m);
      ulong L;
      ulong R,i,Llimit,Rlimit=n;
      ulong matches=0;
      // search for left limit
      L = 0; //-1;
      R = n;
      i = (L+R)/2;
      while (L<R-1) {
        if (CompareIndex(pat,m,i) <= 0)
          R = i;
        else
          L = i;
        i = (R+L)/2;
      }
      Llimit=R;
      // search for right limit
      L = Llimit-1;
      R = n;
      i = (L+R)/2;
      if (Llimit < n) {
      while (L < R-1) {
        if (CompareIndex(pat,m,i) >= 0)
          L = i;
        else
          R = i;
        i = (R+L)/2;
        }
        Rlimit=L;
      }
      if (map0 != 0) free(pat);
      if (Llimit == n || Llimit > Rlimit) {
        (*occ) = NULL;
        return 0;
      }
      matches=Rlimit-Llimit+1;
      //ReportOccurrenceRegion(Llimit,Rlimit,0);
      ObtainOccurrences(Llimit,Rlimit,occ,matches);
      return matches;
    };

    int display(uchar *pattern, ulong m, ulong numc, ulong *numocc, uchar **snippet_text, ulong **snippet_lengths) {
      uchar *pat=pattern;
      if (map0 != 0 )
        pat = pattern0(pattern,m);
      ulong L;
      ulong R,i,Llimit,Rlimit=n;
      // search for left limit
      L = 0; //-1;
      R = n;
      i = (L+R)/2;
      while (L<R-1) {
        if (CompareIndex(pat,m,i) <= 0)
          R = i;
        else
          L = i;
        i = (R+L)/2;
      }
      Llimit=R;
      // search for right limit
      L = Llimit-1;
      R = n;
      i = (L+R)/2;
      if (Llimit < n) {
      while (L < R-1) {
        if (CompareIndex(pat,m,i) >= 0)
          L = i;
        else
          R = i;
        i = (R+L)/2;
        }
        Rlimit=L;
      }
      if (map0 != 0) free(pat);
      if (Llimit == n || Llimit > Rlimit) {
        return 0;
      }
      (*numocc)=Rlimit-Llimit+1;
      //ReportOccurrenceRegion(Llimit,Rlimit,0);
      ObtainOccurrences2(Llimit,Rlimit,m,numc,numocc,snippet_text,snippet_lengths);
      return 0;
    };

    void ObtainOccurrences2(ulong l, ulong r, ulong m, ulong numc, ulong *numocc,
                            uchar **snippet_text, ulong **snippet_lengths) {
      ulong *path = new ulong[samplerate+1];
      ulong pathdepth = 0;
      ulong i,j=0,x;
      *snippet_lengths = (ulong *) malloc((*numocc)*sizeof(ulong));
      *snippet_text = (uchar *) malloc((*numocc)*(m+2*numc)*sizeof(uchar));
      uchar *text_aux=*snippet_text;
      for (i=l;i<=r;i++) {
        x = ReportOccurrence(i,path,&pathdepth);
        (*snippet_lengths)[j] = ShowPath2(x,m,numc,text_aux);
        text_aux+=m+2*numc;
        j++;
      }
      delete [] path;
    };

    ulong ShowPath2( ulong x,  ulong m, ulong numc, uchar *snippet) {
      ulong from;
      if (x>numc) from = x-numc;
      else from=0;
      ulong to=min(x+m+numc-1,n-2);
      ulong left=GetHt(from/samplerate);
      ulong len =to-from+1;
      ulong offset,j;
      uchar c;
      ulong shift = from % samplerate;
      x = posrank->rank(left)-1; // CCSA position
      offset = left-posrank->prev(left); // offset = i-posrank->select(x+1);
      while (shift > 0) {
        shift--;
        left = GetPos(x)+offset; // suffix array position
        x = posrank->rank(left)-1; // CCSA position
        offset = left -posrank->prev(left);
      }

      for (j=0;j<len;j++) {
        c = chars[charrank->rank(x)-1]; //CharAtIndex(x)
        if (c == map0)
           snippet[j]='\0';
        else
           snippet[j]=c;
        left = GetPos(x)+offset; // suffix array position
        x = posrank->rank(left)-1; // CCSA position
        offset = left -posrank->prev(left);
      }
      return len;
    };

    int extract(ulong from, ulong to, uchar **snippet) {
      if (to> n) to=n-2;
      if (from > to) {
        *snippet = NULL;
        return 0;
      }
      ulong left=GetHt(from/samplerate);
      ulong len =to-from+1;
      *snippet = (uchar *) malloc(len*sizeof(uchar));
      ulong x,offset,j;
      uchar c;
      ulong shift = from % samplerate;
      x = posrank->rank(left)-1; // CCSA position
      offset = left-posrank->prev(left); // offset = i-posrank->select(x+1);
      while (shift > 0) {
        shift--;
        left = GetPos(x)+offset; // suffix array position
        x = posrank->rank(left)-1; // CCSA position
        offset = left -posrank->prev(left);
      }

      for (j=0;j<len;j++) {
        c = chars[charrank->rank(x)-1]; //CharAtIndex(x)
        if (c == map0)
           (*snippet)[j]='\0';
        else
           (*snippet)[j]=c;
        left = GetPos(x)+offset; // suffix array position
        x = posrank->rank(left)-1; // CCSA position
        offset = left -posrank->prev(left);
      }
      return len;
    };

    ulong SpaceRequirementInBits() {
      ulong size=0;
      size+= sizeof(TCCSA)*8;
      size+= charrank->SpaceRequirementInBits(); //charrank
      size+= posrank->SpaceRequirementInBits(); // posrank
      size+= samplerank->SpaceRequirementInBits(); // samplerank
      size+= (((length+1)*bitsn)/W+1)*sizeof(ulong)*8; // pos OK!
      size+= ((Htbits*samplen)/W+1)*sizeof(ulong)*8; //Ht OK!
      size+= size_uchar*sizeof(ulong)*8; // chars OK!
      size+= ((samplen*samplebits)/W+2)*sizeof(ulong)*8; // sample
      return size;
    };
    ulong SpaceRequirementInBits_locate() {
      ulong size=0;
      size+= sizeof(TCCSA)*8;
      size+= charrank->SpaceRequirementInBits(); //charrank
      size+= posrank->SpaceRequirementInBits(); // posrank
      size+= samplerank->SpaceRequirementInBits(); // samplerank
      size+= (((length+1)*bitsn)/W+1)*sizeof(ulong)*8; // pos OK!
      //size+= ((Htbits*samplen)/W+1)*sizeof(ulong)*8; //Ht OK!
      size+= size_uchar*sizeof(ulong)*8; // chars OK!
      size+= ((samplen*samplebits)/W+2)*sizeof(ulong)*8; // sample
      return size;
    };
    ulong SpaceRequirementInBits_count() {
      ulong size=0;
      size+= sizeof(TCCSA)*8;
      size+= charrank->SpaceRequirementInBits(); //charrank
      size+= posrank->SpaceRequirementInBits(); // posrank
      //size+= samplerank->SpaceRequirementInBits(); // samplerank
      size+= (((length+1)*bitsn)/W+1)*sizeof(ulong)*8; // pos OK!
      //size+= ((samplebits*samplen)/W+1)*sizeof(ulong)*8; //Ht OK!
      size+= size_uchar*sizeof(ulong)*8; // chars OK!
      //size+= (n/W+2)*sizeof(ulong)*8; // sample
      return size;
    };

    ulong get_length(){
      return n-1;
    }
    ~TCCSA() {
      delete [] pos;
      delete [] Ht;
      delete [] sample;
      delete [] chars;
      delete posrank;
      delete charrank;
      delete samplerank;
    };

     int save(char *filename) {
        char fnamext[1024];
        FILE *f;
        sprintf (fnamext,"%s.ccsa",filename);
        f = fopen (fnamext,"w");
        if (f == NULL) return 20;
        if (fwrite (&n,sizeof(ulong),1,f) != 1) return 21;
        if (fwrite (&bitsn,sizeof(ulong),1,f) != 1) return 21;
        if (fwrite (&posbits,sizeof(ulong),1,f) != 1) return 21;
        if (fwrite (&Htbits,sizeof(ulong),1,f) != 1) return 21;
        if (fwrite (&length,sizeof(ulong),1,f) != 1) return 21;
        if (fwrite (&samplebits,sizeof(ulong),1,f) != 1) return 21;
        if (fwrite (&samplerate,sizeof(ulong),1,f) != 1) return 21;
        if (fwrite (&samplen,sizeof(ulong),1,f) != 1) return 21;
        if (fwrite (&map0,sizeof(uchar),1,f) != 1) return 21;
        if (charrank->save(f) !=0) return 21;
        if (posrank->save(f) !=0) return 21;
        if (samplerank->save(f) !=0) return 21;
        if (fwrite (pos,sizeof(ulong),(length+1)*bitsn/W+1,f) != (length+1)*bitsn/W+1) return 21;
        if (fwrite (Ht,sizeof(ulong),((Htbits*samplen)/W+1),f) != ((Htbits*samplen)/W+1)) return 21;
        if (fwrite (sample,sizeof(ulong),((samplen*samplebits)/W+2),f) != ((samplen*samplebits)/W+2)) return 21;
        if (fwrite (chars,sizeof(ulong),size_uchar,f) != size_uchar) return 21;
        fclose(f);
        return 0;
     }
     int load(char *filename) {
        char fnamext[1024];
        FILE *f;
        int error;
        sprintf (fnamext,"%s.ccsa",filename);
        f = fopen (fnamext,"r");
        if (f == NULL) return 23;
        if (fread (&n,sizeof(ulong),1,f) != 1) return 25;
        if (fread (&bitsn,sizeof(ulong),1,f) != 1) return 25;
        if (fread (&posbits,sizeof(ulong),1,f) != 1) return 25;
        if (fread (&Htbits,sizeof(ulong),1,f) != 1) return 25;
        if (fread (&length,sizeof(ulong),1,f) != 1) return 25;
        if (fread (&samplebits,sizeof(ulong),1,f) != 1) return 25;
        if (fread (&samplerate,sizeof(ulong),1,f) != 1) return 25;
        if (fread (&samplen,sizeof(ulong),1,f) != 1) return 25;
        if (fread (&map0,sizeof(uchar),1,f) != 1) return 25;
        charrank = new BitRankF(f,&error);
        if (error !=0) return error;
        posrank = new BitRankF(f,&error);
        if (error !=0) return error;
        samplerank = new BitRankF(f,&error);
        if (error !=0) return error;
        this->pos = new ulong[(length+1)*bitsn/W+1];
        if (!this->pos) return 1;
        if (fread (pos,sizeof(ulong),(length+1)*bitsn/W+1,f) != (length+1)*bitsn/W+1) return 25;
        this->Ht = new ulong[((Htbits*samplen)/W+1)];
        if (!this->Ht) return 1;
        if (fread (Ht,sizeof(ulong),((Htbits*samplen)/W+1),f) != ((Htbits*samplen)/W+1)) return 25;
        this->sample = new ulong[((samplen*samplebits)/W+2)];
        if (!this->sample) return 1;
        if (fread (sample,sizeof(ulong),((samplen*samplebits)/W+2),f) != ((samplen*samplebits)/W+2)) return 25;
        this->chars = new ulong[size_uchar];
        if (!this->chars) return 1;
        if (fread (chars,sizeof(ulong),size_uchar,f) != size_uchar) return 25;
        fclose(f);
        return 0;
     }

     TCCSA(char *filename, int *error) {
        *error = load(filename);
     }

    
};


////////////////////////
////Building the Index//
////////////////////////
int build_index(uchar *text, ulong length, char *build_options, void **index){
  ulong _samplerate=64;
  int error;
  char delimiters[] = " =;";
  int j,num_parameters;
  char ** parameters;
  bool free_text=false; /* don't free text by default */

  if (build_options != NULL) {
    parse_parameters(build_options,&num_parameters, &parameters, delimiters);
    for (j=0; j<num_parameters;j++) {
      if ((strcmp(parameters[j], "samplerate") == 0 ) && (j < num_parameters-1) ) {
        _samplerate=atoi(parameters[j+1]);
        j++;
      } else if (strcmp(parameters[j], "free_text") == 0 )
        free_text=true;
    }
    free_parameters(num_parameters, &parameters);
  }
  TCCSA *CCSA = new TCCSA(text, length,_samplerate,free_text,&error);
  (*index) = CCSA;
  if (error != 0) return error;
  return 0;
}
int save_index(void *index, char *filename) {
  TCCSA *_index=(TCCSA *) index;
  return _index->save(filename);
}
int load_index(char *filename, void **index){
  int error;
  TCCSA *CCSA = new TCCSA(filename, &error);
  (*index) = CCSA;
  return error;
}
int free_index(void *index){
  TCCSA *_index=(TCCSA *) index;
  delete _index;
  return 0;
}

int index_size(void *index, ulong *size) {
  TCCSA *_index=(TCCSA *) index;
  (*size) = _index->SpaceRequirementInBits()/8;
  return 0;
}
int index_size_count(void *index, ulong *size) {
  TCCSA *_index=(TCCSA *) index;
  (*size) = _index->SpaceRequirementInBits_count()/8;
  return 0;
}
int index_size_locate(void *index, ulong *size) {
  TCCSA *_index=(TCCSA *) index;
  (*size) = _index->SpaceRequirementInBits_locate()/8;
  return 0;
}

////////////////////////
////Querying the Index//
////////////////////////
int count(void *index, uchar *pattern, ulong length, ulong *numocc){
  TCCSA *_index=(TCCSA *) index;
  (*numocc)= _index->count(pattern,length);
  return 0;
}
int locate(void *index, uchar *pattern, ulong length, ulong **occ, ulong *numocc){
  TCCSA *_index=(TCCSA *) index;
  (*numocc)= _index->locate(pattern,length,occ);
  return 0;
}

int display(void *index, uchar *pattern, ulong length, ulong numc, ulong *numocc, uchar **snippet_text, ulong **snippet_lengths) {
  TCCSA *_index=(TCCSA *) index;
  int aux = _index->display(pattern, length, numc, numocc, snippet_text, snippet_lengths);
  return aux;
}

int extract(void *index, ulong from, ulong to, uchar **snippet, ulong *snippet_length){
  TCCSA *_index=(TCCSA *) index;
  (*snippet_length)= _index->extract(from,to, snippet);
  return 0 ;
}
int get_length(void *index, ulong *length){
  TCCSA *_index=(TCCSA *) index;
  (*length)=(_index)->get_length();
  return 0;
}
////////////////////
////Error handling//
////////////////////
char *error_index(int e){
  switch(e) {
    case 0:  return "No error"; break;
    case 1:  return "Out of memory"; break;
    case 2:  return "The text must end with a \\0"; break;
    case 5:  return "You can't free the text if you don't copy it"; break;
    case 20: return "Cannot create files"; break;
    case 21: return "Error writing the index"; break;
    case 22: return "Error writing the index"; break;
    case 23: return "Cannot open index; break";
    case 24: return "Cannot open text; break";
    case 25: return "Error reading the index"; break;
    case 26: return "Error reading the index"; break;
    case 27: return "Error reading the text"; break;
    case 28: return "Error reading the text"; break;
    case 99: return "Not implemented"; break;
    default: return "Unknown error";
  }
}

