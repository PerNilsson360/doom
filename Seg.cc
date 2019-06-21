#include "DataInput.hh"
#include "Line.hh"

#include "Seg.hh"


Seg::Seg(DataInput& dataInput,
	 std::vector<Vertex>& vertices,
	 std::vector<Line>& lines,
	 std::vector<Side>& sides)
{
    short vertexNumber = dataInput.readShort();
    v1 = &vertices[vertexNumber];
    vertexNumber = dataInput.readShort();
    v2 = &vertices[vertexNumber];
    short	a = dataInput.readShort();
    aangle = Angle(a * Angle::A360 / 65536);
    short lineNumber = dataInput.readShort();
    linedef = &lines[lineNumber];;
    short side = dataInput.readShort();
    sidedef = &sides[linedef->sidenum[side]];
    frontsector = sides[linedef->sidenum[side]].sector;
    if (linedef->flags & ML_TWOSIDED) {
	backsector = sides[linedef->sidenum[side^1]].sector;
    } else  {
	backsector = nullptr;
    }
    ooffset = dataInput.readShort();
}
