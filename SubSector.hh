#ifndef _SUBSECTOR_HH_
#define _SUBSECTOR_HH_

/*
[4-7]: SSECTORS
===============

  SSECTOR stands for sub-sector. These divide up all the SECTORS into
convex polygons. They are then referenced through the NODES resources.
There will be (number of nodes + 1) ssectors.
  Each ssector is 4 bytes in 2 <short> fields:

(1) This many SEGS are in this SSECTOR...
(2) ...starting with this SEG number

  The segs in ssector 0 should be segs 0 through x, then ssector 1
contains segs x+1 through y, ssector 2 containg segs y+1 to z, etc.
*/

class DataInput;
class Sector;

class SubSector
{
public:
    SubSector(DataInput& dataInput);
    static int getBinarySize() { return 2 * 2; }
//private:
    Sector* sector;
    short numlines;
    short firstline;
};

#endif
