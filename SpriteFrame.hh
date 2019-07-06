#ifndef _SPRITE_FRAME_HH_
#define _SPRITE_FRAME_HH_

#include <memory>
#include <vector>

#include "doomtype.h"

class Picture;

//	
// Sprites are patches with a special naming convention
//  so they can be recognized by R_InitSprites.
// The base name is NNNNFx or NNNNFxFx, with
//  x indicating the rotation, x = 0, 1-7.
// The sprite and frame specified by a thing_t
//  is range checked at run time.
// A sprite is a patch_t that is assumed to represent
//  a three dimensional object and may have multiple
//  rotations pre drawn.
// Horizontal flipping is used to save space,
//  thus NNNNF2F5 defines a mirrored patch.
// Some sprites will only have one picture used
// for all views: NNNNF0
//
class SpriteFrame
{
public:
    SpriteFrame();
    SpriteFrame(const SpriteFrame& s) = delete;
    bool* getRotate() const { return _rotate.get(); }
    void setRotate(bool rotate) { _rotate.reset(new bool(rotate)); }
private:
    // If false use 0 for any position.
    // Note: as eight entries are available,
    //  we might as well insert the same name eight times.
    std::unique_ptr<bool> _rotate;
public:
    // Pictures for view angles 0-7.
    std::vector<Picture*> pictures;
    // Flip bit (1 = flip) to use for view angles 0-7.
    byte	flip[8];
};

#endif
