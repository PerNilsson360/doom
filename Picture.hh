#ifndef _PICTURE_HH_
#define _PICTURE_HH_

/*
[5-1]: Picture Format
=====================

  Each picture has three sections. First, an 8-byte header composed of
four short-integers. Then a number of long-integer pointers. Then the
picture's pixel/color data. See [A-1] for concise BNF style definitions,
here is a meatier explanation of the format:

(A) The header's four fields are:

  (1) Width. The number of columns of picture data.
  (2) Height. The number of rows.
  (3) Left offset. The number of pixels to the left of the center;
        where the first column gets drawn.
  (4) Top offset. The number of pixels above the origin;
        where the top row is.

  The width and height define a rectangular space or limits for drawing
a picture within. To be "centered", (3) is usually about half of the
total width. If the picture had 30 columns, and (3) was 10, then it
would be off-center to the right, especially when the player is standing
right in front of it, looking at it. If a picture has 30 rows, and (4)
is 60, it will appear to "float" like a blue soul-sphere. If (4) equals
the number of rows, it will appear to rest on the ground. If (4) is less
than that for an object, the bottom part of the picture looks awkward.
  With walls patches, (3) is always (columns/2)-1, and (4) is always
(rows)-5. This is because the walls are drawn consistently within their
own space (There are two integers in each SIDEDEF which can offset the
starting position for drawing a wall's texture within the wall space).

  Finally, if (3) and (4) are NEGATIVE integers, then they are the
absolute coordinates from the top-left corner of the screen, to begin
drawing the picture, assuming the VIEW is full-screen (i.e., the full
320x200). This is only done with the picture of the player's current
weapon - fist, chainsaw, bfg9000, etc. The game engine scales the
picture down appropriatelyif the view is less than full-screen.

(B) After the header, there are N = field (1) = <width> = (# of columns)
4-byte <long> integers. These are pointers to the data for each COLUMN.
The value of the pointer represents the offset in bytes from the first
byte of the picture lump.

(C) Each column is composed of some number of BYTES (NOT integers),
arranged in "posts":

  The first byte is the row to begin drawing this post at. 0 means
whatever height the header (4) upwards-offset describes, larger numbers
move correspondingly down.
  The second byte is how many colored pixels (non-transparent) to draw,
going downwards.
  Then follow (# of pixels) + 2 bytes, which define what color each
pixel is, using the game palette. The first and last bytes AREN'T drawn,
and I don't know why they are there. Probably just leftovers from the
creation process on the NeXT machines. Only the middle (# of pixels in
this post) are drawn, starting at the row specified in the first byte
of the post.
  After the last byte of a post, either the column ends, or there is
another post, which will start as stated above.
  255 (0xFF) ends the column, so a column that starts this way is a null
column, all "transparent". Draw the next column.
 */

class Picture
{
public:
    Picture(int16_t width,
	    int16_t leftOffset,
	    int16_t topOffset,
	    patch_t* data) :
	_width(width),
	_leftOffset(leftOffset),
	_topOffset(topOffset),
	_data(data) {}
    int16_t getWidth() const { return _width; }
    int16_t getLeftOffset() const { return _leftOffset; }
    int16_t getTopOffset() const { return _topOffset; }
    patch_t* getData() const {return _data; }
private:
    int16_t _width;
    int16_t _leftOffset;
    int16_t _topOffset;
    patch_t* _data;
};


#endif
