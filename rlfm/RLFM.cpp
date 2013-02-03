/* RLFM.cpp
   Copyright (C) 2005, Rodrigo Gonzalez, all rights reserved.

   This file contains an implementation of a Run Length FM index: RLFM-index.
   Based on a version programmed by Veli Makinen.
   http://www.cs.helsinki.fi/u/vmakinen/software/
   For more information, see [MN2004b, MN2004c, MN2005a].

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
[MN2004b] M\"akinen, V. and Navarro, G. 2004b. New search algorithms and time/space tradeoffs for
succinct suffix arrays. Technical Report C-2004-20 (April), University of Helsinki, Finland.

[MN2004c] M\"akinen, V. and Navarro, G. 2004c. Run-length FM-index. In Proc. DIMACS Workshop:
The Burrows-Wheeler Transform: Ten Years Later (Aug. 2004), pp. 17-19.

[MN2005a] M\"akinen, V. and Navarro, G. 2005a. Succinct suffix arrays based on run-length encoding.
In Proc. 16th Annual Symposium on Combinatorial Pattern Matching (CPM), LNCS v.
3537 (2005), pp. 45-56.

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
#include "HuffAlphabetRank.h"
#include "Huffman_Codes.h"
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


/**********************************************************************************************************
 * FM-index implementation using alphabet ranking and RL-compression: RLFM-index                          *
 **********************************************************************************************************/

class TRLFMindex {
private:
    ulong n;
    ulong len;
    ulong C[size_uchar+1];
    ulong samplerate;
    THuffAlphabetRank *alphabetrank;
    BitRankF *runs_bwt;
    BitRankF *runs_sa;
    BitRankF *sampled;
    TCodeEntry *codetable;
    ulong *suffixes;
    ulong *positions;
    uchar map0;

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
    uchar *pattern0(uchar *pattern, ulong m) {
      uchar *pat0;
      pat0 = (uchar *) malloc (sizeof(uchar)*m);
      for (ulong j=0;j<m;j++) {
        if (pattern[j] == 0) pat0[j]=map0;
        else pat0[j]=pattern[j];
      }
      return pat0;
    }
    uchar *BWT(uchar *text, bool free_text) {
      ulong i;
      int overshoot;
      ulong *sa;
      /* Make suffix array */
      uchar *x;
      overshoot = init_ds_ssort(500, 2000);
      sa= (ulong *) malloc (sizeof(ulong)*(this->n));
      if (!sa) return NULL;
      x= (uchar *) malloc (sizeof(uchar)*(this->n+overshoot));
      if (!x) return NULL;
      x[this->n-1]=0;
      for (i=0;i<this->n-1;i++) x[i]=text[i];
      if (free_text) free(text);
      /* Remap 0 if exist */
      map0=remap0(x,this->n-1); // the text contain a 0 at the end
      ds_ssort( x, sa, this->n);

      uchar *s = new uchar[this->n];
      if (!s) return NULL;
      ulong *sampledpositions = new ulong[this->n/W+1];
      if (!sampledpositions) return NULL;
      this->suffixes = new ulong[this->n/samplerate+1];
      if (!this->suffixes) return NULL;
      this->positions = new ulong[this->n/samplerate+2];
      if (!this->positions) return NULL;
      ulong j=0;
      for (i=0;i<this->n/W+1;i++)
        sampledpositions[i]=0u;
      for (i=0;i<this->n;i++)
        if (sa[i] % samplerate == 0) {
          SetField(sampledpositions,1,i,1);
          this->suffixes[j] = sa[i];
          this->positions[sa[i]/samplerate] = i;
          j++;
        }
      positions[(this->n-1)/samplerate+1] = positions[0];
      this->sampled = new BitRankF(sampledpositions,this->n,true);
      if (!this->sampled) return NULL;
      for (i=0;i<this->n;i++)
         if (sa[i]==0) s[i] = '\0'; //text[this->n-1];
         else s[i] = x[sa[i]-1];
      free(x);
      free(sa);
      return s;
    }

public:
    TRLFMindex(uchar *text, ulong length, ulong samplerate, bool free_text, int *error) {
       //printf("Text size is %d B\n",n);
       this->n = length+1;
       this->samplerate = samplerate;
       uchar *bwt = BWT(text,free_text);
       if (bwt == NULL) *error = 1;

//       uchar *bwt = BWT(text,lsa);
//       uchar *bwt = BWT(text,n,samplerate,&sampled,&suffixes,&positions);
       // caller can delete text
       // remove repeats from bwt and encode runs in a bit-vector, e.g.:
       // aaabbbbaaaacccc ==> abac, 100100010001000
       // represent the same blocks in suffix array, e.g.:
       // ==> aaa aaaa bbbb cccc ==> 100100010001000
       ulong i, min=0, max, j, k;
       //this->n = n;
       this->samplerate = samplerate;
       len = 0;
       for (i=0;i<size_uchar+1;i++)
         C[i]=0;
       for (i=0;i<n;++i)
          C[bwt[i]]++;
       bool **CC = new bool *[size_uchar+1];
       for (i=0;i<size_uchar+1;i++)
          if (C[i]>0)
             CC[i] = new bool[C[i]];
          else
       CC[i] = NULL;

       uchar *bwts;
       bool *B = new bool[n+1];
       B[0] = 1; ++len;
       for (i=1;i<n;i++)
          if (bwt[i]!=bwt[i-1]) { B[i] = 1; ++len;} else B[i] = 0;
       B[n] = 1;
       for (i=0;i<size_uchar+1;i++)
          C[i]=0;
       for (i=0;i<n;i++)
          CC[bwt[i]][C[bwt[i]]++] = B[i];
       bool *BB = new bool[n+1];
       k = 0;
       for (i=0;i<size_uchar+1;i++)
          for (j=0;j<C[i];j++)
       BB[k++] = CC[i][j];
       BB[n] = 1;
       for (i=0;i<size_uchar+1;i++)
          if (CC[i]!= NULL) delete [] CC[i];
       delete [] CC;
       bwts = new uchar[len+1];
       j = 0;
       for (i=0;i<n;i++)
          if (B[i]) bwts[j++]=bwt[i];
       delete [] bwt;
       ulong *Binbits = new ulong[n/W+1];
       for (i=0;i<n;i++)
          SetField(Binbits,1,i,B[i]);
       delete [] B;
       runs_bwt = new BitRankF(Binbits,n,true);
       Binbits = new ulong[(n+1)/W+2];
       for (i=0;i<(n+1)/W+2;i++)
          Binbits[i]=0;
       for (i=0;i<n;i++)
          SetField(Binbits,1,i,BB[i]);
       delete [] BB;
       runs_sa = new BitRankF(Binbits,n,true);
       // recompute C
       for (i=0;i<size_uchar+1;i++)
          C[i]=0;
       for (i=0;i<len;++i)
          C[bwts[i]]++;
       for (i=0;i<size_uchar+1;i++)
          if (C[i]>0) {min = i; break;}
       for (i=size_uchar;i>=min;--i)
          if (C[i]>0) {max = i; break;}
       ulong prev=C[0], temp;
       C[0]=0;
       for (i=1;i<size_uchar+1;i++) {
          temp = C[i];
          C[i]=C[i-1]+prev;
          prev = temp;
       }
       codetable = makecodetable(bwts,len);
       alphabetrank = new THuffAlphabetRank(bwts,len,codetable,0);
       delete [] bwts;
       // to avoid running out of ulong, the sizes are computed in specific order (large/small)*small
       // |class TRLFMindex| +256*|TCodeEntry|+|C[]|+|suffixes[]+positions[]|+...
//       printf("RLFMindex takes %d B\n",9*W/8+256*3*W/8+256*W/8+(2*n/(samplerate*8))*W+ sampled->SpaceRequirementInBits()/8 + alphabetrank->SpaceRequirementInBits()/8 +
//          runs_bwt->SpaceRequirementInBits()/8 + runs_sa->SpaceRequirementInBits()/8);
//       fflush(stdout);
       *error = 0;
    };
    int count(uchar *pattern, ulong m) {
       // use the FM-search replacing function Occ(c,1,i) with alphabetrank->rank(c,i)
       uchar *pat=pattern;
       if (map0 != 0 )
         pat = pattern0(pattern,m);

       ulong c = pat[m-1]; ulong i=m-1;
       ulong rank;
       long sp = runs_sa->select(C[c]+1);
       //if (C[c+1]+1 >524200) printf("valor %d\n",C[c+1]+1); fflush(stdout);
       long ep = runs_sa->select(C[c+1]+1)-1;
       while (sp<=ep && i>=1) {
          c = pat[--i];
          rank = runs_bwt->rank(sp)-1;
          if (alphabetrank->IsCharAtPos(c,rank)) // if sp is inside the run of c
             sp = sp-runs_bwt->prev(sp);
          else
             sp = 0;
          sp += runs_sa->select(C[c]+alphabetrank->rank(c,rank-1)+1);
          rank = runs_bwt->rank(ep)-1;
          if (alphabetrank->IsCharAtPos(c,rank)) // if ep is inside the run of c
             ep = ep-runs_bwt->prev(ep);
          else
             ep = -1;
          ep += runs_sa->select(C[c]+alphabetrank->rank(c,rank-1)+1);
       }
       if (map0 != 0) free(pat);
       if (sp<=ep) {
          return ep-sp+1;
       } else {
          return 0;
       }
    };
    int locate(uchar *pattern, ulong m, ulong **occ) {
       // use the FM-search replacing function Occ(c,1,i) with alphabetrank->rank(c,i)
       uchar *pat=pattern;
       if (map0 != 0 )
         pat = pattern0(pattern,m);

       ulong c = pat[m-1]; long i=m-1;
       ulong rank;
       long sp = runs_sa->select(C[c]+1);
       long ep = runs_sa->select(C[c+1]+1)-1;
       while (sp<=ep && i>=1) {
          c = pat[--i];
          rank = runs_bwt->rank(sp)-1;
          if (alphabetrank->IsCharAtPos(c,rank)) // if sp is inside the run of c
             sp = sp-runs_bwt->prev(sp);
          else
             sp = 0;
          sp += runs_sa->select(C[c]+alphabetrank->rank(c,rank-1)+1);
          rank = runs_bwt->rank(ep)-1;
          if (alphabetrank->IsCharAtPos(c,rank)) // if ep is inside the run of c
             ep = ep-runs_bwt->prev(ep);
          else
             ep = -1;
          ep += runs_sa->select(C[c]+alphabetrank->rank(c,rank-1)+1);
       }
       if (map0 != 0) free(pat);
       if (sp<=ep) {
       	  ulong matches = ep-sp+1;
       	  ulong locate=0;
       	  *occ = (ulong *) malloc(matches*sizeof(ulong));
          i=sp;
          ulong j,dist,rank_tmp;
          while (i<=ep) {
             j=i,dist=0;
             while (!sampled->IsBitSet(j)) { // LF-mapping
                rank = runs_bwt->rank(j)-1;
                c = alphabetrank->charAtPos2(rank,&rank_tmp);
                j = runs_sa->select(C[c]+rank_tmp)+j-runs_bwt->prev(j);
                ++dist;
             }
             if (dist>=samplerate || suffixes[sampled->rank(j)-1] % samplerate != 0) printf("error\n");
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
    ulong get_length () {
      return n-1; // there is a '\0' at n
    }
    int extract(ulong from, ulong to, uchar **snippet) {
        ulong rank;
        if (to> n) to=n-2;
        if (from > to) {
                *snippet = NULL;
                return 0;
        }
        ulong len =to-from+1;
        *snippet = (uchar *) malloc(len*sizeof(uchar));
        uchar c;
        ulong skip,rank_tmp;
        ulong j = positions[to/samplerate+1];
        if ((to/samplerate+1) == ((n-1)/samplerate+1))
           skip = n-1 - to;
        else
           skip = samplerate-to%samplerate-1;
        for (ulong dist=0;dist<skip+len;dist++) {
          rank = runs_bwt->rank(j)-1;
          c = alphabetrank->charAtPos2(rank,&rank_tmp);
          j = runs_sa->select(C[c]+rank_tmp)+j-runs_bwt->prev(j);
          if (dist>= skip) {
            if (c == map0)
              (*snippet)[len-dist-1+skip]='\0';
            else
              (*snippet)[len-dist-1+skip]=c;
            }
        }
        return len;
     }
    int display(uchar *pattern, ulong m, ulong numc, ulong *numocc, uchar **snippet_text, ulong **snippet_lengths, ulong **occ) {
       // use the FM-search replacing function Occ(c,1,i) with alphabetrank->rank(c,i)
       uchar *pat=pattern;
       if (map0 != 0 )
         pat = pattern0(pattern,m);
       ulong c = pat[m-1]; long i=m-1;
       ulong rank;
       long sp = runs_sa->select(C[c]+1);
       long ep = runs_sa->select(C[c+1]+1)-1;
       while (sp<=ep && i>=1) {
          c = pat[--i];
          rank = runs_bwt->rank(sp)-1;
          if (alphabetrank->IsCharAtPos(c,rank)) // if sp is inside the run of c
             sp = sp-runs_bwt->prev(sp);
          else
             sp = 0;
          sp += runs_sa->select(C[c]+alphabetrank->rank(c,rank-1)+1);
          rank = runs_bwt->rank(ep)-1;
          if (alphabetrank->IsCharAtPos(c,rank)) // if ep is inside the run of c
             ep = ep-runs_bwt->prev(ep);
          else
             ep = -1;
          ep += runs_sa->select(C[c]+alphabetrank->rank(c,rank-1)+1);
       }
       if (map0 != 0) free(pat);
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
             while (!sampled->IsBitSet(j)) { // LF-mapping
                rank = runs_bwt->rank(j)-1;
                c = alphabetrank->charAtPos2(rank,&rank_tmp);
                j = runs_sa->select(C[c]+rank_tmp)+j-runs_bwt->prev(j);
                ++dist;
             }
             //if (dist>=samplerate || suffixes[sampled->rank(j)-1] % samplerate != 0) printf("error\n");
             x=suffixes[sampled->rank(j)-1]+dist;
             (*occ)[locate]=x;
             ulong from;
             if (x>numc) from = x-numc;
             else from=0;
             ulong to=min(x+m+numc-1,n-2); // n-1 is '\0'
             ulong len =to-from+1;
             ulong skip;
             ulong j = positions[to/samplerate+1];
             if ((to/samplerate+1) == ((n-1)/samplerate+1))
               skip = n-1 - to;
             else
               skip = samplerate-to%samplerate-1;
             for (ulong dist=0;dist<skip+len;dist++) {
               rank = runs_bwt->rank(j)-1;
               c = alphabetrank->charAtPos2(rank,&rank_tmp);
               j = runs_sa->select(C[c]+rank_tmp)+j-runs_bwt->prev(j);
               if (dist>= skip) {
                 if (c == map0)
                   text_aux[len-dist-1+skip]='\0';
                 else
                   text_aux[len-dist-1+skip]=c;
                 }
             }
             (*snippet_lengths)[locate]=len;
             text_aux+=m+2*numc;
             locate ++ ;
             ++i;
          }
          return ep-sp+1;
       } else {
          *snippet_lengths = NULL;
          *snippet_text = NULL;
          *occ = NULL;
          return 0;
       }
    };
    ulong SpaceRequirement(){
      return sizeof(TRLFMindex)+alphabetrank->SpaceRequirementInBits()/8+
             runs_bwt->SpaceRequirementInBits()/8+runs_sa->SpaceRequirementInBits()/8+
             sampled->SpaceRequirementInBits()/8+sizeof(codetable)*size_uchar+
             (this->n/samplerate+1)*2*sizeof(ulong); //suffix + pos 
    }
    ulong SpaceRequirement_locate(){
      return sizeof(TRLFMindex)+alphabetrank->SpaceRequirementInBits()/8+
             runs_bwt->SpaceRequirementInBits()/8+runs_sa->SpaceRequirementInBits()/8+
             sampled->SpaceRequirementInBits()/8+sizeof(codetable)*size_uchar+
             (this->n/samplerate+1)*sizeof(ulong); //suffix 
    }
    ulong SpaceRequirement_count(){
      return sizeof(TRLFMindex)+alphabetrank->SpaceRequirementInBits()/8+
             runs_bwt->SpaceRequirementInBits()/8+runs_sa->SpaceRequirementInBits()/8+
             sizeof(codetable)*size_uchar;
    }

    ~TRLFMindex() {
       delete alphabetrank;
       delete runs_bwt;
       delete runs_sa;
       delete sampled;
       delete [] codetable;
       delete [] suffixes;
       delete [] positions;
    };
     int save(char *filename) {
        char fnamext[1024];
        FILE *f;
        sprintf (fnamext,"%s.rl",filename);
        f = fopen (fnamext,"w");
        if (f == NULL) return 20;
        if (fwrite (&n,sizeof(ulong),1,f) != 1) return 21;
        if (fwrite (&len,sizeof(ulong),1,f) != 1) return 21;
        if (fwrite (&samplerate,sizeof(ulong),1,f) != 1) return 21;
        if (fwrite (&map0,sizeof(uchar),1,f) != 1) return 21;
        if (fwrite (C,sizeof(ulong),size_uchar+1,f) != size_uchar+1) return 21;
        if (save_codetable(f,codetable) !=0) return 21;
        if (alphabetrank->save(f) !=0) return 21;
        if (sampled->save(f) !=0) return 21;
        if (runs_bwt->save(f) !=0) return 21;
        if (runs_sa->save(f) !=0) return 21;
        if (fwrite (suffixes,sizeof(ulong),this->n/samplerate+1,f) != this->n/samplerate+1) return 21;
        if (fwrite (positions,sizeof(ulong),this->n/samplerate+2,f) != this->n/samplerate+2) return 21;
        fclose(f);
        return 0;
     }

     int load(char *filename) {
        char fnamext[1024];
        FILE *f;
        int error;
        sprintf (fnamext,"%s.rl",filename);
        f = fopen (fnamext,"r");
        if (f == NULL) return 23;
        if (fread (&n,sizeof(ulong),1,f) != 1) return 25;
        if (fread (&len,sizeof(ulong),1,f) != 1) return 25;
        if (fread (&samplerate,sizeof(ulong),1,f) != 1) return 25;
        if (fread (&map0,sizeof(uchar),1,f) != 1) return 25;
        if (fread (C,sizeof(ulong),size_uchar+1,f) != size_uchar+1) return 25;
        if (load_codetable(f,&codetable) !=0) return 25;
        alphabetrank = new THuffAlphabetRank(f,codetable,&error);
        if (error !=0) return error;
        sampled = new BitRankF(f,&error);
        if (error !=0) return error;
        runs_bwt = new BitRankF(f,&error);
        if (error !=0) return error;
        runs_sa = new BitRankF(f,&error);
        if (error !=0) return error;
        this->suffixes = new ulong[this->n/samplerate+1];
        if (!this->suffixes) return 1;
        if (fread (suffixes,sizeof(ulong),this->n/samplerate+1,f) != this->n/samplerate+1) return 25;
        this->positions = new ulong[this->n/samplerate+2];
        if (!this->positions) return 1;
        if (fread (positions,sizeof(ulong),this->n/samplerate+2,f) != this->n/samplerate+2) return 25;
        fclose(f);
        return 0;
     }

     TRLFMindex(char *filename, int *error) {
        *error = load(filename);
     }
};

////////////////////////
////Building the Index//
////////////////////////
int build_index(uchar *text, ulong length, char *build_options, void **index){
  ulong samplerate=64;
  int error;
  char delimiters[] = " =;";
  int j,num_parameters;
  char ** parameters;
  bool free_text=false; /* don't free text by default */

  if (build_options != NULL) {
    parse_parameters(build_options,&num_parameters, &parameters, delimiters);
    for (j=0; j<num_parameters;j++) {
      if ((strcmp(parameters[j], "samplerate") == 0 ) && (j < num_parameters-1) ) {
        samplerate=atoi(parameters[j+1]);
        j++;
      } else if (strcmp(parameters[j], "free_text") == 0 )
        free_text=true;
    }
    free_parameters(num_parameters, &parameters);
  }
  TRLFMindex *RLFMindex = new TRLFMindex(text,length,samplerate,free_text,&error);
  (*index) = RLFMindex;
  if (error != 0) return error;
  return 0;
}

int save_index(void *index, char *filename) {
  TRLFMindex *_index=(TRLFMindex *) index;
  return _index->save(filename);
}
int load_index(char *filename, void **index){
  int error;
  TRLFMindex *RLFMindex = new TRLFMindex(filename, &error);
  (*index) = RLFMindex;
  return error;
}

int free_index(void *index){
  TRLFMindex *_index=(TRLFMindex *) index;
  delete _index;
  return 0;
}

int index_size(void *index, ulong *size) {
  TRLFMindex *_index=(TRLFMindex *) index;
  (*size) = _index->SpaceRequirement();
  return 0;
}
int index_size_count(void *index, ulong *size) {
  TRLFMindex *_index=(TRLFMindex *) index;
  (*size) = _index->SpaceRequirement_count();
  return 0;
}
int index_size_locate(void *index, ulong *size) {
  TRLFMindex *_index=(TRLFMindex *) index;
  (*size) = _index->SpaceRequirement_locate();
  return 0;
}
////////////////////////
////Querying the Index//
////////////////////////
int count(void *index, uchar *pattern, ulong length, ulong *numocc){
  TRLFMindex *_index=(TRLFMindex *) index;
  (*numocc)= _index->count(pattern,length);
  return 0;
}
int locate(void *index, uchar *pattern, ulong length, ulong **occ, ulong *numocc){
  TRLFMindex *_index=(TRLFMindex *) index;
  (*numocc)= _index->locate(pattern,length,occ);
  return 0;
}
/////////////////////////
//Accessing the indexed//
/////////////////////////
int extract(void *index, ulong from, ulong to, uchar **snippet, ulong *snippet_length){
  TRLFMindex *_index=(TRLFMindex *) index;
  (*snippet_length)= _index->extract(from,to, snippet);
  return 0 ;
}
int display(void *index, uchar *pattern, ulong length, ulong numc, ulong *numocc, uchar **snippet_text, ulong **snippet_lengths) {
  TRLFMindex *_index=(TRLFMindex *) index;
  ulong *occ;
  _index->display(pattern, length, numc, numocc, snippet_text, snippet_lengths, &occ);
  free(occ);
  return 0;
}
int get_length(void *index, ulong *length){
  TRLFMindex *_index=(TRLFMindex *) index;
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

