#include "SpriteFrame.hh"

SpriteFrame::SpriteFrame()
{
    for (int i = 0; i < 8; i++) {
	lump[i] = -1;
	flip[i] = -1;
    }
}

SpriteFrame::SpriteFrame(const SpriteFrame& s)
{
    for (int i = 0; i < 8; i++) {
	lump[i] = s.lump[i];
	flip[i] = s.flip[i];
    }
    bool* rotate = s.getRotate();
    if (rotate != nullptr) {
	_rotate.reset(new bool(*rotate));
    }
}

