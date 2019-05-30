#include "DataInput.hh"
#include "Vertex.hh"

Vertex::Vertex(const DataInput& dataInput)
{
    _x = dataInput.readShort();
    _y = dataInput.readShort();
}
