#ifndef _BOUNDINGBOX_HH_
#define _BOUNDINGBOX_HH_

#include <cassert>
#include <limits>

class Vertex;

class BoundingBox
{
public:
    BoundingBox() :
	_xl(std::numeric_limits<double>::max()),
	_xh(std::numeric_limits<double>::min()),
	_yl(std::numeric_limits<double>::max()),
	_yh(std::numeric_limits<double>::min()) {}
    BoundingBox(const Vertex& v1, const Vertex& v2);
    BoundingBox(double x, double y, double distance) :
	_xl(x - distance), _xh(x + distance),
	_yl(y - distance), _yh(y + distance) {}
    BoundingBox(double xl, double xh, double yl, double yh) :
	_xl(xl), _xh(xh), _yl(yl), _yh(yh) {}
    void add(double x, double y);
    bool disjunct(const BoundingBox& box) const;
    double getXl() const { return _xl; }
    double getXh() const { return _xh; }
    double getYl() const { return _yl; }
    double getYh() const { return _yh; }
    double get(int i) const {
	// @todo get rid of this
	// bit of a cludge needed for BSP code
	// top bottom left right
	switch (i) {
	case 0: return _yh;
	case 1: return _yl;
	case 2: return _xl;
	case 3: return _xh;
	default:
	    assert(false);
	}
    }
private:
    double _xl;
    double _xh;
    double _yl;
    double _yh;
};

#endif
