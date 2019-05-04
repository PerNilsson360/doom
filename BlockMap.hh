#ifndef _BLOCK_MAP_HH_
#define _BLOCK_MAP_HH_

#include "Mob.hh"

class BlockMap 
{
public:
    void load(int lump);

//private:

    int width;
    int height;	// size in mapblocks
    short*  blockmap;	// int for larger maps
// offsets in blockmap are from here
    short*	lump;		
// origin of block map
    double oorgx;
    double oorgy;
// for thing chains
    Mob**  blocklinks;		
};

#endif
