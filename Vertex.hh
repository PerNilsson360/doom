#ifndef _VERTEX_HH_
#define _VERTEX_HH_

class DataInput;

class Vertex
{
public:
    Vertex(const DataInput& dataInput);
    // Returns the size in the wad file.
    // Data consists of 2 short values.
    static int getBinarySize() { return 2 * 2; }
    double getX() const { return _x; }
    double getY() const { return _y; }
private:
    double _x;
    double _y;
};

#endif
