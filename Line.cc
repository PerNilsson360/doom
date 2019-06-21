#include "m_bbox.h"
#include "p_local.h"

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

    box = BoundingBox(*v1, *v2);

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

int
Line::boxOnLineSide(const BoundingBox& box)
{
    int		p1;
    int		p2;
    switch (slopetype) {
    case Line::ST_HORIZONTAL:
        p1 = box.getYh() > v1->getY();
        p2 = box.getYl() > v1->getY();
        if (ddx < 0) {
            p1 ^= 1;
            p2 ^= 1;
        }
        break;
    case Line::ST_VERTICAL:
        p1 = box.getXh() < v1->getX();
        p2 = box.getXl() < v1->getX();
        if (ddy < 0) {
            p1 ^= 1;
            p2 ^= 1;
        }
        break;
        
    case Line::ST_POSITIVE:
        p1 = P_PointOnLineSide(Vertex(box.getXl(), box.getYh()), this);
        p2 = P_PointOnLineSide(Vertex(box.getXh(), box.getYl()), this);
	break;
	
    case Line::ST_NEGATIVE:
        p1 = P_PointOnLineSide(Vertex(box.getXh(), box.getYh()), this);
        p2 = P_PointOnLineSide(Vertex(box.getXl(), box.getYl()), this);
        break;
    }
    
    if (p1 == p2)
        return p1;
    return -1;
}
