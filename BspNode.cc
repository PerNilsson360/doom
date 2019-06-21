#include "DataInput.hh"
#include "BoundingBox.hh"
#include "BspNode.hh"

BspNode::BspNode(const DataInput& dataInput) :
    DivLine(dataInput)
{
    // Bounding box order is top, bottom, left, right
    for (int i = 0; i < 2; i++) {
	short yh = dataInput.readShort();
	short yl = dataInput.readShort();
	short xl = dataInput.readShort();
	short xh = dataInput.readShort();
	_box[i] = BoundingBox(xl, xh, yl, yh);
    }
    for (int i = 0; i < 2; i++) {
	children[i] = dataInput.readShort();
    }
}
