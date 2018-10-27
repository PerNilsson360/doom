#include "z_zone.h"
#include "w_wad.h"
#include "m_swap.h"

#include "BlockMap.hh"

void 
BlockMap::load(int lump_num)
{
    int		i;
    int		count;
	
    lump = (short int*)W_CacheLumpNum (lump_num,PU_LEVEL);
    blockmap = lump+4;
    count = W_LumpLength (lump_num)/2;

    for (i=0 ; i<count ; i++)
        lump[i] = SHORT(lump[i]);
    
    oorgx = lump[0];
    oorgy = lump[1];
    width = lump[2];
    height = lump[3];
    
    // clear out mobj chains
    count = sizeof(*blocklinks)* width*height;
    blocklinks = (Mob**)Z_Malloc (count,PU_LEVEL, 0);
    memset (blocklinks, 0, count);
}

