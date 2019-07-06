#include <iostream>

#include "z_zone.h"
#include "i_system.h"
#include "r_state.h"
#include "w_wad.h"

#include "Picture.hh"
#include "AnimatedSprite.hh"

AnimatedSprite::~AnimatedSprite()
{
    for (SpriteFrame* s : _frames) {
	delete s;
    }
    _frames.clear();
}

SpriteFrame&
AnimatedSprite::getFrame(size_t i)
{
    if (i >= _frames.size()) {
	_frames.resize(i + 1);
    }
    SpriteFrame* result = _frames[i];
    if (result == nullptr) {
	result = new SpriteFrame();
	_frames[i] = result;
    }
    return *result;
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
    patch_t* patch =  (patch_t*)W_CacheLumpNum(lump, PU_CACHE);
    Picture* picture = new Picture(patch->width, patch->leftoffset, patch->topoffset, patch);
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
            frame.pictures[r] = picture;
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

    if (frame.pictures[rotation] != nullptr)
        I_Error ("R_InitSprites: Sprite %s : %c : %c "
                 "has two lumps mapped to it",
                 name.c_str(), 'A'+frameIndex, '1'+rotation);

    frame.pictures[rotation] = picture;
    frame.flip[rotation] = (byte)flipped;
}


void
AnimatedSprite::validate(const std::string& name)
{
    for (size_t i = 0, len = _frames.size(); i < len; i++) {
	const SpriteFrame* f = _frames[i];
	if (f == nullptr) {
	}
	bool* rotate = f->getRotate();
	if (rotate == nullptr) { 
	    std::cerr << "AnimatedSprite::validate: No patches found"
		      << " for " << name << "frame"  << i+'A'
		      << std::endl;
	    exit(1);
	} else if (*rotate) {
	    // must have all 8 frames
	    for (size_t j = 0; j < 8; j++) {
		if (f->pictures[j] == nullptr) {
		    std::cerr << "AnimatedSprite::validate: "
			      << "missing rotation"<< " for "
			      << name  << "frame"  << i+'A'
			      << std::endl;
		    exit(1);
		}
	    }
	} else {
	    // only the first rotation is needed
	}
    }
}
