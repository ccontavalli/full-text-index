
#include "lztrie.h"
#include "revtrie.h"
#include "nodemap.h"


typedef struct tpos
 {
    uint *SuperBlock;  // array with starting positions of superblocks 
    uint nSuperBlock;  // number of superblocks
    uint nbitsSB;      // number of bits used for starting positions
    uint SBlock_size;  // superblock size
    uint *Offset;      // array of offsets of each phrase within each superblock
    uint nOffset;      // size of the Offset array
    uint nbitsOffs;    // number of bits used per offset
    ulong Tlength;     // text length 
 } *position;
 
 
position createPosition(lztrie T, revtrie RT, uint text_length);

inline ulong getPosition(position P, uint id);

ulong sizeofPosition(position P);

void destroyPosition(position P);

