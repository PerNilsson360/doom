#ifndef _SECTOR_HH_
#define _SECTOR_HH_

/*
  [4-9]: SECTORS
  ==============

  A SECTOR is a horizontal (east-west and north-south) area of the map
  where a floor height and ceiling height is defined. It can have any
  shape. Any change in floor or ceiling height or texture requires a
  new sector (and therefore separating linedefs and sidedefs). If you
  didn't already know, this is where you find out that DOOM is in many
  respects still a two-dimensional world, because there can only be ONE
  floor height in each sector. No buildings with two floors, one above
  the other, although fairly convincing illusions are possible.
  Each sector's record is 26 bytes, comprising 2 <short> fields, then
  2 <8-byte string> fields, then 3 <short> fields:

  (1) Floor is at this height for this sector
  (2) Ceiling height
  (3) name of the flat used for the floor texture, from the directory.
  (4) name of the flat used for the ceiling texture.
  All the flats in the directory between F_START and F_END work
  as either floors or ceilings. 
  (5) lightlevel of this sector: 0 = total dark, 255 (0xff) = maximum 
  light. There are actually only 32 brightnesses possible (see 
  COLORMAP [8-2]), so 0-7 are the same, ..., 248-255 are the same.
  (6) special sector: see [4-9-1] immediately below.
  (7) a "tag" number corresponding to LINEDEF(s) with the same tag
  number. When that linedef is activated, something will usually
  happen to this sector - its floor will rise, the lights will
  go out, etc. See [4-3-2] for the list of linedef effects.


  [4-9-1]: Special Sector Types
  -----------------------------

  Bytes 22-23 of each Sector record are a <short> which determines
  some area-effects called special sectors.
  Light changes are automatic. The brightness level will alternate
  between the light value specified for the special sector, and the lowest
  value amongst adjacent sectors (two sectors are adjacent if a linedef
  has a sidedef facing each sector). If there is no lower light value,
  or no adjacent sectors, then the "blink" sectors will instead alternate
  between 0 light and their own specified light level. The "oscillate"
  special (8) does nothing if there is no lower light level.
  "blink off" means the light is at the specified level most of the time,
  and changes to the lower value for just a moment. "blink on" means the
  light is usually at the lower value, and changes to the sector's value
  for just a moment. Every "synchronized" blinking sector on the level
  will change at the same time, whereas the unsynchonized blinking sectors
  change independently. "oscillate" means the light level goes smoothly
  from the lower to the higher and back; it takes about 2 seconds to go
  from maximum to minimum and back (255 down to 0 back up to 255).
  The damaging sector types only affect players, monsters suffer no ill
  effects from them whatsoever. Players will only take damage when they
  are standing on the floor of the damaging sector. "-10/20%" means that
  the player takes 20% damage at the end of every second that they are in
  the sector, except at skill 1, they will take 10% damage. If the player
  has armor, then the damage is split between health and armor.

  Dec Hex Class   Condition or effect

  0  00  -       Normal, no special characteristic.
  1  01  Light   random off
  2  02  Light   blink 0.5 second
  3  03  Light   blink 1.0 second
  4  04  Both    -10/20% health AND light blink 0.5 second
  5  05  Damage  -5/10% health
  7  07  Damage  -2/5% health
  8  08  Light   oscillates
  9  09  Secret  a player must stand in this sector to get credit for
  finding this secret. This is for the SECRETS ratio
  on inter-level screens.
  10  0a  Door    30 seconds after level start, ceiling closes like a door.
  11  0b  End     -10/20% health. If a player's health is lowered to less
  than 11% while standing here, then the level ends! Play
  proceeds to the next level. If it is a final level
  (levels 8 in DOOM 1, level 30 in DOOM 2), the game ends!
  12  0c  Light   blink 0.5 second, synchronized
  13  0d  Light   blink 1.0 second, synchronized
  14  0e  Door    300 seconds after level start, ceiling opens like a door.
  16  10  Damage  -10/20% health

  The following value can only be used in versions 1.666 and up, it will
  cause an error and exit to DOS in version 1.2 and earlier:

  17  11  Light   flickers on and off randomly

  All other values cause an error and exit to DOS. This includes these
  two values which were developed and are quoted by id as being available,
  but are not actually implemented in DOOM.EXE (as of version 1.666):

  6  06  -       crushing ceiling
  15  0f  -       ammo creator

  What a shame! The "ammo creator" sounds especially interesting!

*/

#include "r_defs.h"

class Sector
{
public:
    Sector(DataInput& dataInput);
    static int getBinarySize() { return 5 * 2 + 2 * 8; }
    double	ffloorheight;
    double      cceilingheight;
    short	floorpic;
    short	ceilingpic;
    short	lightlevel;
    short	special;
    short	tag;

    // 0 = untraversed, 1,2 = sndlines -1
    int		soundtraversed;

    // thing that made a sound (or null)
    Mob*	soundtarget;

    // mapblock bounding box for height changes
    BoundingBox box;

    
    // origin for any sounds played by the sector
    degenmobj_t	soundorg;

    // if == validcount, already checked
    int		validcount;

    // list of mobjs in sector
    Mob*	thinglist;

    // thinker_t for reversable actions
    void*	specialdata;

    int			linecount;
    Line**	lines;	// [linecount] size
};

#endif
