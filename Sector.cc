#include "DataInput.hh"
#include "r_data.h"

#include "Sector.hh"

Sector::Sector(DataInput& dataInput) :
    soundtarget(nullptr),
    thinglist(nullptr),
    specialdata(nullptr),
    linecount(0)
{
    ffloorheight = dataInput.readShort();
    cceilingheight = dataInput.readShort();
    const std::string& floorPicName = dataInput.readEightBytes();
    floorpic = R_FlatNumForName(floorPicName.c_str());
    const std::string& ceilingPicName = dataInput.readEightBytes();
    ceilingpic = R_FlatNumForName(ceilingPicName.c_str());
    lightlevel = dataInput.readShort();
    special = dataInput.readShort();
    tag = dataInput.readShort();
}
