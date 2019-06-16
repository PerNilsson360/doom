#include "DataInput.hh"

#include "r_data.h"
#include "Sector.hh"

#include "Side.hh"

Side::Side(DataInput& dataInput, std::vector<Sector>& sectors)
{
    ttextureoffset = dataInput.readShort();
    rrowoffset = dataInput.readShort();
    const std::string& topTextureName = dataInput.readEightBytes();
    toptexture = R_TextureNumForName(topTextureName.c_str());
    const std::string& bottomTextureName = dataInput.readEightBytes();
    bottomtexture = R_TextureNumForName(bottomTextureName.c_str());
    const std::string& midTextureName = dataInput.readEightBytes();
    midtexture = R_TextureNumForName(midTextureName.c_str());
    int sectorNumber = dataInput.readShort();
    sector = &sectors[sectorNumber];
}
