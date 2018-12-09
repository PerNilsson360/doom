#include <cmath>
#include <memory>

#include "i_video.h"
#include "m_swap.h"
#include "ImageScaler.hh"


ImageScaler::ImageScaler(int width, int height) :
    _width(width),
    _height(height),
    _image(_width, std::vector<unsigned char>(_height, 0xFF))
{
}

void
ImageScaler::drawPatch(int x, int y, const patch_t* patch)
{
    y -= SHORT(patch->topoffset); 
    x -= SHORT(patch->leftoffset); 
#ifdef RANGECHECK 
    if (x < 0 ||
	x + SHORT(patch->width) > _width ||
	y < 0 ||
	y + SHORT(patch->height) > _height) {
	fprintf(stderr, "Patch at %d,%d exceeds LFB\n", x,y );
	exit(1);
    }
#endif  
    int i = x;
    int width = SHORT(patch->width); 
    for (int col = 0; col < width; i++, col++) {
	column_t* column =
	    (column_t *)((byte *)patch + LONG(patch->columnofs[col])); 
 	// step through the posts in a column 
	while (column->topdelta != 0xff )  { 
	    byte* source = (byte *)column + 3;
	    int j = y + column->topdelta;
	    int count = column->length; 
	    while (count--) { 
		_image[i][j++] = *source++; 
	    } 
	    column = (column_t *)((byte *)column + column->length + 4); 
	} 
    }			 
}

void
ImageScaler::drawText(const hu_textline_t& line, bool drawcursor)
{

    int			i;
    int			w;
    int			x;
    unsigned char	c;

    // draw the new stuff
    x = line.x;
    for (i=0;i<line.len;i++)
    {
	c = toupper(line.l[i]);
	if (c != ' '
	    && c >= line.sc
	    && c <= '_')
	{
	    w = SHORT(line.f[c - line.sc]->width);
	    if (x+w > _width)
		break;
	    drawPatch(x, line.y, line.f[c - line.sc]);
	    x += w;
	}
	else
	{
	    x += 4;
	    if (x >= _width)
		break;
	}
    }

    // draw the cursor if requested
    if (drawcursor
	&& x + SHORT(line.f['_' - line.sc]->width) <= _width)
    {
	drawPatch(x, line.y, line.f['_' - line.sc]);
    }
}

void
ImageScaler::draw(int x, int y, unsigned char color)
{
    _image[x][y] = color;
}

// Taken from dr'dobbs journal
void
ImageScaler::display(int bx, int by, double zoom)
{
    /* Calculate the differential amount */
    int izoom = (int)(1.0/zoom * 1000);
    /* Allocate a buffer for a scan line from original image, and a
    ** resized scan line */
    auto line = std::make_unique<unsigned char[]>(
	static_cast<int>(ceil(zoom * _width + 1)));
    /* Initialize the output Y value and vertial differential */
    int y = 0;
    int dday = 0;
    /* Loop over rows in the original image */
    for (int i = 0; i < _height; i++) {
        /* Adjust the vertical accumulated differential, initialize the
        ** output X pixel and horizontal accumulated differential */
        dday -= 1000;
        int x = 0;
        int ddax = 0;
        /* Loop over pixels in the original image */
        for (int j = 0; j < _width; j++) {
            /* Adjust the horizontal accumulated differential */
            ddax -= 1000;
            while (ddax < 0) {
                /* Store values from original image scanline into the scaled
                ** buffer until accumulated differential crosses threshold */
                line[x] = _image[j][i];
                x++;
                ddax += izoom;
            }
        }
        while (dday < 0) {
            /* The 'outer loop' -- output resized scan lines until the
            ** vertical threshold is crossed */
            dday += izoom;
            for (int k = 0; k < x; k++) {
		// translating to (0.0) at bottom left corner
		unsigned char c = line[k];
		if (c != 0xFF) {
		    I_Draw(bx + k, by + y, line[k]);
		}
            }
            y++;
        }
    }
}

void
ImageScaler::reset()
{
    for (int i = 0; i < _width; i++) {
	for (int j = 0; j < _height; j++) {
	    _image[i][j] = 0xFF;
	}
    }
}
