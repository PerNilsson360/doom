#include "z_zone.h"
#include "w_wad.h"
#include "m_swap.h"
#include "p_local.h"

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


Vertex
BlockMap::getLineDistance(const Vertex& v)
{
    double x(0);
    if (fmod(v.getX() - oorgx, MAPBLOCKUNITS)) {
	x = MAPBLOCKUNITS - fmod(v.getX() - oorgx, MAPBLOCKUNITS);
    }
    double y(0);
    if (fmod(v.getY() - blockMap.oorgy, MAPBLOCKUNITS))
	y= MAPBLOCKUNITS - fmod(v.getY() - oorgy, MAPBLOCKUNITS);
    return Vertex(x, y);
}

BoundingBox
BlockMap::getBox(const Vertex& position, double extent) const
{
    int x = position.getX();
    int y = position.getY();
    int xl = (x - oorgx - extent) / DOUBLE_MAPBLOCKS_DIV;
    int xh = (x - oorgx + extent) / DOUBLE_MAPBLOCKS_DIV;
    int yl = (y - oorgy - extent) / DOUBLE_MAPBLOCKS_DIV;
    int yh = (y - oorgy + extent) / DOUBLE_MAPBLOCKS_DIV;
    return BoundingBox(xl, xh, yl, yh);
}

BoundingBox
BlockMap::getBox(const BoundingBox& box) const
{
    double xl = (box.getXl() - blockMap.oorgx - MMAXRADIUS) / DOUBLE_MAPBLOCKS_DIV;
    if (xl < 0 ) {
	xl = 0;
    }
    double xh = (box.getXh() - blockMap.oorgx + MMAXRADIUS) / DOUBLE_MAPBLOCKS_DIV;
    if (xh >= blockMap.width) {
	xh = blockMap.width - 1;
    }
    double yl  = (box.getYl() - blockMap.oorgy - MMAXRADIUS) / DOUBLE_MAPBLOCKS_DIV;
    if (yl < 0) {
	yl = 0;
    }
    double yh = (box.getYh() - blockMap.oorgy + MMAXRADIUS) / DOUBLE_MAPBLOCKS_DIV;
    if (yh >= blockMap.height) {
	yh = blockMap.height - 1;
    }
    return BoundingBox(xl, xh, yl, yh);
}
