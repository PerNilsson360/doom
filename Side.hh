#ifndef _SIDE_HH_
#define _SIDE_HH_

/*
[4-4]: SIDEDEFS
===============

  A sidedef is a definition of what wall texture(s) to draw along a
LINEDEF, and a group of sidedefs outline the space of a SECTOR.
  There will be one sidedef for a line that borders only one sector
(and it must be the RIGHT side, as noted in [4-3]). It is not 
necessary to define what the doom player would see from the other 
side of that line because the doom player can't go there. The doom
player can only go where there is a sector.
  Each sidedef's record is 30 bytes, comprising 2 <short> fields, then
3 <8-byte string> fields, then a final <short> field:

(1) X offset for pasting the appropriate wall texture onto the wall's
        "space": positive offset moves into the texture, so the left
        portion gets cut off (# of columns off left side = offset).
        Negative offset moves texture farther right, in the wall's space.
(2) Y offset: analogous to the X, for vertical.
(3) "upper" texture name: the part above the juncture with a lower
        ceiling of an adjacent sector.
(4) "lower" texture name: the part below a juncture with a higher
        floored adjacent sector.
(5) "middle" texture name: the regular part of the wall. Also known as
        "normal" or "full" texture.
(6) SECTOR that this sidedef faces or helps to surround.

  The texture names are from the TEXTURE1/2 resources. The names of
wall patches in the directory (between P_START and P_END) are not
directly used, they are referenced through the PNAMES lump.
  Simple sidedefs have no upper or lower texture, and so they will have
"-" instead of a texture name. Also, two-sided lines can be transparent,
in which case "-" means transparent (no texture).
  If the wall is wider than the texture to be pasted onto it, then the
texture is tiled horizontally. A 64-wide texture will be pasted at 0,
64, 128, etc., unless an X-offset changes this.
  If the wall is taller than the texture, than the texture is tiled
vertically, with one very important difference: it starts new tiles
ONLY at 128, 256, 384, etc. So if the texture is less than 128 high,
there will be junk filling the undefined areas, and it looks ugly.
This is sometimes called the "Tutti Frutti" effect.

  There are some transparent textures which can be used as middle textures
on 2-sided sidedefs (between sectors). These textures need to be composed
of a single patch (see [8-4]), and note that on a very tall wall, they
will NOT be tiled. Only one will be placed, at the spot determined by
the "lower unpegged" flag being on/off and the sidedef's y offset. And
if a transparent texture is used as an upper or lower texture, then
the good old "Tutti Frutti" effect will have its way.
  Also note that animated wall textures (see [8-4-1]) do NOT animate
if they are the "middle" texture on a 2-sided line. So much for the
lava waterfall with the hidden room at its base...hmm, maybe not...
*/

#include <vector>

class DataInput;
class Sector;

class Side
{
public:
    Side(DataInput& dataInput, std::vector<Sector>& sectors);
    static int getBinarySize() { return 3 * 2 + 3 * 8; }
//private:
    // add this to the calculated texture column
    double ttextureoffset;
    // add this to the calculated texture top
    double rrowoffset;
    // Texture indices.
    // We do not maintain names here. 
    short toptexture;
    short bottomtexture;
    short midtexture;
    // Sector the SideDef is facing.
    Sector*	sector;
};

#endif
