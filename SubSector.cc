#include "DataInput.hh"

#include "SubSector.hh"

SubSector::SubSector(DataInput& dataInput)
{
    numlines = dataInput.readShort();
    firstline = dataInput.readShort();
}

