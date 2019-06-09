#ifndef _SEGS_HH_
#define _SEGS_HH_

/*
  [4-6]: SEGS
  ===========

  The SEGS are stored in a sequential order determined by the SSECTORS,
  which are part of the NODES recursive tree.
  Each seg is 12 bytes in 6 <short> fields:

  (1) start of seg is VERTEX with this number
  (2) end VERTEX
  (3) angle: 0= east, 16384=north, -16384=south, -32768=west.
  In hex, it's 0000=east, 4000=north, 8000=west, c000=south.
  This is also know as BAMS for Binary Angle Measurement.
  (4) LINEDEF that this seg goes along
  (5) direction: 0 if the seg goes the same direction as the linedef it
  is on, 1 if the seg goes the opposite direction. This is the
  same as (0 if the seg is on the RIGHT side of the linedef) or
  (1 if the seg is on the LEFT side of the linedef).
  (6) offset: distance along the linedef to the start of this seg (the
  vertex in field 1). The offset is in the same direction as the
  seg. If field 5 is 0, then the distance is from the "start"
  vertex of the linedef to the "start" vertex of the seg. If field
  5 is 1, then the offset is from the "end" vertex of the linedef
  to the "start" vertex of the seg. So if the seg begins at one of
  the two endpoints of the linedef, this offset will be 0.

  For diagonal segs, the offset distance can be obtained from the
  formula DISTANCE = SQR((x2 - x1)^2 + (y2 - y1)^2). The angle can be
  calculated from the inverse tangent of the dx and dy in the vertices,
  multiplied to convert PI/2 radians (90 degrees) to 16384. And since
  most arctan functions return a value between -(pi/2) and (pi/2),
  you'll have to do some tweaking based on the sign of (x2-x1), to
  account for segs that go "west".

*/

class Sector;

class Seg
{
public:
    static int getBinarySize() { return 6 * 2; }
    // private:
    Vertex*	v1;
    Vertex*	v2;
    
    double	ooffset;

    Angle	aangle;

    side_t*	sidedef;
    line_t*	linedef;

    // Sector references.
    // Could be retrieved from linedef, too.
    // backsector is NULL for one sided lines
    Sector*	frontsector;
    Sector*	backsector;
};
#endif
