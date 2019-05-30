#ifndef _BSP_NODE_HH_
#define _BSP_NODE_HH_

#include "DivLine.hh"

class DataInput;

// BSP node structure.

// Indicate a leaf.
#define	NF_SUBSECTOR	0x8000

class BspNode : public DivLine
{
public:
    BspNode(const DataInput& dataInput);
    
    // Returns the size in the wad file.
    // Data consists of 14 short values.
    static int getBinarySize() { return 14 * 2; }
    unsigned short getChild(int side) const { return children[side]; }
    const double* getBBox(int side) const { return bbbox[side]; }
private:
    // Bounding box for each child.
    double bbbox[2][4];

    // If NF_SUBSECTOR its a subsector.
    unsigned short children[2];   
};

#endif
