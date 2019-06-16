#include "m_bbox.h"
#include "DataInput.hh" 

#include "Line.hh"

Line::Line() :
    v1(nullptr),
    v2(nullptr),
    frontsector(nullptr),
    backsector(nullptr),
    specialdata(nullptr)
{
}

Line::Line(const DataInput& dataInput,
	   std::vector<Vertex>& vertices,
	   std::vector<Side>& sides)
{
    short vertexNumber = dataInput.readShort();
    v1 = &vertices[vertexNumber];
    vertexNumber = dataInput.readShort();
    v2 = &vertices[vertexNumber];
    ddx = v2->getX() - v1->getX();
    ddy = v2->getY() - v1->getY();
	
    if (ddx == 0) {
	slopetype = ST_VERTICAL;
    } else if (ddy == 0) {
	slopetype = ST_HORIZONTAL;
    } else {
	if (ddy / ddx > 0) {
            slopetype = ST_POSITIVE;
	} else {
            slopetype = ST_NEGATIVE;
	}
    }
		
    if (v1->getX() < v2->getX()) {
	bbbox[BOXLEFT] = v1->getX();
	bbbox[BOXRIGHT] = v2->getX();
    } else {
	bbbox[BOXLEFT] = v2->getX();
	bbbox[BOXRIGHT] = v1->getX();
    }

    if (v1->getY() < v2->getY()) {
	bbbox[BOXBOTTOM] = v1->getY();
	bbbox[BOXTOP] = v2->getY();
    } else {
	bbbox[BOXBOTTOM] = v2->getY();
	bbbox[BOXTOP] = v1->getY();
    }

    flags = dataInput.readShort();
    special = dataInput.readShort();;
    tag = dataInput.readShort();
    sidenum[0] = dataInput.readShort();
    sidenum[1] = dataInput.readShort();

    if (sidenum[0] != -1)
	frontsector = sides[sidenum[0]].sector;
    else
	frontsector = nullptr;

    if (sidenum[1] != -1)
	backsector = sides[sidenum[1]].sector;
    else
	backsector = nullptr;

}
