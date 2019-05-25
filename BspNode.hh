#ifndef _BSP_NODE_HH_
#define _BSP_NODE_HH_

#include "DivLine.hh"

class DataInput;

class BspNode : public DivLine
{
public:
    BspNode(const DataInput& dataInput);
    
    // Returns the size in the wad file.
    // Data consists of 14 short values.
    static int getBinarySize() { return 14 * 2; }
    //private:
    // Bounding box for each child.
    double bbbox[2][4];

    // If NF_SUBSECTOR its a subsector.
    unsigned short children[2];   
};

#endif
