#ifndef _BSP_NODE_HH_
#define _BSP_NODE_HH_

#include "DivLine.hh"

class BspNode : public DivLine
{
public:
    // Bounding box for each child.
    double bbbox[2][4];

    // If NF_SUBSECTOR its a subsector.
    unsigned short children[2];   
};


#endif
