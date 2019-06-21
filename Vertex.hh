#ifndef _VERTEX_HH_
#define _VERTEX_HH_

class DataInput;
class Angle;

/*
  [4-5]: VERTEXES
  ===============
  
  These are the beginning and end points for LINEDEFS and SEGS. Each
  vertice's record is 4 bytes in 2 <short> fields:

  (1) X coordinate
  (2) Y coordinate

  On the automap within the game, with the grid on (press 'G'), the
  lines are 128 apart (0x80), two lines = 256 (0x100).
  A note on the coordinates: the coordinate system used for the vertices
  and the heights of the sectors corresponds to pixels, for purposes of
  texture-mapping. So a sector that's 128 high, or a multiple of 128, is
  pretty typical, since many wall textures are 128 pixels high.
  And yes, the correct spelling of the plural of "vertex" is "vertices".
*/

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

inline Vertex operator + (const Vertex& left, const Vertex& right) {
    return Vertex(left.getX() + right.getX(),
		  left.getY() + right.getY()); 
}

inline bool operator == (const Vertex& left, const Vertex& right) {
    return left.getX() == right.getX() && left.getY() == right.getY(); 
}


#endif
