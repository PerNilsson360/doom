#ifndef _IMAGE_SCALER_
#define _IMAGE_SCALER_

#include <vector>

#include "r_defs.h"
#include "hu_lib.h"

class Screen;

// Doom graphics are made for 320 x 200

class ImageScaler
{
public:
    ImageScaler(int width, int height);
    void drawText(const hu_textline_t& line, bool drawcursor);
    void drawPatch(int x, int y, const patch_t* patch);
    void draw(int x, int y, unsigned char color);
    // Displays the image on the screen 0.0 is upper left corner
    void display(int x, int y, double zoom);
    void reset();
    int getWidth() const { return _width; }
    int getHeight() const { return _height; }
private:
    int _width;
    int _height;
    std::vector<std::vector<unsigned char> > _image;
};

#endif
