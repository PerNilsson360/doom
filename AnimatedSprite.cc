#include "i_system.h"
#include "r_state.h"
#include "AnimatedSprite.hh"

SpriteFrame&
AnimatedSprite::getFrame(size_t i)
{
    if (i >= _frames.size()) {
	_frames.resize(i + 1);
    }
    return _frames[i];
}

void
AnimatedSprite::installLump(const std::string& name,
			   int lump,
			   unsigned frameIndex,
			   unsigned rotation,
			   bool flipped)
{
    if (frameIndex >= 29 || rotation > 8)
	I_Error("R_InstallSpriteLump: "
		"Bad frame characters in lump %i", lump);
    
    SpriteFrame& frame = getFrame(frameIndex);
    bool* rotate = frame.getRotate();
    if (rotation == 0) {
        // the lump should be used for all rotations
        if (rotate != nullptr && *rotate == false)
            I_Error ("R_InitSprites: Sprite %s frame %c has "
                     "multip rot=0 lump", name.c_str(), 'A'+frameIndex);
       
        if (rotate != nullptr && *rotate == true)
            I_Error ("R_InitSprites: Sprite %s frame %c has rotations "
                     "and a rot=0 lump", name.c_str(), 'A'+frameIndex);

        frame.setRotate(false);
        for (size_t r = 0; r < 8; r++) {
            frame.lump[r] = lump - firstspritelump;
            frame.flip[r] = (byte)flipped;
        }
        return;
    }
	

    if (rotate != nullptr && *rotate == false) {
        I_Error ("R_InitSprites: Sprite %s frame %c has rotations "
                 "and a rot=0 lump", name.c_str(), 'A'+frameIndex);
    }
		
    frame.setRotate(true);
    
    // make 0 based
    rotation--;

    if (frame.lump[rotation] != -1)
        I_Error ("R_InitSprites: Sprite %s : %c : %c "
                 "has two lumps mapped to it",
                 name.c_str(), 'A'+frameIndex, '1'+rotation);

    frame.lump[rotation] = lump - firstspritelump;
    frame.flip[rotation] = (byte)flipped;
}


void
AnimatedSprite::validate(const std::string& name)
{
    for (size_t i = 0, len = _frames.size(); i < len; i++) {
	const SpriteFrame& f = _frames[i];
	bool* rotate = f.getRotate();
	if (rotate == nullptr) { 
	    // no rotations were found for that frame at all
	    I_Error ("R_InitSprites: No patches found "
		     "for %s frame %c", name.c_str(), i+'A');
	} else if (*rotate) {
	    // must have all 8 frames
	    for (size_t j = 0; j < 8; j++) {
		if (f.lump[j] == -1) {
		    I_Error ("R_InitSprites: Sprite %s"
			     "is missing rotations",
			     name.c_str(), i+'A');
		}
	    }
	} else {
	    // only the first rotation is needed
	}
    }
}
