#ifndef _BSP_NODE_HH_
#define _BSP_NODE_HH_

#include "DivLine.hh"
#include "BoundingBox.hh"

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
    const BoundingBox& getBBox(int side) const { return _box[side]; }
private:
    // Bounding box for each child.
    BoundingBox _box[2];

    // If NF_SUBSECTOR its a subsector.
    unsigned short children[2];   
};

#endif
