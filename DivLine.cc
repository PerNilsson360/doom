#include "stdio.h"

#include "DivLine.hh"


//
// P_PointOnDivlineSide
// Returns 0 or 1.
//
int
DivLine::pointOnSide(
    double px, 
    double py)
{
    double pdx;
    double pdy;
    double left;
    double right;
	
    if (!dx)
    {
        if (px <= x)
            return dy > 0;
        
        return dy < 0;
    }

    if (!dy)
    {
        if (py <= y)
            return dx < 0;
        
        return dx > 0;
    }
	
    pdx = px - x;
    pdy = py - y;
	
    left = dy * pdx;
    right = pdy * dx;
	
    if (right < left)
	return 0;		// front side
    return 1;			// back side
}

//
// P_InterceptVector
// Returns the fractional intercept point
// along the first divline.
// This is only called by the addthings
// and addlines traversers.
//
double
DivLine::interceptVector( 
    const DivLine& divLine)
{
    double den = divLine.dy*dx - divLine.dx*dy;

    if (den == 0)
        return 0;	// parallel
    
    double num = (divLine.x - x)*divLine.dy + (y - divLine.y)*divLine.dx;

    return num / den;;
}
