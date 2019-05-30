#include <cstdio>

#include "r_defs.h"
#include "DataInput.hh"
#include "DivLine.hh"

DivLine::DivLine() : x(0), y(0), dx(0), dy(0)
{
}

DivLine::DivLine(const Vertex& v1, const Vertex& v2) :
    DivLine(v1.getX(), v2.getX(), v1.getY(), v2.getY())
{
}

DivLine::DivLine(double x1, double x2, double y1, double y2) :
    x(x1), y(y1), dx(x2 - x1), dy(y2 - y1)
{
}

DivLine::DivLine(const line_t& li) :
    x(li.v1->getX()),
    y(li.v1->getY()),
    dx(li.ddx),
    dy(li.ddy)
{
}

DivLine::DivLine(const DataInput& dataInput)
{
    x = dataInput.readShort();
    y = dataInput.readShort();
    dx = dataInput.readShort();
    dy = dataInput.readShort();
}

int
DivLine::moveX(double fraction) const
{
    return x + (dx *  fraction);
}

int
DivLine::moveY(double fraction) const
{
    return y + (dy *  fraction);
}

bool
DivLine::isChangeBiggerThan(double d) const
{
    return dx > d || dy > d || dx < -d || dy < -d;
}

bool
DivLine::isPositive() const
{
    return dx * dy > 0;
}

//
// P_PointOnDivlineSide
// Returns 0 or 1.
//
int
DivLine::pointOnSide(double px, double py) const
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

int
DivLine::pointOnSide(const Vertex& vertex) const
{
    return pointOnSide(vertex.getX(), vertex.getY());
}

//
// P_InterceptVector
// Returns the fractional intercept point
// along the first divline.
// This is only called by the addthings
// and addlines traversers.
//
double
DivLine::interceptVector(const DivLine& divLine) const
{
    double den = divLine.dy*dx - divLine.dx*dy;

    if (den == 0)
        return 0;	// parallel
    
    double num = (divLine.x - x)*divLine.dy + (y - divLine.y)*divLine.dx;

    return num / den;;
}
