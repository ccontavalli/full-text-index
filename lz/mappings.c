
#include "mappings.h"

inline uint NODE(lztrie T, revtrie RT, uint id)
 {
    uint nbits = T->nbits;
    if (!id) return 0;
    return select_0(T->pdata->bdata,bitget(RT->R,bitget(RT->rids_1,id*nbits,nbits)*nbits,nbits)+1);
 }

inline uint RNODE(lztrie T, revtrie RT, uint id)
 {
    uint nbits = T->nbits;
    if (!id) return 0;
    return getnodeRevTrie(RT,bitget(RT->rids_1,id*nbits,nbits));
 }

inline uint IDS(lztrie T, uint pos)
 {
    return rthLZTrie(T, pos);
 }

inline uint RIDS(revtrie RT, uint rpos)
 {
    return rthRevTrie(RT, rpos);
 }
