 
// Search module

#ifdef QUERYREPORT

#include <sys/time.h>


struct tms time1,time2;
double fwdtime;
double bwdtime;
double type1time;
double type2time;
double type3time;
double ticks;

#endif


static uint Count, SIZE_OCCARRAY;
static ulong *OccArray, *OffsetArray;
position TPos;


uint nbits_aux;


inline uint bitget_aux(uint *e, uint p)
 {
    register uint i=(p>>5), j=p&0x1F, answ;
    
    if (j+nbits_aux <= W)
       answ = (e[i] << (W-j-nbits_aux)) >> (W-nbits_aux);
    else 
       answ = (e[i] >> j) | ((e[i+1]<<(W-j-nbits_aux)) >> (W-nbits_aux));
    return answ;
 }




uint p_aux3, nbits_aux3, *e_aux3;

inline uint bitget_aux3()
 {
    register uint i=(p_aux3>>5), j=p_aux3&0x1F, answ;

    if (j+nbits_aux3 <= W)
        answ = (e_aux3[i] << (W-j-nbits_aux3)) >> (W-nbits_aux3);
    else 
       answ = (e_aux3[i] >> j) | ((e_aux3[i+1]<<(W-j-nbits_aux3))>>(W-nbits_aux3));
    return answ;
 }
 

uint *e_aux4, p_aux4, nbits_aux4;

inline uint bitget_aux4()
 {
    register uint i=(p_aux4>>5), j=p_aux4&0x1F, answ;

    if (j+nbits_aux4 <= W)
       answ = (e_aux4[i] << (W-j-nbits_aux4)) >> (W-nbits_aux4);
    else 
       answ = (e_aux4[i] >> j) | ((e_aux4[i+1]<<(W-j-nbits_aux4))>> (W-nbits_aux4));
    return answ;
 }


#define INIT_OCCARRAY 10000
#define ALPHA 0.7


inline void report(uint id, uint delta)
 { 
    OccArray[Count]      = id;
    OffsetArray[Count++] = delta;
    if (Count == SIZE_OCCARRAY) {
       SIZE_OCCARRAY = (SIZE_OCCARRAY+1)/ALPHA;
       OccArray       = realloc(OccArray, SIZE_OCCARRAY*sizeof(ulong));
       OffsetArray    = realloc(OffsetArray, SIZE_OCCARRAY*sizeof(ulong));
    }   
 }

        // returns answ[i,j] = lztrie node corresponding to pat[i..j], or
        //    NULLT if it does not exist. answ is really used as answ[i*m+j]
        // time is O(m^2s) in the worst case, but probably closer to O(m^2)

static trieNode *fwdmatrix(lztrie T, byte *pat, uint m, uint **fwdid)
 { 
    trieNode *answ, cur, new;
    uint i,j,ptr, *id;
    bitmap Rdir = T->pdata->bdata;
    uint nbits, *ids;
    nbits = T->nbits;
    ids = T->ids;
    nbits_aux = nbits;
    nbits_aux3 = nbits;
    e_aux3 = ids;
    answ = malloc(m*m*sizeof(trieNode));
    id   = malloc(m*m*sizeof(uint));
    ptr  = 0; // ptr = i*m+j
    for (i=0; i<m; i++) {
       cur = ROOT; ptr += i;
       for (j=i; j<m; j++) {
          new = childLZTrie(T,cur,pat[j]);
          if (new != NULLT) {
             uint pos = new - rank(Rdir,new);
             answ[ptr] = new;
             p_aux3 = pos*nbits; 
             id [ptr]  = bitget_aux3();
             cur       = new;
             ptr++;
          }
          else // no children, so no more entries in answ
             for (;j<m;j++) {
                answ[ptr] = NULLT; 
                id[ptr]   = ~0; 
                ptr++;
             }
       }
    }
    *fwdid = id;
    return answ;
 }

        // returns answ[j] = revtrie node corresponding to pat[0..j]^R, or
        //    NULLT if it does not exist.
        //    note that it needs the lztrie LZT, the answers computed by 
        //    fwdmatrix (fwd) and the corresponding block ids (fedid)
        // time is O(m^3s) in the worst case, but probably closer to O(m^2)

static trieNode *bwdmatrix(revtrie T, byte *pat, uint m, 
                           uint *fwdid, lztrie LZT)
 { 
    int i,j,k;
    trieNode cur,cur2,lzcur,lzcur2,*answ;
    byte c1,c2, *letters;
    bitmap Rdir;
    uint *boost;
    answ = malloc(m*sizeof(trieNode));
    letters = LZT->letters;
    Rdir = LZT->pdata->bdata;
    boost = LZT->boost;
    // answ[j] = node(pat[0..j]);
    for (j=0; j<m; j++) { 
       for (i=0;i<=j;i++) if (fwdid[m*i+j] != ~0) break;
       if (i <= j) {// i is last nonempty node in the path
          cur = RNODE(LZT,T,fwdid[m*i+j]);
       }
       else cur = ROOT;
       i--; // unresolved 0..i
       while (i >= 0) {// once per (empty) node traversed
          cur = childRevTrie(T,cur,j-i,pat[i],&lzcur,&lzcur2);
          if (cur == NULLT) break; // no further prefixes exist
          while (--i >= 0) {// once per letter
             if (lzcur == ROOT) lzcur = NULLT; // parent of root
             else if (boost[letters[lzcur-rank(Rdir,lzcur)]] == lzcur) lzcur = ROOT;
             else lzcur = enclose(LZT->pdata,lzcur); 
             if (lzcur == ROOT) break; // arrived at cur
             c1 = letters[lzcur-rank(Rdir,lzcur)];
	     if (lzcur2 != NULLT) {
                if (lzcur2 == ROOT) lzcur2 = NULLT; // parent of root
                else if (boost[letters[lzcur2-rank(Rdir,lzcur2)]] == lzcur2) lzcur2 = ROOT;
                else lzcur2 = enclose(LZT->pdata,lzcur2);
                c2     = letters[lzcur2-rank(Rdir,lzcur2)];
                if (c1 != c2) break; // end of common path
             }
             if (c1 != pat[i]) // no further prefixes exist
             { cur = NULLT; i = -1; break; }
          }
       }
       answ[j] = cur;
    }
    return answ;
 }

  
        // reports occurrences of type 1
        // time is O(occ1),

static void reportType1(lztrie fwdtrie, revtrie bwdtrie,
                        trieNode *fwd, trieNode *bwd, uint *fwdid, uint m, bool counting)
 { 
    uint from,to,id,siz,pos,delta,lztpos,nbits;
    trieNode lzcur,new;
    uint *data, *fdata, *R, rvtpos;
    bitmap fwd_bdata;
    
    if (bwd[m-1] == NULLT) return; // there isn't any LZ78 phrase ending with P
    from      = leftrankRevTrie(bwdtrie,bwd[m-1]);
    to        = rightrankRevTrie(bwdtrie,bwd[m-1]);
    nbits     = fwdtrie->nbits;
    data      = bwdtrie->B->data;
    R         = fwdtrie->R;
    fwd_bdata = fwdtrie->pdata->bdata;
    e_aux3  = fwdtrie->ids;
    nbits_aux3 = nbits;
    
    rvtpos = getposRevTrie(bwdtrie, from);
    while (from <= to) {
       if (bitget1(data, from)) {
          lztpos = bitget_aux(R,rvtpos*nbits);
          rvtpos++;
          lzcur  = select_0(fwd_bdata, lztpos+1);
          siz    = subtreesizeLZTrie(fwdtrie,lzcur);
          if (counting) Count += siz; // just count, faster
          else { 
             p_aux3 = lztpos*nbits;
             id     = bitget_aux3();
             delta = m;
             while (true) {
                report(id,delta);
                if (!--siz) break;
                p_aux3 += nbits;
                id    = bitget_aux3();
                lzcur = nextLZTrie(fwdtrie,lzcur,&delta);
             }
          }
       }
       from++;
    }
 }

        // reports occurrences of type 2
        // time is O(min range among the 2 one-dimensional ones)

static void reportType2(lztrie fwdtrie, revtrie bwdtrie,
                        trieNode *fwd, trieNode *bwd, uint m, bool counting)
 { 
    uint i,ffrom,fto,bfrom,bto;
    uint id,nbits;
    trieNode f, b;
    trieNode cur,open,close;
    uint *data, *rids_1, *R, *ids, rvtpos;
 
    nbits = fwdtrie->nbits;
    data = bwdtrie->B->data;
    rids_1 = bwdtrie->rids_1;
    ids = fwdtrie->ids;
    R = bwdtrie->R;
    for (i=1; i<m; i++) {
       f = fwd[i*m+m-1];
       b = bwd[i-1];
       if ((f != NULLT) && (b != NULLT)) {
          ffrom = leftrankLZTrie(fwdtrie,f);
          fto   = rightrankLZTrie(fwdtrie,f);
          bfrom = leftrankRevTrie(bwdtrie,b);
          bto   = rightrankRevTrie(bwdtrie,b);
          if ((fto-ffrom) <= 4*(bto-bfrom)) {
             open = getposRevTrie(bwdtrie,bfrom);
             close = getposRevTrie(bwdtrie, bto);
             e_aux3 = rids_1;
             nbits_aux3 = nbits;
             while (ffrom <= fto) {
                id  = bitget_aux(ids, ffrom*nbits);
                ffrom++;
                p_aux3 = (id-1)*nbits;
                cur = bitget_aux3();
                if ((open <= cur) && (cur <= close))  
                   if (counting) Count++;
		   else report(id-1,i);
             }
          }
          else {
             open  = ffrom;
             close = fto;
             e_aux3 = ids;
             nbits_aux3 = nbits;
             rvtpos = getposRevTrie(bwdtrie,bfrom);
             while (bfrom <= bto) {
                if (bitget1(data,bfrom)) {
                   p_aux3 = bitget_aux(R,rvtpos*nbits)*nbits;
                   rvtpos++;
                   id = bitget_aux3();
                   cur = bitget_aux(R,bitget_aux(rids_1,(id+1)*nbits)*nbits);
                   if ((open <= cur) && (cur <= close)) 
                      if (counting) Count++;
                      else report(id,i);
                }
                bfrom++;
             }
          }
       }
    }
 }

        // reports occurrences of type 3
        // time is O(m^3s) worst case, but probably closer to O(m^2)

        // hashing

typedef struct 
 { 
    uint k;
    short i,j;
 } helem;

static helem *hcreate(uint m, uint *size)
 { 
    helem *table;
    int i;
    *size = 1 << bits(m*m); // so factor is at least 2.0
    table = malloc (*size*sizeof(helem));
    (*size)--;
    for (i=*size;i>=0;i--) table[i].k = ~0;
    return table;
 }

static void hinsert(helem *table, uint size, uint k, uint i, uint j)
 { 
    uint key = (k*i) & size;
    while (table[key].k != ~0) key = (key + PRIME2) & size;
    table[key].k = k; table[key].i = i; table[key].j = j;
 }

static int hsearch(helem *table, uint size, uint k, uint i)
 { 
    uint key = (k*i) & size;
    uint sk;
    while ((sk = table[key].k) != ~0) {
       if ((sk == k) && (table[key].i == i)) return table[key].j;
       key = (key + PRIME2) & size;
    }
    return -1;
 }

static void reportType3(lztrie fwdtrie, revtrie bwdtrie,
                        trieNode *fwd, uint *fwdid, trieNode *bwd, 
                        byte *pat, uint m, bool counting)
 {
    uint i,j,k,f,t,nt,ok;
    trieNode node;
    helem *table;
    uint tsize;
    // find and store all blocks contained in pat
    table = hcreate(m,&tsize);
    for (i=1; i<m; i++)
       for (j=i; j<m; j++)
          if (fwdid[i*m+j] != ~0)
             hinsert(table,tsize,fwdid[i*m+j],i,j);
    // now find maximal segments
    for (i=0;i<m;i++)
       for (j=i; j<m; j++)
          if ((k = fwdid[i*m+j]) != ~0) {
             f = i; t = j; ok = k;
             while ((nt = hsearch(table,tsize,k+1,t+1)) != -1) {
                fwdid[(t+1)*m+nt] = ~0; t = nt; k++; 
             }
             // now we know that pat[f,t] is a maximal sequence
             if ((k-ok+1) + (f > 0) + (t < m-1) < 3) continue;
             // ok, it spans 3 blocks at least
             if (t < m-1) {// see if block k+1 descends from pat[t+1..m-1]
                if (k+1 == fwdtrie->n) continue; // this was the last block
                node = fwd[(t+1)*m+m-1];
                if (node == NULLT) continue; // rest does not exist 
                if (!ancestorLZTrie(fwdtrie,node,NODE(fwdtrie,bwdtrie,k+1))) continue;
             }
             // ok, test on block k+1 passed
             if (f == 0) { // left test not needed
                if (counting) Count++;
                else report(ok,j-i+1);
             }
	     else {
                node = bwd[f-1];
                if (node == NULLT) continue; // rest does not exist 
                if (!ancestorRevTrie(bwdtrie,node,RNODE(fwdtrie,bwdtrie,ok-1))) 
                   continue;
                if (counting) Count++;
                else report(ok-1,f);
             }
          }
    free(table);
 }


int count(void *index, byte *pattern, ulong length, ulong* numocc)
 {
    lzindex I = *(lzindex *)index;
    trieNode *fwd,*bwd;
    uint *fwdid;
    
    Count       = 0;    

#ifdef QUERYREPORT
    ticks= (double)sysconf(_SC_CLK_TCK);
    times(&time1);
#endif

    fwd = fwdmatrix(I.fwdtrie,pattern,length,&fwdid);
    
#ifdef QUERYREPORT
    times(&time2);
    fwdtime += (time2.tms_utime - time1.tms_utime)/ticks;
    times(&time1);
#endif    

    bwd = bwdmatrix(I.bwdtrie,pattern,length,fwdid,I.fwdtrie);
    
#ifdef QUERYREPORT
    times(&time2);
    bwdtime += (time2.tms_utime - time1.tms_utime)/ticks;
    times(&time1);
#endif    

    reportType1(I.fwdtrie,I.bwdtrie,fwd,bwd,fwdid,length,true);
    
#ifdef QUERYREPORT
    times(&time2);
    type1time += (time2.tms_utime - time1.tms_utime)/ticks;
    times(&time1);
#endif    

    if (length > 1) {
       reportType2(I.fwdtrie,I.bwdtrie,fwd,bwd,length,true);

#ifdef QUERYREPORT
       times(&time2);
       type2time += (time2.tms_utime - time1.tms_utime)/ticks;
       times(&time1);
#endif               
    }

    if (length > 2) {
       reportType3(I.fwdtrie,I.bwdtrie,fwd,fwdid,bwd,pattern,length,true);

#ifdef QUERYREPORT
       times(&time2);
       type3time += (time2.tms_utime - time1.tms_utime)/ticks;
       times(&time1);
#endif        
    }
   
    free(fwd); 
    free(bwd); 
    free(fwdid);
    *numocc     = Count;
    return 0; // no errors yet
 }
 

int locate(void *index, byte *pattern, ulong length, 
           ulong **occ, ulong *numocc)
 {
    lzindex I = *(lzindex *)index;
    trieNode *fwd, *bwd;
    uint *fwdid, i;
    ulong *occaux, id;
    uint posSB, *SuperBlock, *Offset, SBlock_size, nOffset, Tlength,
         nbits_Offs, nbits_SB;
    
     
#ifdef QUERYREPORT
    ticks= (double)sysconf(_SC_CLK_TCK);
#endif

    TPos          = I.TPos;
    SuperBlock    = TPos->SuperBlock;
    Offset        = TPos->Offset;
    nbits_SB      = TPos->nbitsSB;
    nbits_Offs    = TPos->nbitsOffs;
    SBlock_size   = TPos->SBlock_size;
    Tlength       = TPos->Tlength;
    nOffset       = TPos->nOffset;
    
    Count         = 0;
    SIZE_OCCARRAY = INIT_OCCARRAY;
    OccArray      = malloc(sizeof(ulong)*SIZE_OCCARRAY);
    OffsetArray   = malloc(sizeof(ulong)*SIZE_OCCARRAY);

#ifdef QUERYREPORT
    times(&time1);
#endif    

    fwd = fwdmatrix(I.fwdtrie,pattern,length,&fwdid);

#ifdef QUERYREPORT
    times(&time2);
    fwdtime += (time2.tms_utime - time1.tms_utime)/ticks;
    times(&time1);
#endif    

    bwd = bwdmatrix(I.bwdtrie,pattern,length,fwdid,I.fwdtrie);

#ifdef QUERYREPORT
    times(&time2);
    bwdtime += (time2.tms_utime - time1.tms_utime)/ticks;
    times(&time1);
#endif        

    reportType1(I.fwdtrie,I.bwdtrie,fwd,bwd,fwdid,length,false);

#ifdef QUERYREPORT
    times(&time2);
    type1time += (time2.tms_utime - time1.tms_utime)/ticks;
    times(&time1);
#endif        

    if (length > 1) {
       reportType2(I.fwdtrie,I.bwdtrie,fwd,bwd,length,false);

#ifdef QUERYREPORT
       times(&time2);
       type2time += (time2.tms_utime - time1.tms_utime)/ticks;
       times(&time1);
#endif    
    }

    if (length > 2) {
        reportType3(I.fwdtrie,I.bwdtrie,fwd,fwdid,bwd,pattern,length,false);

#ifdef QUERYREPORT
       times(&time2);
       type3time += (time2.tms_utime - time1.tms_utime)/ticks;
       times(&time1);
#endif            
    }

    free(fwd); 
    free(bwd); 
    free(fwdid);
    *numocc = Count;
    *occ    = malloc(sizeof(ulong)*(*numocc));
    occaux = *occ;
    
    nbits_aux3 = nbits_SB;
    e_aux3     = SuperBlock;
    nbits_aux4 = nbits_Offs;
    e_aux4 = Offset;
    
    for (i=Count-1; i; i--) { 
       id = OccArray[i];  
       if ((id+1) > nOffset) occaux[i] = Tlength - OffsetArray[i];
       else {
          p_aux3 = (id>>5)*nbits_SB;
          p_aux4 = id*nbits_Offs;
          occaux[i] = (bitget_aux3() + bitget_aux4()) - OffsetArray[i];
       }
    }
    id = OccArray[0];
    if ((id+1) > nOffset) occaux[0] = Tlength - OffsetArray[0];
    else {
       p_aux3 = (id>>5)*nbits_SB;
       p_aux4 = id*nbits_Offs;
       occaux[0] = (bitget_aux3() + bitget_aux4()) - OffsetArray[0];
    }   
    free(OccArray);
    free(OffsetArray);
    return 0; // no errors yet
 } 
 
 


void extract_text(void *index, ulong from, ulong to, byte *snippet, 
                  ulong *snippet_length)
 {
    ulong snpp_pos, posaux, idaux;
    lzindex I = *(lzindex *)index;
    uint idfrom,idto;
    trieNode p;
    
    idfrom = getphrasePosition(TPos, from);
    idto   = getphrasePosition(TPos, to);
    *snippet_length = to-from+1;
    
    posaux = getPosition(TPos, idto+1)-1;
    p = NODE(I.fwdtrie, I.bwdtrie, idto);//mapto(I.map, idto);
    while (p&&(posaux > to)) {
       p = parentLZTrie(I.fwdtrie, p);
       posaux--;
    }
    snpp_pos = (*snippet_length)-1;
    for (idaux = idto; idaux != idfrom;) {
       while (p /*&& (posaux <= to)*/) {
          snippet[snpp_pos--] = letterLZTrie(I.fwdtrie, p);
          p = parentLZTrie(I.fwdtrie, p);   
       }
       p = NODE(I.fwdtrie, I.bwdtrie, --idaux);//mapto(I.map, --idaux);
    }
    if (idfrom != idto) posaux = getPosition(TPos, idfrom+1)-(from!=0);
    while (p && (posaux >= from)) {
       snippet[snpp_pos--] = letterLZTrie(I.fwdtrie, p);
       p = parentLZTrie(I.fwdtrie, p);
       //if (p==ROOT) p = mapto(I.map, --idaux);
       posaux--;
    }
 }



int extract(void *index, ulong from, ulong to, byte **snippet, 
            ulong *snippet_length)
 {
    ulong text_length;
    lzindex I = *(lzindex *)index;
    
    get_length(index, &text_length);
    if (to > (text_length-1)) to = text_length-1;
    if (from > (text_length-1)) from = text_length-1;

    *snippet_length = to-from+1;
    *snippet = malloc(*snippet_length);
    TPos     = I.TPos;
    extract_text(&I,from,to,*snippet,snippet_length);
    return 0; // no errors yet
 }
 
 
   
int display(void *index, byte *pattern, ulong plength, ulong numc,
            ulong *numocc, byte **snippet_text, ulong **snippet_lengths)
 {
    ulong *occ, i, k, from, to, text_length;
    lzindex I = *(lzindex *)index;
    
    get_length(index, &text_length);
    locate(&I,pattern,plength,(ulong **)&occ,numocc);
    k = *numocc;
    (*snippet_text)    = malloc(sizeof(byte)*k*(plength+2*numc));
    for (i = 0; i < k; i++) {
       from = occ[i] - numc;
       to   = occ[i] + plength -1 + numc;
       if (numc >= occ[i]) from = 0;
       if (to > (text_length-1)) to = text_length-1;
       if (from > (text_length-1)) from = text_length-1;
       extract_text(&I,from,to,&((*snippet_text)[i*(plength+2*numc)]),&(occ[i]));
    }
    *snippet_lengths = occ;
    return 0; // no errors yet
 }
