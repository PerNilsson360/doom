#ifndef _ANIMATED_SPRITE_HH_
#define _ANIMATED_SPRITE_HH_

/*
---------------------
CHAPTER [5]: Graphics
---------------------

  The great majority of the entries in the directory reference lumps
that are in a special picture format. The same format is used for the
sprites (monsters, items), the wall patches, and various miscellaneous
pictures for the status bar, menu text, inter-level map, etc. Every
one of these picture lumps contains exactly one picture. The flats
(floor and ceiling pictures) are NOT in this format, they are raw data;
see chapter [6].
  A great many of these lumps are used in sprites. A "sprite" is the
picture or pictures necessary to display any of the THINGS. Some of
them are simple, a single lump like SUITA0. Most of the monsters have
50 or more lumps.
  The first four letters of these lumps are the sprite's "name". TROO
is for imps, BKEY is for the blue key, and so on. See [4-2-1] for a list
of them all. The fifth letter in the lump is an indication of what "frame"
it is, for animation sequences. The letters correspond to numbers, ASCII
"A" equalling 0, "B" is 1, ... "Z" is 25, etc. The highest needed by a
DOOM 1 sprite is W=22, but some of the DOOM 2 monsters need a few more
frames.
  The "0" in the lump name is for "rotations" or "rot"s. All the
static objects like torches and barrels and dead bodies look the same
from any angle. This is because they have a "rot=0 lump" as DOOM itself
might say. Monsters and projectiles look different from different
angles. This is done with rots 1-8. This diagram shows how rot 1 is for
the front and they go counter-clockwise (looking from above) to 8:

        3
      4 | 2
       \|/
     5--*----> 1   Thing is facing this direction
       /|\
      6 | 8
        7

  Many things have sets of lumps like this: TROOA1, TROOA2A8, TROOA3A7,
TROOA4A6, TROOA5, TROOB1, etc. This means that for frame 0 (A), the
pairs of rots/angles (2 and 8), (3 and 7), and (4 and 6) are mirror-
images. In the long run, this saves space in the wad file, and from the
designer's point of view, it's 37% fewer pictures to have to draw.
  If a sprite's frame has a non-zero rot, it needs to have ALL 8 of
them. Also note that no more than two rots can be squeezed into one
lump's name. Some other two-rot lumps with a different format are
shown in the SPIDA1D1, SPIDA2D2, etc. lumps.

  IMPORTANT: Sprite lumps and flats cannot be added or replaced via pwads
unless they ALL are. That is, ALL sprites' lumps must be located in a
single wad file, and ALL flats' lumps must be in a single wad file. Wall
patches CAN be used in external wads, because the PNAMES lump gives a
number to every pname, and is used as a quick-index list to load in
wall patches.
  Version 1.666 was rumored to be able to include sprites in pwads (in
fact the README says it can), but it can't.
*/

#include <vector>

class SpriteFrame;

class AnimatedSprite
{
public:
    ~AnimatedSprite();
private:
    SpriteFrame& getFrame(size_t i);
public:
    void installLump(const std::string& name,
		    int lump,
		    unsigned frameIndex,
		    unsigned rotation,
		    bool flipped);
    void validate(const std::string& name);
//private:
    std::vector<SpriteFrame*> _frames;
};

#endif
