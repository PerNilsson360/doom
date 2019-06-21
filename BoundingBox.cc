#include "Vertex.hh"
#include "BoundingBox.hh"

BoundingBox::BoundingBox(const Vertex& v1, const Vertex& v2)
{
    double x1 = v1.getX();
    double y1 = v1.getY();
    double x2 = v2.getX();
    double y2 = v2.getY();
    if (x1 < x2) {
	_xl = x1;
	_xh = x2;
    } else {
	_xl = x2;
	_xh = x1;
    }
    if (y1 < y2) {
	_yl = y1;
	_yh = y2;
    } else {
	_yl = y2;
	_yh = y1;
    }
}

bool
BoundingBox::disjunct(const BoundingBox& box) const
{
    return
	_xh <= box._xl ||
	_xl >= box._xh ||
	_yh <= box._yl ||
	_yl >= box._yh;
}
