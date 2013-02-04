/* af-index.cpp
   Copyright (C) 2005, Rodrigo Gonzalez, all rights reserved.

   This file contains an implementation of an Alphabet-Friendly FM-index.
   For more information, see [FMMN05].

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
   [FMMN05] Paolo Ferragina, Giovanni Manzini, Veli Makinen, and Gonzalo Navarro.
   An Alphabet-Friendly FM-index. In Proc. SPIRE'04, pages 150-160. LNCS 3246.
   http://www.dcc.uchile.cl/~gnavarro/ps/spire04.1.ps.gz

   This library uses a suffix array construction algorithm developed by
   Giovanni Manzini (Universit\uffff del Piemonte Orientale) and
   Paolo Ferragina (Universit\uffff di Pisa).
   You can find more info at:
   http://roquefort.di.unipi.it/~ferrax/SuffixArray/

   Also this library uses a bwt optimal partitioning developed by
   Giovanni Manzini (manzini@mfn.unipmn.it)
   You can find more info at:
   http://www.mfn.unipmn.it/~manzini/manzini_uk.html

*/

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/times.h>
#include <sys/resource.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>

#include "utils/interface.h"
#include "386.c"

/* --- read proptotypes and typedef --- */
#include "basic.h"
#include "bitarray.h"
#include "HuffAlphabetRank.h"
#include "Huffman_Codes.h"

/* --- read proptotypes and typedef for ds_ssort --- */
#include "ds/ds_ssort.h"
#include "bwtopt/bwt_aux.h"
#include "bwtopt/lcp_aux.h"
/* --- read proptotypes and typedef for bwtopt --- */
#include "bwtopt/bwtopt1.h"

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

/* some clases used in af-index */
class block1 {
public:
  THuffman_Codes *codetable;
  THuffAlphabetRank *alphabetrank;
  block1() { codetable=NULL; alphabetrank=NULL;};
  ~block1() { delete codetable; delete alphabetrank;};
  ulong SpaceRequirementInBits() { return codetable->SpaceRequirementInBits() + alphabetrank->SpaceRequirementInBits()+sizeof(block1)*8;};
};

union block {
  block1 *block_t1;
  uchar block_t2; //is just one char
};

class TAFindex {
private:
  ulong n;
  ulong bits_n;
  ulong samplerate;
  ulong C[size_uchar+1];
  uchar remap_new[size_uchar];
  ulong alphasize;
  ulong factor;
  uchar *remap_old;
  BitRankW32Int *sampled;
  BitRankFSparse *blocks;
  ulong numblocks;
  ulong *type_block; //true => size > 1
  ulong *suffixes;
  ulong *positions;
  ulong bits_CC;
  ulong *CC;
  block *block_array;

  uchar *BWTT(uchar *text, bool free_text, bool withload, char *fnamext) {
    // Build the suffix array
    ulong block_aux;
    ulong *sa;
    int overshoot;
    ulong *lcp=NULL;
    double estimate; // For lcp creation
    bwt_data b; // For lcp creation
    ulong i;
    uchar *x;
    overshoot = init_ds_ssort(500, 2000);
    sa= (ulong *) malloc (sizeof(ulong)*(this->n+1));
    if (!sa) return NULL;
    x= (uchar *) malloc (sizeof(uchar)*(this->n+overshoot));
    if (!x) return NULL;
    x[this->n-1]=0;
    for (i=0;i<this->n-1;i++) x[i]=text[i];
    /* Remap 0 if exist */
    this->remap(x);
    if (free_text) free(text); 

    if (withload) {
      ulong filename_len;
      char fnameaux[1024];
      FILE *f;
      sprintf (fnameaux,"%s.sa1",fnamext);
      f = fopen (fnameaux,"r");
      if (fread (&filename_len,sizeof(ulong),1,f) != 1) return NULL;
      assert(filename_len==n+1);
      if (fread (sa,sizeof(ulong),filename_len,f) != filename_len) return NULL;
      if (fclose(f) != 0) return NULL;
    } else {
      ds_ssort( x, sa, this->n);
/*
      ulong filename_len=n+1;
      char fnameaux[1024];
      FILE *f;
      sprintf (fnameaux,"%s.sa1",fnamext);
      f = fopen (fnameaux,"w");
      if (fwrite (&filename_len,sizeof(ulong),1,f) != 1) return NULL;
      assert(filename_len==n+1);
      if (fwrite (sa,sizeof(ulong),filename_len,f) != filename_len) return NULL;
      if (fclose(f) != 0) return NULL;
*/
    }

    uchar *s = new uchar[n+1];
    ulong *sampledpositions = new ulong[n/W+1];
    ulong *blockspositions = new ulong[(n+1)/W+1];
    suffixes = new ulong[n/samplerate+1];
    positions = new ulong[n/samplerate+2];
    ulong j=0;
    for (i=0;i<n/W+1;i++) {
      sampledpositions[i]=0u;
      blockspositions[i]=0u;
    }
    for (i=0;i<n;i++)
      if (sa[i] % samplerate == 0) {
        SetField(sampledpositions,1,i,1);
        suffixes[j++] = sa[i];
        positions[sa[i]/samplerate] = i;
      }
    positions[(this->n-1)/samplerate+1] = positions[0];

    sampled = new BitRankW32Int(sampledpositions,this->n,true, factor);
    for (i=0;i<this->n;i++)
      if (sa[i]==0) {
        s[i] = '\0'; //text[this->n-1];
        b.eof_pos = i;
      } else s[i] = x[sa[i]-1];

    if (withload) {
      free(x);
      lcp =sa;
      ulong filename_len;
      char fnameaux[1024];
      FILE *f;
      sprintf (fnameaux,"%s.af_lcp",fnamext);
      f = fopen (fnameaux,"r");
      if (fread (&filename_len,sizeof(ulong),1,f) != 1) return NULL;
      assert(filename_len==n+1);
      if (fread (lcp,sizeof(ulong),filename_len,f) != filename_len) return NULL;
      if (fclose(f) != 0) return NULL;
    } else { 
      int occ[size_uchar], extra_bytes; // For lcp creation
      for(i=0;i<size_uchar;i++) occ[i]=0; // For lcp creation
      for (i=0; i<this->n; i++) {
        occ[x[i]]++; // For lcp creation
      }

      b.bwt = (uchar *)malloc((n+1)*sizeof (uchar));
      assert(b.bwt!=NULL);
      _bw_sa2bwt(x, n, sa, &b);
      extra_bytes = _lcp_sa2lcp_6n(x,&b, sa, occ);
      free(x);
      lcp = sa;
      estimate = bwt_partition1(&b,sa,alphasize);
      free(b.bwt);
    }

/*
    {
      ulong filename_len=n+1;
      char fnameaux[1024];
      FILE *f;
      sprintf (fnameaux,"%s.af_lcp",fnamext);
      f = fopen (fnameaux,"w");
      if (fwrite (&filename_len,sizeof(ulong),1,f) != 1) return NULL;
      assert(filename_len==n+1);
      if (fwrite (lcp,sizeof(ulong),filename_len,f) != filename_len) return NULL;
      if (fclose(f) != 0) return NULL;
    }
*/
    
    /*lcp = sa;
    f = fopen ("/data/rgonzale/texts/dna/temporal2","r");
    fread (sa,sizeof(ulong),this->n+1,f);
    fclose(f); */

    SetField(blockspositions,1,0,1);
    numblocks=1;
    block_aux = lcp[0];
    //  printf("%d\n", block_aux);
    while ( block_aux != n+1 ) {
      SetField(blockspositions,1,block_aux,1);
      numblocks++;
      block_aux = lcp[block_aux];
    //  printf("%d\n", block_aux);
    }
    SetField(blockspositions,1,n,1);
    free(lcp); //same of sa pointer
    blocks = new BitRankFSparse(blockspositions,n+1);
    delete [] blockspositions;
    return s;
  };
  int build_blocks(uchar *bwt, ulong factor){
    uchar *bwt_aux;
    ulong aux1,aux2;
    ulong sub_size;
    if (bwt == NULL) return 1;
    block_array = new block[numblocks];
    if (!block_array) return 1;
    for (ulong j=1;j<=numblocks;j++) {
      aux2=blocks->select(j+1);
      aux1=blocks->prev(aux2-1);
      bwt_aux = bwt+aux1;
      sub_size = aux2-aux1;
      //assert(aux1 == blocks->select(j));
      if (bitget(type_block,j-1)) {
        block_array[j-1].block_t1 = new block1();
        if (!block_array[j-1].block_t1) return 1;
        block_array[j-1].block_t1->codetable = new THuffman_Codes(bwt_aux,sub_size);
        if (!block_array[j-1].block_t1->codetable) return 1;
        block_array[j-1].block_t1->alphabetrank = new THuffAlphabetRank(bwt_aux,sub_size,block_array[j-1].block_t1->codetable,0, factor);
        if (!block_array[j-1].block_t1->alphabetrank) return 1;
      } else {
        block_array[j-1].block_t2 = bwt_aux[0];
      }
    }
    return 0;
  };

  int super(uchar *bwt ){
    ulong i,j,k;
    bool type;
    uchar cha;
    ulong aux1,aux2;
    if (bwt == NULL) return 1;
    type_block= new ulong[numblocks/W+1];
    if (!type_block) return 1;
    for (j=1;j<=numblocks;j++) {
      aux2=blocks->select(j+1);
      aux1=blocks->prev(aux2-1);
      type = false;
      cha = bwt[aux1];
      for (i=aux1+1;i<aux2;i++) {
        //if (i>n-1) printf ("bloques %d %d %d %d\n",aux1+1, aux2, blocks->select(j),blocks->select(j+1));
        if ( cha != bwt[i] ) type =  true;
          cha = bwt[i];
      }
      if (type) SetField(type_block,1,j-1,1);
      else SetField(type_block,1,j-1,0);
    }

    //ulong *CC_aux = new ulong[(numblocks*alphasize*bits_n)/32+1];
    ulong *CC_aux = (ulong *) malloc ( ((numblocks*alphasize*bits_n)/32+1)*sizeof(ulong)); 
    if (!CC_aux) return 1;
    //printf("tamano %lu\n", ((numblocks*alphasize*bits_n)/W+1)*sizeof(ulong));
    for (i=0;i<numblocks*alphasize;i++)
      SetField(CC_aux,bits_n,i,0);
    for (j=2;j<=numblocks;j++) {
      for (k=0;k<alphasize;k++) SetField(CC_aux,bits_n,k+(j-1)*alphasize,GetField(CC_aux,bits_n,k+(j-2)*alphasize));
      aux2=blocks->select(j);
      aux1=blocks->prev(aux2-1);
      for (i=aux1;i<aux2;i++)
        SetField(CC_aux,bits_n,bwt[i]+(j-1)*alphasize,GetField(CC_aux,bits_n,bwt[i]+(j-1)*alphasize)+1);
    }
    ulong max=0,aux;
    //printf ("original tamano estimado %lu\n",((numblocks*alphasize*bits_n)/W+1)*sizeof(ulong));
    for (i=1;i<numblocks;i++) {
      for (j=0;j<alphasize;j++){
        aux = GetField(CC_aux,bits_n,j+i*alphasize);
        if (max < aux) max = aux;
      }
    }
    bits_CC = bits(max+1); 
    //printf ("final tamano estimado %lu\n",((numblocks*alphasize*bits_CC)/W+1)*sizeof(ulong));
    CC = new ulong[(numblocks*alphasize*bits_CC)/W+1];
    if (!CC) return 1;
    for (i=0;i<numblocks;i++) 
      for (j=0;j<alphasize;j++)
        SetField(CC,bits_CC,j+i*alphasize, GetField(CC_aux,bits_n,j+i*alphasize));
    free(CC_aux);
    return 0;
  };

  ulong occ_count(uchar c, ulong pos) {
    ulong in_block, type_of_block, begin_block, offset, sub_block_result;
    in_block = blocks->rank(pos);// Block where pos belong
    type_of_block = bitget(type_block,in_block-1); //Type of block
    begin_block = blocks->prev(pos); // Beginning of block
    offset = pos - begin_block; //Relative position within block
    if (type_of_block) {
      sub_block_result = block_array[in_block-1].block_t1->alphabetrank->rank(c,offset,block_array[in_block-1].block_t1->codetable);
    } else {
      if (block_array[in_block-1].block_t2 == c) sub_block_result = offset+1;
      else sub_block_result = 0;
    }
    return  GetField(CC,bits_CC,c+(in_block-1)*alphasize)+sub_block_result;
  };
  ulong char_at_pos(ulong pos) {
    ulong in_block, type_of_block, begin_block, offset;
    in_block = blocks->rank(pos);// Block where pos belong
    type_of_block = bitget(type_block,in_block-1); //Type of block
    if (type_of_block) {
      begin_block = blocks->prev(pos); // Beginning of block
      offset = pos - begin_block; //Relative position within block
      return block_array[in_block-1].block_t1->alphabetrank->charAtPos(offset);
    } else {
      return block_array[in_block-1].block_t2;
    }
  };
  ulong char_at_pos2(ulong pos,ulong *rank_tmp) {
    ulong in_block, type_of_block, begin_block, offset;
    uchar c;
    in_block = blocks->rank(pos);// Block where pos belong
    type_of_block = bitget(type_block,in_block-1); //Type of block
    begin_block = blocks->prev(pos); // Beginning of block
    offset = pos - begin_block; //Relative position within block
    if (type_of_block) {
      c = block_array[in_block-1].block_t1->alphabetrank->charAtPos2(offset,rank_tmp);
    } else {
      *rank_tmp=offset;
      c = block_array[in_block-1].block_t2;
    }
    *rank_tmp+=GetField(CC,bits_CC,c+(in_block-1)*alphasize);
    return c;
  };
  void remap(uchar *text) {
    ulong i,j,size=0;
    ulong Freq_old[size_uchar];
    for(i=0;i<size_uchar;i++)
      Freq_old[i]=0;
    for(i=0;i<n;i++) {
      if(Freq_old[text[i]]++==0) size++;
    }
    alphasize=size;
    // remap alphabet
    if (Freq_old[0]>1) {i=1;alphasize++;} //test if some character of T is zero, we already know that text[n-1]='\0'
    else i=0;

    remap_old= new uchar[size];
    for(j=0;j<size_uchar;j++)
      if(Freq_old[j]!=0) {
        remap_new[j]=i;
        remap_old[i++]=j;
      }
    // remap text
    for(i=0;i<n-1;i++) // the last character must be zero
      text[i]=remap_new[text[i]];
  }

public:
  TAFindex(uchar *text, ulong length, ulong samplerate, ulong factor, bool free_text, bool withload, char *fnamext, int *error) {
    this->n = length+1;
    this->bits_n=bits(n+1);
    this->samplerate = samplerate;
	this->factor = factor;
    uchar *bwt;
    bwt = BWTT(text,free_text,withload,fnamext);
    if (bwt == NULL) *error = 1;
    if (super(bwt) != 0) *error = 1;
    build_blocks(bwt, factor);
    //printf ("Numero de bloques=%lu\n", numblocks);
    // caller can delete text now
    ulong i;
    for (i=0;i<size_uchar+1;i++)
      C[i]=0;
    for (i=0;i<n;++i)
      C[bwt[i]]++;
    ulong prev=C[0], temp;
    C[0]=0;
    for (i=1;i<size_uchar+1;i++) {
      temp = C[i];
      C[i]=C[i-1]+prev;
      prev = temp;
    }
    //printf("number of blocks %lu\n", numblocks);fflush(stdout);
    *error=0;
    delete [] bwt;
  };
  ulong count(uchar *pattern, ulong m) {
    // use the FM-search replacing function Occ(c,1,i) with occ_count(c,i)
    uchar c = remap_new[pattern[m-1]]; ulong i=m-1;
    ulong sp = C[c];
    ulong ep = C[c+1]-1;
    while (sp<=ep && i>=1) {
      c = remap_new[pattern[--i]];
      sp = C[c]+occ_count(c, sp-1);
      ep = C[c]+occ_count(c, ep)-1;
    }
    if (sp<=ep) {
      return ep-sp+1;
    } else {
      return 0;
    }
  };

int locate_extract(){
ulong largo,*occ,i,lar,ep;
long te; uchar c;
ulong matches,locate;
ulong j,dist,rank_tmp;
ulong random;
for (ulong hh=1; hh <= 1000000; hh*=10)
 for (lar=1;lar<=9;lar++) {
   largo=lar*hh;
   startclock3();
   occ=NULL;
   random = (ulong) (((float) rand()/ (float) RAND_MAX)*(n-1));
   matches = largo+1;
   locate=0;
   occ = (ulong *) malloc(matches*sizeof(ulong));
   i=random;
   ep=min(random+largo,n-3);
   while (i<=ep) {
     j=i,dist=0;
     while (!sampled->IsBitSet(j)) {
       c = char_at_pos2(j,&rank_tmp);
       j = C[c]+rank_tmp; // LF-mapping
       ++dist;
       assert(dist < samplerate);
     }
     occ[locate]=suffixes[sampled->rank(j)-1]+dist;
     locate ++ ;
     ++i;
   }
   free(occ);
   te = stopclock3();
   printf("Largo=%lu|e_t=%.4fs(%ldms)\n",largo, (te*1.0)/(HZ*1.0), te);fflush(stdout);
 }
 return 0;
}

  ulong locate(uchar *pattern, ulong m, ulong **occ) {
    //locate_extract();
    //exit(0);
    // use the FM-search replacing function Occ(c,1,i) with occ_count(c,i)
    uchar c =  remap_new[pattern[m-1]]; ulong i=m-1;
    ulong sp = C[c];
    ulong ep = C[c+1]-1;
    while (sp<=ep && i>=1) {
      c =  remap_new[pattern[--i]];
      sp = C[c]+occ_count(c,sp-1);
      ep = C[c]+occ_count(c,ep)-1;
    }
    if (sp<=ep) {
      ulong matches = ep-sp+1;
      ulong locate=0;
      *occ = (ulong *) malloc(matches*sizeof(ulong));
      i=sp;
      ulong j,dist,rank_tmp;
      while (i<=ep) {
        j=i,dist=0;
        while (!sampled->IsBitSet(j)) {
          c = char_at_pos2(j,&rank_tmp);
          j = C[c]+rank_tmp; // LF-mapping
          ++dist;
        }
        (*occ)[locate]=suffixes[sampled->rank(j)-1]+dist;
        locate ++ ;
        ++i;
      }
      return ep-sp+1;
    } else {
      *occ = NULL;
      return 0;
    }
  };
  int display(uchar *pattern, ulong m, ulong numc, ulong *numocc, uchar **snippet_text, ulong **snippet_lengths, ulong **occ) {
    // use the FM-search replacing function Occ(c,1,i) with alphabetrank->rank(c,i)
    uchar c = remap_new[pattern[m-1]]; ulong i=m-1;
    ulong sp = C[c];
    ulong ep = C[c+1]-1;
    while (sp<=ep && i>=1) {
      c = remap_new[pattern[--i]];
      sp = C[c]+occ_count(c,sp-1);
      ep = C[c]+occ_count(c,ep)-1;
    }
    if (sp<=ep) {
      (*numocc) = ep-sp+1;
      ulong locate=0;
      *occ = (ulong *) malloc((*numocc)*sizeof(ulong));
      *snippet_lengths = (ulong *) malloc((*numocc)*sizeof(ulong));
      *snippet_text = (uchar *) malloc((*numocc)*(m+2*numc)*sizeof(uchar));
      uchar *text_aux=*snippet_text;

      i=sp;
      ulong j,dist,x,rank_tmp;
      while (i<=ep) {
        j=i,dist=0;
        while (!sampled->IsBitSet(j)) {
          c = char_at_pos2(j,&rank_tmp);
          j = C[c]+rank_tmp; // LF-mapping
          ++dist;
        }
        x = suffixes[sampled->rank(j)-1]+dist;
        (*occ)[locate]= x;
        ulong from;
        if (x>numc) from = x-numc;
        else from=0;
        ulong to=min(x+m+numc-1,n-2); // n-1 is a '\0' 
        ulong len =to-from+1;
        ulong skip;
        ulong j = positions[to/samplerate+1];
        if ((to/samplerate+1) == ((n-1)/samplerate+1))
          skip = n-1 - to;
        else
          skip = samplerate-to%samplerate-1;
        for (ulong dist=0;dist<skip+len;dist++) {
          c = char_at_pos2(j,&rank_tmp);
          j = C[c]+rank_tmp; // LF-mapping
          if (dist>= skip)
            text_aux[len-dist-1+skip]=remap_old[c];
        }
        (*snippet_lengths)[locate]=len;
        text_aux+=m+2*numc;

        locate ++ ;
        i++;
      }
    } else {
      *snippet_lengths = NULL;
      *snippet_text = NULL;
      *occ = NULL;
      (*numocc) = 0;
    }
    return 0;
  };

  ulong SpaceRequirement(){
    ulong size=0;
    size+=sizeof(TAFindex);
    size+=sizeof(uchar)*alphasize; //remap + pointer remap_old
    size+=sampled->SpaceRequirementInBits()/8;
    size+=blocks->SpaceRequirementInBits()/8;
    size+=(numblocks/W+1)*sizeof(ulong); //type_block
    size+=((n/samplerate+1)*sizeof(ulong))*2; // suffixes + positions
    size+=((numblocks*alphasize*bits_CC)/W+1)*sizeof(ulong); //CC
    size+=sizeof(block)*numblocks; //block_array
    for (ulong j=0;j<numblocks;j++)
      if (bitget(type_block,j))
        size+=block_array[j].block_t1->SpaceRequirementInBits()/8; //size block1
    return size;
  }
  ulong SpaceRequirement_count(){
    ulong size=0;
    size+=sizeof(TAFindex);
    size+=sizeof(uchar)*alphasize; //remap + pointer remap_old
    //size+=sampled->SpaceRequirementInBits();
    size+=blocks->SpaceRequirementInBits()/8;
    size+=(numblocks/W+1)*sizeof(ulong); //type_block
    //size+=((n/samplerate+1)*W)*2; // suffixes + positions
    size+=((numblocks*alphasize*bits_CC)/W+1)*sizeof(ulong); //CC
    size+=sizeof(block)*numblocks; //block_array
    for (ulong j=0;j<numblocks;j++)
      if (bitget(type_block,j))
        size+=block_array[j].block_t1->SpaceRequirementInBits()/8; //size block1
    return size;
  }
  ulong SpaceRequirement_locate(){
    ulong size=0;
    size+=sizeof(TAFindex);
    size+=sizeof(uchar)*alphasize; //remap + pointer remap_old
    size+=sampled->SpaceRequirementInBits()/8;
    size+=blocks->SpaceRequirementInBits()/8;
    size+=(numblocks/W+1)*sizeof(ulong); //type_block
    size+=((n/samplerate+1)*sizeof(ulong)); // suffixes 
    size+=((numblocks*alphasize*bits_CC)/W+1)*sizeof(ulong); //CC
    size+=sizeof(block)*numblocks; //block_array
    for (ulong j=0;j<numblocks;j++)
      if (bitget(type_block,j))
        size+=block_array[j].block_t1->SpaceRequirementInBits()/8; //size block1
    return size;
  }


  ulong get_length () {
    return n-1;
  }

  ~TAFindex() {
    for (ulong j=numblocks;j>0;j--) {
      if (bitget(type_block,j-1))
        delete block_array[j-1].block_t1;
    }
    delete [] block_array;
    delete [] suffixes;
    delete [] positions;
    delete [] remap_old;
    delete sampled;
    delete blocks;
    delete [] type_block;
    delete [] CC;
  };
  int extract(ulong from, ulong to, uchar **snippet) {
    if (to> n) to=n-2; // n-1 is '\0'
    if (from > to) {
      *snippet = NULL;
      return 0;
    }
    ulong len =to-from+1;
    *snippet = (uchar *) malloc((len)*sizeof(uchar));
    uchar c;
    ulong skip,rank_tmp;
    ulong j = positions[to/samplerate+1];
    if ((to/samplerate+1) == ((n-1)/samplerate+1))
      skip = n-1 - to;
    else
      skip = samplerate-to%samplerate-1;

    for (ulong dist=0;dist<skip+len;dist++) {
      c = char_at_pos2(j,&rank_tmp);
      j = C[c]+rank_tmp; // LF-mapping
      if (dist>= skip)
        (*snippet)[len-dist-1+skip]=remap_old[c];
    }
    return len;
  }
  int save(char *filename) {
    char fnamext[1024];
    FILE *f;
    sprintf (fnamext,"%s.af",filename);
    f = fopen (fnamext,"w");
    if (f == NULL) return 20;
    if (fwrite (&n,sizeof(ulong),1,f) != 1) return 21;
    if (fwrite (&bits_n,sizeof(ulong),1,f) != 1) return 21;
    if (fwrite (&samplerate,sizeof(ulong),1,f) != 1) return 21;
    if (fwrite (C,sizeof(ulong),size_uchar+1,f) != size_uchar+1) return 21;
    if (fwrite (remap_new,sizeof(uchar),size_uchar,f) != size_uchar) return 21;
    if (fwrite (&alphasize,sizeof(ulong),1,f) != 1) return 21;
    if (fwrite (remap_old,sizeof(uchar),alphasize,f) != alphasize) return 21;
    if (sampled->save(f) !=0) return 21;
    if (blocks->save(f) !=0) return 21;
    if (fwrite (&numblocks,sizeof(ulong),1,f) != 1) return 21;
    if (fwrite (type_block,sizeof(ulong),numblocks/W+1,f) != numblocks/W+1) return 21;
    if (fwrite (suffixes,sizeof(ulong),n/samplerate+1,f) != n/samplerate+1) return 21;
    if (fwrite (positions,sizeof(ulong),n/samplerate+2,f) != n/samplerate+2) return 21;
    if (fwrite (&bits_CC,sizeof(ulong),1,f) != 1) return 21;
    if (fwrite (CC,sizeof(ulong),(numblocks*alphasize*bits_CC)/W+1,f) != (numblocks*alphasize*bits_CC)/W+1) return 21;
    for (ulong j=1;j<=numblocks;j++) {
      if (bitget(type_block,j-1)) {
        if (block_array[j-1].block_t1->codetable->save(f) != 0) return 21;
        if (block_array[j-1].block_t1->alphabetrank->save(f) != 0 ) return 21;
      } else {
        if (fwrite (block_array+j-1,sizeof(block),1,f) != 1) return 21;
      }
    }
    fclose(f);
    return 0;
  }
  int load(char *filename) {
    char fnamext[1024];
    FILE *f;
    int error;
    sprintf (fnamext,"%s.af",filename);
    f = fopen (fnamext,"r");
    if (f == NULL) return 23;
    if (fread (&n,sizeof(ulong),1,f) != 1) return 25;
    if (fread (&bits_n,sizeof(ulong),1,f) != 1) return 25;
    if (fread (&samplerate,sizeof(ulong),1,f) != 1) return 25;
    if (fread (C,sizeof(ulong),size_uchar+1,f) != size_uchar+1) return 25;
    if (fread (remap_new,sizeof(uchar),size_uchar,f) != size_uchar) return 25;
    if (fread (&alphasize,sizeof(ulong),1,f) != 1) return 25;
    remap_old= new uchar[alphasize];
    if (!remap_old) return 25;
    if (fread (remap_old,sizeof(uchar),alphasize,f) != alphasize) return 25;
    sampled = new BitRankW32Int(f,&error);
    if (error !=0) return 25;
    blocks = new BitRankFSparse(f,&error);
    if (error !=0) return 25;
    if (fread (&numblocks,sizeof(ulong),1,f) != 1) return 25;
    type_block= new ulong[numblocks/W+1];
    if (!type_block) return 25;
    if (fread (type_block,sizeof(ulong),numblocks/W+1,f) != numblocks/W+1) return 25;
    suffixes = new ulong[n/samplerate+1];
    if (!suffixes) return 25;
    if (fread (suffixes,sizeof(ulong),n/samplerate+1,f) != n/samplerate+1) return 25;
    positions = new ulong[n/samplerate+2];
    if (!positions) return 25;
    if (fread (positions,sizeof(ulong),n/samplerate+2,f) != n/samplerate+2) return 25;
    if (fread (&bits_CC,sizeof(ulong),1,f) != 1) return 25;
    CC = new ulong[(numblocks*alphasize*bits_CC)/W+1];
    if (!CC) return 25;
    if (fread (CC,sizeof(ulong),(numblocks*alphasize*bits_CC)/W+1,f) != (numblocks*alphasize*bits_CC)/W+1) return 25;
    block_array = new block[numblocks];
    if (!block_array) return 1;
    for (ulong j=1;j<=numblocks;j++) {
      if (bitget(type_block,j-1)) {
        block_array[j-1].block_t1 = new block1();
        if (!block_array[j-1].block_t1) return 1;
        block_array[j-1].block_t1->codetable = new THuffman_Codes(f,&error);
        if (!block_array[j-1].block_t1->codetable) return 1;
        block_array[j-1].block_t1->alphabetrank = new THuffAlphabetRank(f,&error);
        if (!block_array[j-1].block_t1->alphabetrank) return 1;
      } else {
        if (fread (block_array+j-1,sizeof(block),1,f) != 1) return 25;
      }
    }
    fclose(f);
    return 0;
  }

  TAFindex(char *filename, int *error) {
    *error = load(filename);
  }

};


////////////////////////
////Building the Index//
////////////////////////
int build_index(uchar *text, ulong length, char *build_options, void **index){
  ulong samplerate=64;
  ulong factor = 4;
  int error;
  char fnamext[1024];
  char delimiters[] = " =;";
  int j,num_parameters;
  char ** parameters;
  bool free_text=false; /* don't free text by default */
  bool  withload=false;           /* don't load SA and BPE by default */
	
  if (build_options != NULL) {
    parse_parameters(build_options,&num_parameters, &parameters, delimiters);
    for (j=0; j<num_parameters;j++) {
      if ((strcmp(parameters[j], "samplerate") == 0 ) && (j < num_parameters-1) ) {
        samplerate=atoi(parameters[j+1]);
        j++;
      } else if (strcmp(parameters[j], "withload") == 0 )
        withload=true;
      else if ((strcmp(parameters[j], "factor") == 0 ) && (j < num_parameters-1) ) {
        factor=atoi(parameters[j+1]);
        j++;
      } else if (strcmp(parameters[j], "filename") == 0 ) {
        strcpy(fnamext,parameters[j+1]);
        j++;
      } else if (strcmp(parameters[j], "free_text") == 0 )
        free_text=true;
    }
    free_parameters(num_parameters, &parameters);
  }
  
  TAFindex *FMindex = new TAFindex(text,length,samplerate, factor, free_text, withload, fnamext, &error);
  (*index) = FMindex;
  if (error != 0) return error;
  return 0;
}
int save_index(void *index, char *filename) {
  TAFindex *_index=(TAFindex *) index;
  return _index->save(filename);
}
int load_index(char *filename, void **index){
  int error;
  TAFindex *AFindex = new TAFindex(filename, &error);
  (*index) = AFindex;
  return error;
}

int free_index(void *index){
  TAFindex *_index=(TAFindex *) index;
  delete _index;
  return 0;
}

int index_size(void *index, ulong *size) {
  TAFindex *_index=(TAFindex *) index;
  (*size) = _index->SpaceRequirement();
  return 0;
}

int index_size_count(void *index, ulong *size) {
  TAFindex *_index=(TAFindex *) index;
  (*size) = _index->SpaceRequirement_count();
  return 0;
}
int index_size_locate(void *index, ulong *size) {
  TAFindex *_index=(TAFindex *) index;
  (*size) = _index->SpaceRequirement_locate();
  return 0;
}
////////////////////////
////Querying the Index//
////////////////////////
int count(void *index, uchar *pattern, ulong length, ulong *numocc){
  TAFindex *_index=(TAFindex *) index;
  (*numocc)= _index->count(pattern,length);
  return 0;
}
int locate(void *index, uchar *pattern, ulong length, ulong **occ, ulong *numocc){
  TAFindex *_index=(TAFindex *) index;
  (*numocc)= _index->locate(pattern,length,occ);
  return 0;
}
/////////////////////////
//Accessing the indexed//
/////////////////////////
int extract(void *index, ulong from, ulong to, uchar **snippet, ulong *snippet_length){
  TAFindex *_index=(TAFindex *) index;
  (*snippet_length)= _index->extract(from,to, snippet);
  return 0 ;
}
int display(void *index, uchar *pattern, ulong length, ulong numc, ulong *numocc, uchar **snippet_text, ulong **snippet_lengths) {
  TAFindex *_index=(TAFindex *) index;
  ulong *occ;
  int aux = _index->display(pattern, length, numc, numocc, snippet_text, snippet_lengths, &occ);
  free(occ);
  return aux;
}
int get_length(void *index, ulong *length){
  TAFindex *_index=(TAFindex *) index;
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
