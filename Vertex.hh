#ifndef _VERTEX_HH_
#define _VERTEX_HH_

class DataInput;
class Angle;

class Vertex
{
public:
    Vertex(double x = 0, double y = 0);
    Vertex(const Vertex& v, double distance, const Angle& angle);
    Vertex(const DataInput& dataInput);
    // Returns the size in the wad file.
    // Data consists of 2 short values.
    static int getBinarySize() { return 2 * 2; }
    double distance(const Vertex& v);
    void increment(double x, double y) { _x += x; _y += y; } 
    double getX() const { return _x; }
    double getY() const { return _y; }
    Vertex& operator += (const Vertex& v) {
	_x += v._x;
	_y += v._y;
        return *this;
    }

private:
    double _x;
    double _y;
};

inline Vertex operator - (const Vertex& left, const Vertex& right) {
    return Vertex(left.getX() - right.getX(),
		  left.getY() - right.getY()); 
}

inline bool operator == (const Vertex& left, const Vertex& right) {
    return left.getX() == right.getX() && left.getY() == right.getY(); 
}


#endif
