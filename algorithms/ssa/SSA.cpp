/* SSA.cpp
   Copyright (C) 2005, Rodrigo Gonzalez, all rights reserved.

   This file contains an implementation of a succinct full-text index: SSA-index.
   Based on a version programmed by Veli Makinen.
   http://www.cs.helsinki.fi/u/vmakinen/software/
   For more information, see [MN04]. 

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
   [MN04] Veli Makinen and Gonzalo Navarro. New search algorithms and space/time tardeoffs
          for succinct suffix arrays Technical report C-2004-20, Dept. CS, Univ. Helsinki, April 2004.

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
#include "386.c"
#include "basic.h"
#include "ds/ds_ssort.h"
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
 * succinct full-text index                                                                               *
 **********************************************************************************************************/


class TFMindex {
  private:
    ulong n;
    ulong samplerate;
    ulong sampletext;
    ulong C[size_uchar+1];
    TCodeEntry *codetable;
    THuffAlphabetRank *alphabetrank;
    BitRankW32Int *sampled;
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
    uchar *BWT(uchar *text, bool free_text,ulong factor, bool withload, char *fnamext) {
      ulong i;
      int overshoot;
      ulong *sa;
      /* Make suffix array */
      uchar *x;
      overshoot = init_ds_ssort(500, 2000);
      sa= (ulong *) malloc (sizeof(ulong)*(this->n+1));
      if (!sa) return NULL;
      x= (uchar *) malloc (sizeof(uchar)*(this->n+overshoot)); 
      if (!x) return NULL;
      x[this->n-1]=0;
      for (i=0;i<this->n-1;i++) x[i]=text[i];
      if (free_text) free(text);
      /* Remap 0 if exist */
      map0=remap0(x,this->n-1); // the text contain a 0 at the end
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

      uchar *s = new uchar[this->n];
      if (!s) return NULL;
      ulong *sampledpositions = new ulong[this->n/W+1];
      if (!sampledpositions) return NULL;
      this->suffixes = new ulong[this->n/samplerate+1];
      if (!this->suffixes) return NULL;
      this->positions = new ulong[this->n/sampletext+2];
      if (!this->positions) return NULL;
      ulong j=0;
      for (i=0;i<this->n/W+1;i++)
        sampledpositions[i]=0u;
      for (i=0;i<this->n;i++)
        if (sa[i] % samplerate == 0) {
          SetField(sampledpositions,1,i,1);
          this->suffixes[j] = sa[i];
          //this->positions[sa[i]/samplerate] = i;
          j++;
        }
      for (i=0;i<this->n;i++)
        if (sa[i] % sampletext == 0) 
          this->positions[sa[i]/sampletext] = i;

      positions[(this->n-1)/sampletext+1] = positions[0];
      this->sampled = new BitRankW32Int(sampledpositions,this->n,true,factor);
      if (!this->sampled) return NULL;
      for (i=0;i<this->n;i++)
         if (sa[i]==0) s[i] = '\0'; //text[this->n-1];
         else s[i] = x[sa[i]-1];
      free(x);
      free(sa);
      return s;
    }
public:
    TFMindex(uchar *text, ulong length, ulong samplerate, ulong sampletext, bool free_text,ulong factor, bool withload, char *fnamext, int *error) { 
       this->n = length+1;
       this->samplerate = samplerate;
       this->sampletext = sampletext;
       uchar *bwt = BWT(text,free_text,factor,withload,fnamext);
       if (bwt == NULL) *error = 1;
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
       codetable = makecodetable(bwt,n);
       alphabetrank = new THuffAlphabetRank(bwt,n,codetable,0,factor);
       delete [] bwt;
       *error=0;
    };
    ulong count(uchar *pattern, ulong m) {
       // use the FM-search replacing function Occ(c,1,i) with alphabetrank->rank(c,i)
       uchar *pat=pattern;
       if (map0 != 0 )
         pat = pattern0(pattern,m);
       uchar c = pat[m-1]; ulong i=m-1;
       ulong sp = C[c];
       ulong ep = C[c+1]-1;
       while (sp<=ep && i>=1) {
          c = pat[--i];
          sp = C[c]+alphabetrank->rank(c,sp-1);
          ep = C[c]+alphabetrank->rank(c,ep)-1;
       }
       if (map0 != 0) free(pat);
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
       c = alphabetrank->charAtPos2(j,&rank_tmp);
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

       // use the FM-search replacing function Occ(c,1,i) with alphabetrank->rank(c,i)
       uchar *pat=pattern;
       if (map0 != 0 )
         pat = pattern0(pattern,m);
       uchar c = pat[m-1]; ulong i=m-1;
       ulong sp = C[c];
       ulong ep = C[c+1]-1;
       while (sp<=ep && i>=1) {
          c = pat[--i];
          sp = C[c]+alphabetrank->rank(c,sp-1);
          ep = C[c]+alphabetrank->rank(c,ep)-1;
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
             while (!sampled->IsBitSet(j)) {
                c = alphabetrank->charAtPos2(j,&rank_tmp);
                j = C[c]+rank_tmp; // LF-mapping The rank_tmp is already decremented by 1
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
       uchar *pat=pattern;
       if (map0 != 0 )
         pat = pattern0(pattern,m);
       uchar c = pat[m-1]; ulong i=m-1;
       ulong sp = C[c];
       ulong ep = C[c+1]-1;
       while (sp<=ep && i>=1) {
          c = pat[--i];
          sp = C[c]+alphabetrank->rank(c,sp-1);
          ep = C[c]+alphabetrank->rank(c,ep)-1;
       }
       if (map0 != 0) free(pat);
       if (sp<=ep) {
          (*numocc) = ep-sp+1;
          ulong locate=0;
          *occ = (ulong *) malloc((*numocc)*sizeof(ulong));
          *snippet_lengths = (ulong *) malloc((*numocc)*sizeof(ulong));
          *snippet_text = (uchar *) malloc((*numocc)*(m+2*numc)*sizeof(uchar));
          uchar *text_aux=*snippet_text;

          ulong j,dist,x,rank_tmp,skip,locate_pos;
          i=sp;
          while (i<=ep) {
            j=i;
            x = 0;
            for (dist=numc;dist>0;dist--) {
              if ((x == 0) && (sampled->IsBitSet(j))) x = suffixes[sampled->rank(j)-1]+numc-dist+1; // +1 to be able to check 0
              c = alphabetrank->charAtPos2(j,&rank_tmp);
              if (c == 0) break;
              j = C[c]+rank_tmp; // LF-mapping The rank_tmp is already decremented by 1
              if (c == map0)
                text_aux[dist-1]='\0';
              else
                text_aux[dist-1]=c;
             }
             if ( x == 0) { //if not locate yet keep searching
               locate_pos=0;
               while (!sampled->IsBitSet(j)) {
                 c = alphabetrank->charAtPos2(j,&rank_tmp);
                 j = C[c]+rank_tmp; // LF-mapping The rank_tmp is already decremented by 1
                 locate_pos++;
               }
               x = suffixes[sampled->rank(j)-1]+numc-dist+1+locate_pos;
             } 
             (*occ)[locate]= x-1;

             skip=dist;
             if (dist != 0)
               for (ulong ind=0;ind< numc-dist;ind++)
                 text_aux[ind]=text_aux[dist+ind];

             
             ulong to=min(x-1+m+numc-1,n-2); // n-1 is '\0'
             ulong len =to-(x-1)+1-m;//from+1;
             ulong skip2,valor_der;
             ulong j = positions[to/sampletext+1];
             if ((to/sampletext+1) == ((n-2)/sampletext+1))
               skip2 = n-2 - to;
             else
               skip2 = sampletext-to%sampletext-1;
             valor_der=len+m+(numc-skip)+skip2-1;
             for (ulong dist=0;dist<skip2+len;dist++) {
               c = alphabetrank->charAtPos2(j,&rank_tmp);
               j = C[c]+rank_tmp; // LF-mapping The rank_tmp is already decremented by 1
               if (dist>= skip2) {
                 if (c == map0)
                   text_aux[valor_der-dist]='\0';
                 else
                   text_aux[valor_der-dist]=c;
                 }
             }
             for (ulong dist=0;dist<m;dist++) {
               c=pat[dist];
               if (c == map0)
                 text_aux[numc-skip+dist]='\0';
               else
                 text_aux[numc-skip+dist]=c;
             }
                
             (*snippet_lengths)[locate]=len+m+numc-skip;
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
    ulong get_length () {
      return n-1;
    }
    ~TFMindex() {
       delete alphabetrank;
       delete sampled;
       delete [] codetable;
       delete [] suffixes;
       delete [] positions;
    };
    ulong SpaceRequirement(){
      return sizeof(TFMindex)+alphabetrank->SpaceRequirementInBits()/8+
             sampled->SpaceRequirementInBits()/8+sizeof(codetable)*size_uchar+
             (this->n/samplerate+1)*2*sizeof(ulong);
    }
    ulong SpaceRequirement_locate(){
      return sizeof(TFMindex)+alphabetrank->SpaceRequirementInBits()/8+
             sampled->SpaceRequirementInBits()/8+sizeof(codetable)*size_uchar+
             (this->n/samplerate+1)*sizeof(ulong);
    }
    ulong SpaceRequirement_count(){
      return sizeof(TFMindex)+alphabetrank->SpaceRequirementInBits()/8+
             sizeof(codetable)*size_uchar;
    }

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
        ulong j = positions[to/sampletext+1];
        if ((to/sampletext+1) == ((n-1)/sampletext+1)) 
           skip = n-1 - to;
        else 
           skip = sampletext-to%sampletext-1;
        for (ulong dist=0;dist<skip+len;dist++) {
          c = alphabetrank->charAtPos2(j,&rank_tmp);
          j = C[c]+rank_tmp; // LF-mapping The rank_tmp is already decremented by 1
          if (dist>= skip) {
            if (c == map0)
              (*snippet)[len-dist-1+skip]='\0';
            else 
              (*snippet)[len-dist-1+skip]=c;
            }
        }
        return len;
     }
     int save(char *filename) {
        char fnamext[1024];        
        FILE *f;
        sprintf (fnamext,"%s.ssa",filename);
        f = fopen (fnamext,"w");
        if (f == NULL) return 20;
        if (fwrite (&n,sizeof(ulong),1,f) != 1) return 21;
        if (fwrite (&samplerate,sizeof(ulong),1,f) != 1) return 21;
        if (fwrite (&sampletext,sizeof(ulong),1,f) != 1) return 21;
        if (fwrite (&map0,sizeof(uchar),1,f) != 1) return 21;
        if (fwrite (C,sizeof(ulong),size_uchar+1,f) != size_uchar+1) return 21;
        if (save_codetable(f,codetable) !=0) return 21;
        if (alphabetrank->save(f) !=0) return 21;
        if (sampled->save(f) !=0) return 21;
        if (fwrite (suffixes,sizeof(ulong),this->n/samplerate+1,f) != this->n/samplerate+1) return 21;
        if (fwrite (positions,sizeof(ulong),this->n/sampletext+2,f) != this->n/sampletext+2) return 21;
        fclose(f);
        return 0;
     }

     int load(char *filename) {
        char fnamext[1024];        
        FILE *f;
        int error;
        sprintf (fnamext,"%s.ssa",filename);
        f = fopen (fnamext,"r");
        if (f == NULL) return 23;
        if (fread (&n,sizeof(ulong),1,f) != 1) return 25;
        if (fread (&samplerate,sizeof(ulong),1,f) != 1) return 25;
        if (fread (&sampletext,sizeof(ulong),1,f) != 1) return 25;
        if (fread (&map0,sizeof(uchar),1,f) != 1) return 25;
        if (fread (C,sizeof(ulong),size_uchar+1,f) != size_uchar+1) return 25;
        if (load_codetable(f,&codetable) !=0) return 25;
        alphabetrank = new THuffAlphabetRank(f,codetable,&error);
        if (error !=0) return error;
        sampled = new BitRankW32Int(f,&error);
        if (error !=0) return error;
        this->suffixes = new ulong[this->n/samplerate+1];
        if (!this->suffixes) return 1;
        if (fread (suffixes,sizeof(ulong),this->n/samplerate+1,f) != this->n/samplerate+1) return 25;
        this->positions = new ulong[this->n/sampletext+2];
        if (!this->positions) return 1;
        if (fread (positions,sizeof(ulong),this->n/sampletext+2,f) != this->n/sampletext+2) return 25;
        fclose(f);
        return 0;
     }

     TFMindex(char *filename, int *error) {
        *error = load(filename);
     }
};

////////////////////////
////Building the Index//
////////////////////////
int build_index(uchar *text, ulong length, char *build_options, void **index){
  ulong samplerate=64;
  ulong sampletext=64;
  ulong factor=20; 
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
      } else if ((strcmp(parameters[j], "sampletext") == 0 ) && (j < num_parameters-1) ) {
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
  TFMindex *FMindex = new TFMindex(text,length,samplerate,sampletext,free_text,factor,withload, fnamext, &error);
  (*index) = FMindex;
  if (error != 0) return error;
  return 0;
}
int save_index(void *index, char *filename) {
  TFMindex *_index=(TFMindex *) index;
  return _index->save(filename);
}
int load_index(char *filename, void **index){
  int error;
  TFMindex *FMindex = new TFMindex(filename, &error);
  (*index) = FMindex;
  return error;
}

int free_index(void *index){
  TFMindex *_index=(TFMindex *) index;
  delete _index;
  return 0;
}

int index_size(void *index, ulong *size) {
  TFMindex *_index=(TFMindex *) index;
  (*size) = _index->SpaceRequirement();
  return 0;
}
int index_size_count(void *index, ulong *size) {
  TFMindex *_index=(TFMindex *) index;
  (*size) = _index->SpaceRequirement_count();
  return 0;
}
int index_size_locate(void *index, ulong *size) {
  TFMindex *_index=(TFMindex *) index;
  (*size) = _index->SpaceRequirement_locate();
  return 0;
}
////////////////////////
////Querying the Index//
////////////////////////
int count(void *index, uchar *pattern, ulong length, ulong *numocc){
  TFMindex *_index=(TFMindex *) index;
  (*numocc)= _index->count(pattern,length);
  return 0;
}
int locate(void *index, uchar *pattern, ulong length, ulong **occ, ulong *numocc){
  TFMindex *_index=(TFMindex *) index;
  (*numocc)= _index->locate(pattern,length,occ);
  return 0;
}
/////////////////////////
//Accessing the indexed//
/////////////////////////
int extract(void *index, ulong from, ulong to, uchar **snippet, ulong *snippet_length){
  TFMindex *_index=(TFMindex *) index;
  (*snippet_length)= _index->extract(from,to, snippet);
  return 0 ;
}
int display(void *index, uchar *pattern, ulong length, ulong numc, ulong *numocc, uchar **snippet_text, ulong **snippet_lengths) {
  TFMindex *_index=(TFMindex *) index;
  ulong *occ;
  int aux = _index->display(pattern, length, numc, numocc, snippet_text, snippet_lengths, &occ);
  free(occ);
  return aux;
}
int get_length(void *index, ulong *length){
  TFMindex *_index=(TFMindex *) index;
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

