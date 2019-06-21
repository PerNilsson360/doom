#ifndef _BLOCK_MAP_HH_
#define _BLOCK_MAP_HH_

#include "Mob.hh"
#include "BoundingBox.hh"

class BlockMap 
{
public:
    void load(int lump);
    Vertex getLineDistance(const Vertex& v);
    BoundingBox getBox(const Vertex& position, double extent) const;
    // Returns a box in map block coordinates
    BoundingBox getBox(const BoundingBox& box) const; 
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
