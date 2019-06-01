#include <cmath>

#include "DataInput.hh"
#include "Angle.hh"
#include "Vertex.hh"

Vertex::Vertex(double x, double y) : _x(x), _y(y)
{
}

Vertex::Vertex(const Vertex& v,
	       double distance,
	       const Angle& angle)
{
    _x = v._x + (distance * cos(angle));
    _y = v._y + (distance * sin(angle));
}

Vertex::Vertex(const DataInput& dataInput) :
    _x(dataInput.readShort()),
    _y(dataInput.readShort())
{
}

double
Vertex::distance(const Vertex& v)
{
    double dx = _x - v.getX();
    double dy = _y - v.getY();
    return sqrt((dx * dx) + (dy * dy));
}
