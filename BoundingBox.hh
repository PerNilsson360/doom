#ifndef _BOUNDINGBOX_HH_
#define _BOUNDINGBOX_HH_

class Vertex;

class BoundingBox
{
public:
    BoundingBox() : _xl(0), _xh(0), _yl(0), _yh(0) {}
    BoundingBox(const Vertex& v1, const Vertex& v2);
    BoundingBox(double x, double y, double distance) :
	_xl(x - distance), _xh(x + distance),
	_yl(y - distance), _yh(y + distance) {}
    BoundingBox(double xl, double xh, double yl, double yh) :
	_xl(xl), _xh(xh), _yl(yl), _yh(yh) {}
    bool disjunct(const BoundingBox& box);
    double getXl() const { return _xl; }
    double getXh() const { return _xh; }
    double getYl() const { return _yl; }
    double getYh() const { return _yh; }
private:
    double _xl;
    double _xh;
    double _yl;
    double _yh;
};

#endif
