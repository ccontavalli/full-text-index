/* SAu.c
   Copyright (C) 2005, Rodrigo Gonzalez, all rights reserved.

   This file contains an implementation of Suffix Array in a uncompressed
   form.

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
   This library uses a suffix array construction algorithm developed by
   Giovanni Manzini (Universit del Piemonte Orientale) and
   Paolo Ferragina (Universit di Pisa).
   You can find more info at:
   http://roquefort.di.unipi.it/~ferrax/SuffixArray/
*/

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include "bpe.h"
#include <fcntl.h>
#include <math.h>

#include "utils/interface.h"


#ifndef uchar
#define uchar unsigned char
#endif
#ifndef ulong
#define ulong unsigned long
#endif

typedef struct
{
  BPE *bpe;                      //SA compressed
  ulong *pos;                    //sampled positions
  uchar *text;                   //text
  ulong n;
  ulong samplerate;
  ulong ns;
  bool own;
} TSA_Un;

/*/////////////////////
//External procedures//
/////////////////////*/
extern void ds_ssort(uchar *x, ulong *p, long n);
extern int init_ds_ssort(int adist, int bs_ratio);

/*/////////////////////
//Internal procedures//
/////////////////////*/
static inline void SetPos(TSA_Un *index, ulong position, ulong x) {
  index->pos[position]=x;
}


static inline ulong GetPos(TSA_Un *index, ulong position) {
  return index->pos[position];
}


static inline int ComparePos2(TSA_Un *index, ulong position, uchar *pattern, ulong m) {
  ulong n=index->n;
  uchar *text=index->text;
  ulong j=0;

  while (position+j < n && j < m && text[position+j]==pattern[j]) {
    j++;
  }
  if (j >= m)
    return 0;
  else if (position+j >= n)
    return +1;
  else if (text[position+j]>pattern[j])
    return -1;
  else
    return +1;
}

static inline int ComparePos(TSA_Un *index, ulong position, uchar *pattern, ulong m) {
  ulong n=min(index->n-position,m);
  char *text=(char*) (index->text+position);
  int aux2;
  aux2 = strncmp((char *) pattern,text,n);
  if (aux2 != 0) {
    return aux2;
  } else 
    return n<m;
}  


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


/*////////////////////
//Building the Index//
////////////////////*/
int build_index(uchar *text, ulong length, char *build_options, void **index) {
  /*if (text[length-1]!='\0') return 2;*/
  ulong i, *p, *sa_diff;
  long overshoot;
  TSA_Un *_index= (TSA_Un *) malloc(sizeof(TSA_Un));
  uchar *x;
  FILE *f;
  char fnamext[1024];
  char fnameaux[1024];
  char delimiters[] = " =;";
  int j,num_parameters;
  char ** parameters;
  int copy_text=false;           /* don't copy text by default */
  int free_text=false;           /* don't free text by default */
  int withload=false;           /* don't load SA and BPE by default */
  int samplerate=64;             /* samplerate for bpe */
  int s=8;             /* s alpha=1/(s*log n) */
  int delta=4;             /* How threshold going down */
  int gamma=bits(length+1);             /* How threshold going down */
  int max_phrase=256;
  bool verbose=false;
  double cutoff=100.0;
  bool SA_treap=true,SA_psi=false;
  if (!_index) return 1;
  if (build_options != NULL) {
    parse_parameters(build_options,&num_parameters, &parameters, delimiters);
    for (j=0; j<num_parameters;j++) {
      if (strcmp(parameters[j], "copy_text") == 0 )
        copy_text=true;
      else if (strcmp(parameters[j], "withload") == 0 )
        withload=true;
      else if (strcmp(parameters[j], "filename") == 0 ) {
        strcpy(fnamext,parameters[j+1]);
        j++;
      } else if ((strcmp(parameters[j], "s") == 0 ) && (j < num_parameters-1) ) {
        s=atoi(parameters[j+1]);
        j++;
      } else if ((strcmp(parameters[j], "delta") == 0 ) && (j < num_parameters-1) ) {
        delta=atoi(parameters[j+1]);
        j++;
      } else if ((strcmp(parameters[j], "gamma") == 0 ) && (j < num_parameters-1) ) {
        gamma=atoi(parameters[j+1]);
        j++;
      } else if ((strcmp(parameters[j], "samplerate") == 0 ) && (j < num_parameters-1) ) {
        samplerate=atoi(parameters[j+1]);
        j++;
      } else if ((strcmp(parameters[j], "max_phrase") == 0 ) && (j < num_parameters-1) ) {
        max_phrase=atoi(parameters[j+1]);
        j++;
      } else if ((strcmp(parameters[j], "cutoff") == 0 ) && (j < num_parameters-1) ) {
        cutoff=atof(parameters[j+1]);
        j++;
      } else if (strcmp(parameters[j], "free_text") == 0 )
        free_text=true;
      else if (strcmp(parameters[j], "verbose") == 0 )
        verbose=true;
      else if (strcmp(parameters[j], "SA_treap") == 0 ) {
        SA_treap=true;SA_psi=false;
      } else if (strcmp(parameters[j], "SA_psi") == 0 ) {
        SA_treap=false;SA_psi=true;
      }
    }
    free_parameters(num_parameters, &parameters);
  }
  //printf("samplerate = %lu\n",samplerate);
  /* Consistence of parameters  */
  if ((!copy_text) && (free_text))
    return 5;
  /*                            */
  if ( !copy_text ) {
    _index->text = text;
    _index->own=false;
  }
  else {
    _index->text = (uchar *) malloc(sizeof(uchar)*length);
    if (!_index->text) return 1;
    for (i=0;i<length;i++) _index->text[i]=text[i];
    _index->own=true;
  }
  if ( free_text )
    free(text);

  _index->n=length;

  /* Make suffix array */
  if (withload) {
    ulong filename_len;
    p= (ulong *) malloc (sizeof(ulong)*(length));
    if (!p) return 1;
    sprintf (fnameaux,"%s.sa",fnamext);
    f = fopen (fnameaux,"r");
    if (fread (&filename_len,sizeof(ulong),1,f) != 1) return 25;
    assert(filename_len==_index->n);
    if (fread (p,sizeof(ulong),filename_len,f) != filename_len) return 25;
    if (fclose(f) != 0) return 28;
  } else {
    overshoot = init_ds_ssort(500, 2000);
    p= (ulong *) malloc (sizeof(ulong)*(length));
    if (!p) return 1;
    x= (uchar *) malloc (sizeof(uchar)*(length+overshoot));
    if (!x) return 1;
    for (i=0;i<length;i++) x[i]=_index->text[i];
    ds_ssort( x, p, _index->n);
    free(x);
  }

  /* Make bpe */
  if (withload && false ) {
    int error;
    sprintf (fnameaux,"%s.bpe",fnamext);
    f = fopen (fnameaux,"r");
    _index->bpe = new BPE(f,&error);
    if (error !=0) return error;
    if (fclose(f) != 0) return 28;
  } else {
    if (SA_treap) {
      sa_diff= (ulong *) malloc (sizeof(ulong)*(length+3));
      if (!sa_diff) return 1;
      for (i=0;i<length-1;i++) {
        assert(p[i+1]-p[i]+length>0);
        sa_diff[i+1]=p[i+1]-p[i]+length;
      }
      free(p);
      ulong maximo=0;
      for (i=0;i<length-1;i++) {
        if (maximo < sa_diff[i+1]) maximo=sa_diff[i+1];
      }
      sa_diff[0]=maximo+1;
      sa_diff[length+1]=maximo+2;
      sa_diff[length+2]=maximo+3;

      _index->bpe = new BPE(sa_diff,length-1+3, max_phrase, cutoff, verbose);
    }
    if (SA_psi) {
      ulong *ip= (ulong *) malloc (sizeof(ulong)*(length));
      for (i=0;i<length;i++) ip[p[i]] = i;
      for (i=0;i<length;i++) assert(ip[p[i]] == i);
      ulong *Psi= (ulong *) malloc (sizeof(ulong)*(length));
      for (i=0;i<length;i++) if (p[i] == length-1) Psi[i] = ip[0];
                            else Psi[i] = ip[p[i]+1];

      ulong ini=ip[0];
      free(ip);

      sa_diff= (ulong *) malloc (sizeof(ulong)*length);
      if (!sa_diff) return 1;
      for (i=0;i<length-1;i++) {
        assert(p[i+1]-p[i]+length>0);
        sa_diff[i]=p[i+1]-p[i]+length;
      }
      free(p);
      _index->bpe = new BPE(sa_diff,Psi,ini,length-1, max_phrase, cutoff, s, delta, gamma, verbose);
    }
  }

  /* Make suffix array again */
  if (withload) {
    ulong filename_len;
    p= (ulong *) malloc (sizeof(ulong)*(length));
    if (!p) return 1;
    sprintf (fnameaux,"%s.sa",fnamext);
    f = fopen (fnameaux,"r");
    if (fread (&filename_len,sizeof(ulong),1,f) != 1) return 25;
    assert(filename_len==_index->n);
    if (fread (p,sizeof(ulong),filename_len,f) != filename_len) return 25;
    if (fclose(f) != 0) return 28;
  } else {
    overshoot = init_ds_ssort(500, 2000);
    p= (ulong *) malloc (sizeof(ulong)*(length));
    if (!p) return 1;
    x= (uchar *) malloc (sizeof(uchar)*(length+overshoot));
    if (!x) return 1;
    for (i=0;i<length;i++) x[i]=_index->text[i];
    ds_ssort( x, p, _index->n);
    free(x);
  }

/*
////////////////////////////////////////////////////
      sa_diff= (ulong *) malloc (sizeof(ulong)*length);
      for (i=0;i<length-1;i++){
         assert(p[i+1]-p[i]+length>0);
         sa_diff[i]=p[i+1]-p[i]+length;
      }
      ulong *z2;
      z2=_index->bpe->dispairall();
      printf("Check SA_diff todo\n");
      for (i=0;i<length-1;i++){
        if (z2[i]-sa_diff[i] !=0) {printf("%lu, %lu         %lu,%lu\n",i, z2[i]-sa_diff[i],z2[i],sa_diff[i]);fflush(stdout);}
      }
      printf("End Check SA_diff todo\n");
      free(z2);

      printf("End Check SA_diff 2\n");
      for (ulong mmm=1; mmm < 5000; mmm++) {
        printf("Check SA_diff %lu ",mmm);
        for (i=0;i<length-1-(mmm-1);i++){
          z2=_index->bpe->dispair(i,mmm);
          //if (i % (n/10) == 0)  {printf("C2 %lu\n",i);fflush(stdout);}
          for (ulong mm =1 ; mm <= mmm; mm++)
                if (z2[mm]-sa_diff[i+mm-1] !=0) {printf("T%lu %lu, %lu         %lu,%lu\n",mm,i, z2[mm]-sa_diff[i+mm-1],z2[mm],sa_diff[i+mm-1]);fflush(stdout);}
          free(z2);
        }
        printf("End Check SA_diff %lu\n",mmm);fflush(stdout);
      }

      free(sa_diff);



/////////////////////////////////////////////////////////
*/
  /* Make samplerate */

  _index->samplerate = samplerate;
  _index->ns = (length-1)/samplerate+1;
  if (((length-1) % samplerate) != 0) _index->ns++;
  _index->pos = (ulong *) malloc (sizeof(ulong)*_index->ns);
  //_index->pos[0]=p[0];
  j=0;
  for (i=0; i < length ; i+=samplerate) {
    if (i != length-1) {
      //if (p[_index->bpe->BR->prev(i)] != _index->pos[j-1]) {
        _index->pos[j]=p[_index->bpe->BR->prev(i)];
        j++;
     // }
    } else {
      _index->pos[j]=p[i];
      j++;
    }
  }
  if (((length-1) % samplerate) != 0) _index->pos[j]=p[length-1];
  _index->ns=j+1;


/*
  _index->samplerate = samplerate;
  _index->ns = (length-1)/samplerate+1;
  if (((length-1) % samplerate) != 0) _index->ns++; 
  _index->pos = (ulong *) malloc (sizeof(ulong)*_index->ns);
  j=0;
  for (i=0; i < length ; i+=samplerate) {
    _index->pos[j]=p[i];
    j++;
  }
  if (((length-1) % samplerate) != 0) _index->pos[j]=p[length-1];
    
  assert(j+1==_index->ns);
*/

  free(p);
  (*index) = _index;
  return 0;
}


int save_index(void *index, char *filename) {
  TSA_Un *_index=(TSA_Un *) index;
  char fnamext[1024];
  FILE *f;
  sprintf (fnamext,"%s.sa_bpe",filename);
  f = fopen (fnamext,"w");
  if (f == NULL) return 20;
  if (fwrite (&_index->n,sizeof(ulong),1,f) != 1) return 21;
  if (fwrite (&_index->ns,sizeof(ulong),1,f) != 1) return 21;
  if (fwrite (&_index->samplerate,sizeof(ulong),1,f) != 1) return 21;
  if (fwrite (_index->pos,sizeof(ulong),_index->ns,f) != _index->ns) return 21;
  if (_index->bpe->save(f) !=0) return 21;
  if (fclose(f) != 0) return 22;
  return 0;
}

int index_size(void *index, ulong *size);

int load_index(char *filename, void **index) {
  char fnamext[1024];
  int error;
  FILE *f;
  struct stat sdata;
  TSA_Un *_index;
  ulong file_size;
  /* Read index */
  sprintf (fnamext,"%s.sa_bpe",filename);
  f = fopen (fnamext,"r");
  if (f == NULL) return 23;
  _index= (TSA_Un *) malloc(sizeof(TSA_Un));
  if (!_index) return 1;
  if (fread (&_index->n,sizeof(ulong),1,f) != 1) return 25;
  if (fread (&_index->ns,sizeof(ulong),1,f) != 1) return 25;
  if (fread (&_index->samplerate,sizeof(ulong),1,f) != 1) return 25;
  //printf("samplerate is %lu\n", _index->samplerate);
  _index->pos= (ulong *) malloc (sizeof(ulong)*_index->ns);
  if (!_index->pos) return 1;
  if (fread (_index->pos,sizeof(ulong),_index->ns,f) != _index->ns) return 25;
  _index->bpe = new BPE(f,&error);
/*
  printf("Costo promedio de descompresion %f\n",_index->bpe->avg_cost()); fflush(stdout);
  printf("bpe usage %lu\n",_index->bpe->SpaceRequirement()); fflush(stdout);
  printf("Samplerate %lu\n",_index->samplerate); fflush(stdout);
  printf("Number of samples %lu\n",_index->ns);
  printf("Space sample usage %lu\n",_index->ns*4);
*/
  //_index->ns = (_index->n-1)/32+1; if (((_index->n-1) % 32) != 0) _index->ns++;
  //ulong borrame;
  //index_size(_index, &borrame);
  //printf("Index size %lu\n", borrame);
  //printf("Index size %f\n", borrame/(float)1024.0/1024.0);
  //printf("Index size %f\n", borrame/104857600.0/4.0*100);
  //exit(0);
  
  
  if (error !=0) return error;
  if (fclose(f) != 0) return 26;
  // save BR
  /*
  char fnamext1[1024];
  FILE *f1;
  sprintf (fnamext1,"%s.br",filename);
  f1 = fopen (fnamext1,"w");
  if (_index->bpe->BR->save(f1) !=0) return 21;
  if (fclose(f1) != 0) return 22;
  exit(0);
  */
  // end save BR

  /*read text */
  _index->own = true;
  stat(filename,&sdata);
  file_size = sdata.st_size;

  f = fopen (filename,"r");
  if (f == NULL) return 24;
  _index->text= (uchar *) malloc (sizeof(uchar)*file_size);
  if (!_index->text) return 1;
  if (fread (_index->text,sizeof(uchar),file_size,f) != file_size) return 27;
  if (fclose(f) != 0) return 28;
  (*index) = _index;
  return 0;
}


int free_index(void *index) {
  TSA_Un *_index=(TSA_Un *) index;
  free(_index->pos);
  if (_index->own) free(_index->text);
  delete _index->bpe;
  free(_index);

  return 0;
}


int index_size(void *index, ulong *size) {
  TSA_Un *_index=(TSA_Un *) index;
  (*size) = sizeof(TSA_Un)+_index->bpe->SpaceRequirement()+_index->ns*sizeof(ulong);//(_index->ns*bits(_index->n+1))/8;
  return 0;
}


int index_size_count(void *index, ulong *size) {
  TSA_Un *_index=(TSA_Un *) index;
  (*size) = sizeof(TSA_Un)+_index->bpe->SpaceRequirement()+_index->ns*sizeof(ulong);//(_index->ns*bits(_index->n+1))/8;
  return 0;
}


int index_size_locate(void *index, ulong *size) {
  TSA_Un *_index=(TSA_Un *) index;
  (*size) = sizeof(TSA_Un)+_index->bpe->SpaceRequirement()+_index->ns*sizeof(ulong);//(_index->ns*bits(_index->n+1))/8;
  return 0;
}


/*////////////////////
//Querying the Index//
////////////////////*/
int count(void *index, uchar *pattern, ulong length, ulong *numocc) {
  TSA_Un *_index=(TSA_Un *) index;
  ulong LL,RR;
  /* Search for the left boundary */
  ulong L=0;
  ulong R=_index->ns;            // over the sample positions
  ulong i=(L+R)/2;
  ulong Lraja=0;
  ulong DER=_index->ns;
  int aux;
  if (ComparePos(_index,GetPos(_index,0),pattern,length) <= 0)
    Lraja=0;
  else {
    while (L<R-1) {
      aux=ComparePos(_index,GetPos(_index,i),pattern,length);
      if (aux > 0)
        L = i;
      else if (aux < 0) {
        DER = R = i; 
      } else 
        R = i;
      i = (R+L)/2;
    }
    Lraja=R;
  }
  while ((Lraja != 0) && (_index->pos[Lraja-1]==_index->pos[Lraja]))
    Lraja--;

  /* Search for the right boundary */

  if (Lraja == 0) L=0;
  else L=Lraja-1;
  R=DER;//_index->ns;
  i=(L+R)/2;
  ulong Rraja=0;
  if (ComparePos(_index,GetPos(_index,0),pattern,length) <= 0)
    Rraja=0;
  else {
    while (L < R-1) {
      if (ComparePos(_index,GetPos(_index,i),pattern,length) >= 0)
        L = i;
      else
        R = i;
      i = (R+L)/2;
    }
    Rraja=L;
  }

  while ((Rraja != _index->ns-1) && (_index->pos[Rraja+1]==_index->pos[Rraja]))
    Rraja++;

  /* Cross case */
  ulong *SA;
  ulong largo,RR_N;
  if ((Rraja-Lraja+1)==0) {
    if (Lraja == _index->ns-1) {LL = _index->n-1; RR_N=_index->n-1;}
    else {
      LL=_index->bpe->BR->prev(Lraja*_index->samplerate);
      RR_N=_index->bpe->BR->prev((Rraja+1)*_index->samplerate);
    }
    RR=_index->bpe->BR->prev(Rraja*_index->samplerate);

    SA=_index->bpe->dispair2(RR,RR_N-RR);
    //ulong *SA2 =_index->bpe->dispair2(RR,RR_N-RR);
    //for (i=0;i<=SA2[0];i++)
      //assert(SA[i]==SA2[i]);
    assert(RR_N-RR==SA[0]);
    largo=SA[0];
    SA[1]=_index->pos[Rraja];
    SA[largo+1]=_index->pos[Rraja+1];
    for (i=largo; i >=2;i--)
      SA[i]=SA[i+1]-SA[i]+_index->n;
    /* Search for the left boundary */
    if (Lraja == 0) LL=0;
    else {
      L=1;
      DER=R=largo+2;                 // over the sample positions
      i=(L+R)/2;
      Lraja=1;
      
      while (L<R-1) {
        aux = ComparePos(_index,SA[i],pattern,length);
        if (aux > 0)
          L = i;
        else if (aux < 0) {
          DER = R = i;
        } else
          R = i;
        i = (R+L)/2;
      }
      Lraja=R;
      LL=LL-largo+Lraja-1;
    }
    /* Search for the right boundary */
    if (Rraja == _index->ns-1) RR=_index->n-1;
    else {
      
      if (Lraja == 1) L=1;
      else L=Lraja-1;
      
      //L=1;
      R=DER;//largo+2;
      i=(L+R)/2;
      Rraja=1;
      while (L < R-1) {
        if (ComparePos(_index,SA[i],pattern,length) >= 0)
          L = i;
        else
          R = i;
        i = (R+L)/2;
      }
      Rraja=L;
      RR=RR+Rraja-1;
    }
    //printf("\n");
    //printf("Count: %lu de %lu hasta %lu\n",RR-LL+1, LL, RR);fflush(stdout);
    //if (LL==4864)
     //printf("caso1\n");

    (*numocc)=RR-LL+1;
    free(SA);
    return 0;
  }
  /* Normal case */
  ulong LL_P;
  /* Search for the left boundary */
  if (Lraja == 0) LL=0;
  else if (Lraja == _index->ns-1) LL=_index->n-1;
  else {
    LL=_index->bpe->BR->prev(Lraja*_index->samplerate);
    LL_P=_index->bpe->BR->prev((Lraja-1)*_index->samplerate);


    //LL=Lraja*_index->samplerate;
    //SA=_index->bpe->dispair((Lraja-1)*_index->samplerate+1,_index->samplerate-1);
    SA=_index->bpe->dispair2(LL_P,LL-LL_P);
    assert(LL-LL_P==SA[0]);
    largo=SA[0];
    SA[1]=_index->pos[Lraja-1];
    SA[largo+1]=_index->pos[Lraja];
    for (i=largo; i >=2;i--)
      SA[i]=SA[i+1]-SA[i]+_index->n;
    L=1;
    R=largo+2;                   // over the sample positions
    i=(L+R)/2;
    Lraja=1;
    while (L<R-1) {
      if (ComparePos(_index,SA[i],pattern,length) <= 0)
        R = i;
      else
        L = i;
      i = (R+L)/2;
    }
    Lraja=R;
    LL=LL-largo+Lraja-1;
    free(SA);
  }
  if (Rraja == _index->ns-1) RR=_index->n-1;
  else {

    if (Rraja+1 == _index->ns-1) RR_N=_index->n-1;
    else RR_N=_index->bpe->BR->prev((Rraja+1)*_index->samplerate);

    RR=_index->bpe->BR->prev(Rraja*_index->samplerate);
    
    SA=_index->bpe->dispair2(RR,RR_N-RR);
    
    assert(RR_N-RR==SA[0]);


    //RR=Rraja*_index->samplerate;
    //SA=_index->bpe->dispair(RR+1,_index->samplerate-1);
    largo=SA[0];
    SA[1]=_index->pos[Rraja];
    SA[largo+1]=_index->pos[Rraja+1];
    for (i=largo; i >=2;i--)
      SA[i]=SA[i+1]-SA[i]+_index->n;

    L=1;
    R=largo+2;
    i=(L+R)/2;
    Rraja=1;
    while (L < R-1) {
      if (ComparePos(_index,SA[i],pattern,length) >= 0)
        L = i;
      else
        R = i;
      i = (R+L)/2;
    }
    Rraja=L;
    RR=RR+Rraja-1;
    free(SA);
  }
  //printf("\n");
  //printf("Count: %lu de %lu hasta %lu\n",RR-LL+1, LL, RR);fflush(stdout);
    //if (LL==4864)
     //printf("caso2\n");

  (*numocc)=RR-LL+1;
  return 0;
}
/*
int locate_extract2(void *index){
ulong largo,izq,der,*occ,randl,i,aux,lar;
long te;
ulong random,number=10000;
TSA_Un *_index=(TSA_Un *) index; 
BitRankW32Int *BR=_index->bpe->BR;
ulong samplerate=_index->samplerate;
for (ulong hh=1; hh <= 1000000; hh*=10)
 for (lar=1;lar<=9;lar++) {
   largo=lar*hh;
   startclock3();
   for (i=0;i<number;i++) {
     occ=NULL;
     random = (ulong) (((float) rand()/ (float) RAND_MAX)*(_index->n-1));
     if (largo > samplerate) {
        aux=BR->prev(random);
        occ=_index->bpe->dispair2(aux, _index->bpe->BR->next(min(random+largo,_index->n-2))-aux);
     } else {
       izq=random-(random/samplerate)*samplerate;
       randl=(random+largo);
       der=(randl/samplerate+1)*samplerate-randl;
       if  (izq < der) {
         aux=BR->prev(random-izq);
         occ=_index->bpe->dispair2(aux, BR->next(min(random+largo,_index->n-2))-aux);
       } else {
         aux=BR->prev(random);
         occ=_index->bpe->dispair2(aux, BR->next(min(random+largo+der,_index->n-2))-aux);
       }
     }
     free(occ);
   }
   te = stopclock3();
   printf("Largo=%lu|e_t=%.4fs(%ldms)\n",largo, (te*1.0)/(HZ*1.0), te);fflush(stdout);

 }
 return 0;
}

int locate_extract3(void *index){
ulong largo,izq,*occ,i,aux,lar,largo2,largo3,posi,j,*respuesta,k;
long te;
ulong random,number=10000;
TSA_Un *_index=(TSA_Un *) index;
BitRankW32Int *BR=_index->bpe->BR;
ulong samplerate=_index->samplerate;
for (ulong hh=1; hh <= 1000000; hh*=10)
 for (lar=1;lar<=9;lar++) {
   largo=lar*hh;
   startclock3();
   for (i=0;i<number;i++) {
     occ=NULL;
     random = (ulong) (((float) rand()/ (float) RAND_MAX)*(_index->n-1));
     izq=(random/samplerate)*samplerate;
     aux=BR->prev(izq);
     
     largo2=BR->next(min(random+largo,_index->n-2))-aux;
     largo3=largo2+aux-random;
     occ=_index->bpe->dispair2(aux, largo2);
     //printf("%lu samplerate %lu\n",occ[0],samplerate);
     posi=random-aux+1;
     occ[1]=_index->pos[(random/samplerate)];
     for (j=2;j<occ[0];j++)
       occ[j]+=occ[j-1];
     respuesta=(ulong *) malloc (sizeof(ulong)*(largo3+1));
     respuesta[0]=largo3;
     k=1;
     for (j=posi;j<largo3+posi;j++){
        respuesta[k]=occ[j];k++;
     }
     free(occ);
     free(respuesta);
   }
   te = stopclock3();
   printf("Largo=%lu|e_t=%.4fs(%ldms)\n",largo, (te*1.0)/(HZ*1.0), te);fflush(stdout);

 }
 return 0;
}

int locate_extract(void *index){
ulong largo,*occ,i,lar;
long te;
ulong random,number=10000;
TSA_Un *_index=(TSA_Un *) index; 
for (ulong hh=1; hh <= 1000000; hh*=10)
 for (lar=1;lar<=9;lar++) {
   largo=lar*hh;
   startclock3();
   for (i=0;i<number;i++) {
     occ=NULL;
     random = (ulong) (((float) rand()/ (float) RAND_MAX)*(_index->n-1));
     occ=_index->bpe->dispair(random, largo+1);//min(random+largo,_index->n-2)-random+1);
     free(occ);
   }
   te = stopclock3();
   printf("Largo=%lu|e_t=%.4fs(%ldms)\n",largo, (te*1.0)/(HZ*1.0), te);fflush(stdout);

 }
 return 0;
}
*/

ulong *locate_extract(void *index, ulong LL, ulong RR){
ulong largo,izq,*occ,aux,largo2,j,*respuesta,k,temp;
TSA_Un *_index=(TSA_Un *) index;
BitRankW32Int *BR=_index->bpe->BR;
ulong samplerate=_index->samplerate;
    //printf("hooooooooooooolaaaaaaaaaaaaaaaaaa2\n");
   largo=RR-LL+1;
     //printf ("largo %lu, RR %lu, LL %lu ",largo, RR, LL);
     occ=NULL;
     izq=(LL/samplerate)*samplerate;
     aux=BR->prev(izq);
     //printf ("aux %lu , izq %lu ", aux, izq);
     largo2=BR->next(RR)-aux;
     occ=_index->bpe->dispair2(aux, largo2);

     //printf("\n desde aqui     ");
     respuesta=(ulong *) malloc (sizeof(ulong)*(largo));
     temp=_index->pos[(LL/samplerate)];
     //printf ("temp %lu ",temp);
     for (j=1;j<LL-aux;j++) {
        temp+=occ[j]-_index->n;
        //printf ("temp %lu %lu ",j, temp);
     }
    //printf("\n");
     
     //printf ("temp %lu temp ",temp);
     k=0;
     if (LL-aux == 0 ) {
       respuesta[k]=temp; k++;
       for (j=LL-aux+1;j<LL-aux+largo;j++){
         temp+=occ[j]-_index->n;
         respuesta[k]=temp;
         k++;
       }
     } else {
       for (j=LL-aux;j<LL-aux+largo;j++){
         temp+=occ[j]-_index->n;
         respuesta[k]=temp;
         k++;
       }
     }
     free(occ);
    //for (ulong i=0;i<largo;i++) 
      //printf ("%lu ",respuesta[i]);
    //printf("hooooooooooooolaaaaaaaaaaaaaaaaaa5\n\n\n\n");
 return respuesta;
}

int locate(void *index, uchar *pattern, ulong length, ulong **occ, ulong *numocc) {
  //locate_extract3(index);
  //exit(0);
  TSA_Un *_index=(TSA_Un *) index; *occ=NULL;
/*
  printf("aqui parte\n");
  for (int i=23119; i < 23119+10; i++)
    printf("%d ", _index->text[i]);
  printf("\naqui termina\n");
  printf("aqui parte\n");
  for (int i=488; i < 488+10; i++)
    printf("%d ", _index->text[i]);
  printf("\naqui termina\n");
  exit(0);
*/
  ulong LL,RR, matches;
  /* Search for the left boundary */
  ulong L=0;
  ulong R=_index->ns;            // over the sample positions
  ulong i=(L+R)/2;
  ulong Lraja=0;
  ulong DER=_index->ns;
  int aux;
  if (ComparePos(_index,GetPos(_index,0),pattern,length) <= 0)
    Lraja=0;
  else {
    while (L<R-1) {
      aux=ComparePos(_index,GetPos(_index,i),pattern,length);
      if (aux > 0)
        L = i;
      else if (aux < 0) {
        DER = R = i; 
      } else  
        R = i;
      i = (R+L)/2;
    }
    Lraja=R;
  }
  while ((Lraja != 0) && (_index->pos[Lraja-1]==_index->pos[Lraja]))
    Lraja--;

  /* Search for the right boundary */

  if (Lraja == 0) L=0;
  else L=Lraja-1;
  R=DER;//_index->ns;
  i=(L+R)/2;
  ulong Rraja=0;
  if (ComparePos(_index,GetPos(_index,0),pattern,length) <= 0)
    Rraja=0;
  else {
    while (L < R-1) {
      if (ComparePos(_index,GetPos(_index,i),pattern,length) >= 0)
        L = i;
      else
        R = i;
      i = (R+L)/2;
    }
    Rraja=L;
  }

  while ((Rraja != _index->ns-1) && (_index->pos[Rraja+1]==_index->pos[Rraja]))
    Rraja++;

  /* Cross case */
  ulong *SA;
  ulong largo,RR_N;
  if ((Rraja-Lraja+1)==0) {
    if (Lraja == _index->ns-1) {LL = _index->n-1; RR_N=_index->n-1;}
    else {
      LL=_index->bpe->BR->prev(Lraja*_index->samplerate);
      RR_N=_index->bpe->BR->prev((Rraja+1)*_index->samplerate);
    }
    RR=_index->bpe->BR->prev(Rraja*_index->samplerate);

    SA=_index->bpe->dispair2(RR,RR_N-RR);



    assert(RR_N-RR==SA[0]);
    largo=SA[0];
    SA[1]=_index->pos[Rraja];
    SA[largo+1]=_index->pos[Rraja+1];
    for (i=largo; i >=2;i--)
      SA[i]=SA[i+1]-SA[i]+_index->n;
    /* Search for the left boundary */
    if (Lraja == 0) LL=0;
    else {
      L=1;
      DER=R=largo+2;                 // over the sample positions
      i=(L+R)/2;
      Lraja=1;

      while (L<R-1) {
        aux = ComparePos(_index,SA[i],pattern,length);
        if (aux > 0)
          L = i;
        else if (aux < 0) {
          DER = R = i;
        } else
          R = i;
        i = (R+L)/2;
      }
      Lraja=R;
      LL=LL-largo+Lraja-1;
    }
    /* Search for the right boundary */
    if (Rraja == _index->ns-1) RR=_index->n-1;
    else {
      if (Lraja == 1) L=1;
      else L=Lraja-1;
      //L=1;
      R=DER;//_index->ns;

      //L=0;
      //R=largo+2;
      i=(L+R)/2;
      Rraja=1;
      while (L < R-1) {
        if (ComparePos(_index,SA[i],pattern,length) >= 0)
          L = i;
        else
          R = i;
        i = (R+L)/2;
      }
      Rraja=L;
      RR=RR+Rraja-1;
    }
    //printf("\n");
    //printf("Count: %lu de %lu hasta %lu\n",RR-LL+1, LL, RR);fflush(stdout);
    //if (LL==102369)
    //printf("caso1\n");

    (*numocc)=RR-LL+1;
    matches = RR-LL+1;
    if (matches >0) {
      ulong locate=0;
      (*occ) = (ulong *) malloc(matches*sizeof(ulong));
      if (!(*occ)) return 1;
      for (; Lraja<=Rraja; Lraja++) {
        (*occ)[locate]=SA[Lraja];
        locate++;
      }
    } else
      *occ = NULL;
    free(SA);
    //for (i=0;i<(*numocc);i++) 
      //printf ("%lu ",(*occ)[i]);
    //printf(" tipo1\n");
    //locate_extract(index, LL, RR);
    //printf(" tipo1\n");
    return 0;
  }

  /* Normal case */

  ulong *SA1=NULL, *SA2=NULL,L1,R1,LL_P, largol=0;
  /* Search for the left boundary */
  if (Lraja == 0) {LL=0;L1=0;}
  else if (Lraja == _index->ns-1) {LL=_index->n-1; L1=Lraja;}
  else {
    LL=_index->bpe->BR->prev(Lraja*_index->samplerate);
    LL_P=_index->bpe->BR->prev((Lraja-1)*_index->samplerate);
    L1=Lraja;
  
    SA1=_index->bpe->dispair2(LL_P,LL-LL_P);
    largol=SA1[0];
    SA1[1]=_index->pos[Lraja-1];
    SA1[largol+1]=_index->pos[Lraja];
    for (i=largol; i >=2;i--)
      SA1[i]=SA1[i+1]-SA1[i]+_index->n;
    L=1;
    R=largol+2;                   // over the sample positions
    i=(L+R)/2;
    Lraja=1;
    while (L<R-1) {
      if (ComparePos(_index,SA1[i],pattern,length) <= 0)
        R = i;
      else
        L = i;
      i = (R+L)/2;
    }
    Lraja=R;
    LL=LL-largol+Lraja-1;
  }
  if (Rraja == _index->ns-1) {RR=_index->n-1; R1=Rraja; Rraja=0;}
  else {

    if (Rraja+1 == _index->ns-1) RR_N=_index->n-1;
    else RR_N=_index->bpe->BR->prev((Rraja+1)*_index->samplerate);

    RR=_index->bpe->BR->prev(Rraja*_index->samplerate);

    SA2=_index->bpe->dispair2(RR,RR_N-RR);

    R1=Rraja;
    largo=SA2[0];
    SA2[1]=_index->pos[Rraja];
    SA2[largo+1]=_index->pos[Rraja+1];
    for (i=largo; i >=2;i--)
      SA2[i]=SA2[i+1]-SA2[i]+_index->n;

    L=1;
    R=largo+2;
    i=(L+R)/2;
    Rraja=1;
    while (L < R-1) {
      if (ComparePos(_index,SA2[i],pattern,length) >= 0)
        L = i;
      else
        R = i;
      i = (R+L)/2;
    }
    Rraja=L;
    RR=RR+Rraja-1;
  }
  //printf("\n");
  //printf("Count: %lu de %lu hasta %lu\n",RR-LL+1, LL, RR);fflush(stdout);
  (*numocc)=RR-LL+1;
  matches = RR-LL+1;
  (*occ) = (ulong *) malloc(matches*sizeof(ulong));
  if (!(*occ)) return 1;

  if ((SA1 == NULL) && (SA2 == NULL)) {
    (*occ)[0]=_index->pos[_index->ns-1];
    return 0;
  }
  
  ulong locate=0;
  if (SA1 != NULL) 
    for (; Lraja<=largol; Lraja++) {
      (*occ)[locate]=SA1[Lraja];
      locate++;
    }
  locate=matches-1;
  for (; 0 < Rraja; Rraja--) {
    (*occ)[locate]=SA2[Rraja];
    locate--;
  }


  if (SA1 !=NULL) free(SA1);
  if (SA2 !=NULL) free(SA2);
  if ((_index->pos[R1]-_index->pos[L1]) == 0) {
    //for (i=0;i<(*numocc);i++) 
      //printf ("%lu ",(*occ)[i]);
    //printf(" tipo2\n");
    //locate_extract(index, LL, RR);
   //printf(" tipo2\n");
    return 0;
  }

  SA=_index->bpe->dispair2(_index->bpe->BR->prev(L1*_index->samplerate),_index->bpe->BR->prev(R1*_index->samplerate)-_index->bpe->BR->prev(L1*_index->samplerate));
  //SA=_index->bpe->dispair(L1*_index->samplerate+1,_index->samplerate*(R1-L1)-1);
  largo=SA[0];
  SA[0]=_index->pos[L1];
  SA[largo+1]=_index->pos[R1];
  for (i=largo; i >=1;i--) {
    (*occ)[locate]=(*occ)[locate+1]-SA[i]+_index->n; 
    locate--;
  }

  free(SA);
    //for (i=0;i<(*numocc);i++) 
      //printf ("%lu ",(*occ)[i]);
   //printf(" tipo3\n");
    //locate_extract(index, LL, RR);
   //printf(" tipo3\n");
  return 0;
}




/*///////////////////////
//Accessing the indexed//
///////////////////////*/
int display(void *index, uchar *pattern, ulong length, ulong numc, ulong *numocc, uchar **snippet_text, ulong **snippet_lengths) {
  ulong *occ, i, j, from, to, len, x;
  ulong n = ((TSA_Un *) index)->n;
  uchar *text = ((TSA_Un *) index)->text;
  uchar *text_aux;
  int aux = locate(index, pattern, length, &occ, numocc);
  if (aux != 0) return aux;
  *snippet_lengths = (ulong *) malloc((*numocc)*sizeof(ulong));
  if (!(*snippet_lengths)) return 1;
  *snippet_text = (uchar *) malloc((*numocc)*(length+2*numc)*sizeof(uchar));
  if (!(*snippet_text)) return 1;
  text_aux=*snippet_text;
  for (i=0;i<(*numocc);i++) {
    x=occ[i];
    if (x>numc) from = x-numc;
    else from=0;
    to= ((x+length+numc-1)<(n-1)?(x+length+numc-1):(n-1));
    len =to-from+1;
    for (j=0; j<len;j++)
      text_aux[j] = text[from+j];
    text_aux+=length+2*numc;
    (*snippet_lengths)[i] = len;
  }
  free(occ);
  return 0;
}


int extract(void *index, ulong from, ulong to, uchar **snippet, ulong *snippet_length) {
  ulong n = ((TSA_Un *) index)->n;
  uchar *text = ((TSA_Un *) index)->text;
  if (to >= n) to=n-1;
  if (from > to) {
    *snippet = NULL;
    *snippet_length=0;
  }
  else {
    ulong j;
    ulong len =to-from+1;
    *snippet = (uchar *) malloc((len)*sizeof(uchar));
    if (!(*snippet)) return 1;
    for (j=from; j<=to;j++) {
      (*snippet)[j-from]=text[j];
    }
    (*snippet_length)=len;
  }
  return 0;
}


int get_length(void *index, ulong *length) {
  (*length)=((TSA_Un *) index)->n;
  return 0;
}


/*////////////////
//Error handling//
////////////////*/
char *error_index(int e) {
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
