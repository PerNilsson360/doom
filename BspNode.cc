#include "DataInput.hh"
#include "BspNode.hh"

BspNode::BspNode(const DataInput& dataInput) :
    DivLine(dataInput)
{
    for (int i = 0; i < 2; i++) {
	for (int j = 0; j < 4; j++) {
	    bbbox[i][j] = dataInput.readShort();
	}
    }
    for (int i = 0; i < 2; i++) {
	children[i] = dataInput.readShort();
    }
}
