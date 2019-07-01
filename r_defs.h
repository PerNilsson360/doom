// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// DESCRIPTION:
//      Refresh/rendering module, shared data struct definitions.
//
//-----------------------------------------------------------------------------


#ifndef __R_DEFS__
#define __R_DEFS__

#include <memory>
#include <vector>

// Screenwidth.
#include "doomdef.h"

// We rely on the thinker data struct
// to handle sound origins in sectors.
#include "d_think.h"
// SECTORS do store MObjs anyway.
#include "Mob.hh"
#include "Vertex.hh"
#include "Side.hh"
#include "SpriteFrame.hh"

#include <stdint.h>

#ifdef __GNUG__
#pragma interface
#endif

class Sector;
class Line;
class Seg;

// Silhouette, needed for clipping Segs (mainly)
// and sprites representing things.
#define SIL_NONE		0
#define SIL_BOTTOM		1
#define SIL_TOP			2
#define SIL_BOTH		3

#define MAXDRAWSEGS		256



//
// INTERNAL MAP TYPES
//  used by play and refresh
//

// Each sector has a degenmobj_t in its center
//  for sound origin purposes.
// I suppose this does not handle sound from
//  moving objects (doppler), because
//  position is prolly just buffered, not
//  updated.
typedef struct
{
    thinker_t		thinker;	// not used for anything
    double		xx;
    double		yy;
    double              zz;

} degenmobj_t;

// posts are runs of non masked source pixels
typedef struct
{
    byte		topdelta;	// -1 is the last post in a column
    byte		length; 	// length data bytes follows
} post_t;

// column_t is a list of 0 or more post_t, (byte)-1 terminated
typedef post_t	column_t;

//
// OTHER TYPES
//

// This could be wider for >8 bit display.
// Indeed, true color support is posibble
//  precalculating 24bpp lightmap/colormap LUT.
//  from darkening PLAYPAL to all black.
// Could even us emore than 32 levels.
typedef byte	lighttable_t;	


//
// ?
//
typedef struct drawseg_s
{
    Seg*		curline;
    int			x1;
    int			x2;

    double		sscale1;
    double 		sscale2;
    double		sscalestep;

    // 0=none, 1=bottom, 2=top, 3=both
    int			silhouette;

    // do not clip sprites above this
    double		bbsilheight;

    // do not clip sprites below this
    double		ttsilheight;
    
    // Pointers to lists for sprite clipping,
    //  all three adjusted so [x1] is first value.
    short*		sprtopclip;		
    short*		sprbottomclip;	
    short*		maskedtexturecol;
    
} drawseg_t;

// Patches.
// A patch holds one or more columns.
// Patches are used for sprites and all masked pictures,
// and we compose textures from the TEXTURE1/2 lists
// of patches.
typedef struct 
{ 
    short		width;		// bounding box size 
    short		height; 
    short		leftoffset;	// pixels to the left of origin 
    short		topoffset;	// pixels below the origin 
    int32_t		columnofs[8];	// only [width] used
    // the [0] is &columnofs[width] 
} patch_t;

// A vissprite_t is a thing
//  that will be drawn during a refresh.
// I.e. a sprite object that is partly visible.
typedef struct vissprite_s
{
    // Doubly linked list.
    struct vissprite_s*	prev;
    struct vissprite_s*	next;
    
    int			x1;
    int			x2;

    // for line side calculation
    double		ggx;
    double		ggy;		

    // global bottom / top for silhouette clipping
    double		ggz;
    double		ggzt;

    // horizontal position of x1
    double		sstartfrac;
    
    double		sscale;
    
    // negative if flipped
    double		xxiscale;	

    double		ttexturemid;
    int			patch;

    // for color translation and shadow draw,
    //  maxbright frames as well
    lighttable_t*	colormap;
   
    int			mobjflags;
    
} vissprite_t;

//
// Now what is a visplane, anyway?
// 
typedef struct
{
  double hheight;
  int			picnum;
  int			lightlevel;
  int			minx;
  int			maxx;
  
  // leave pads for [minx-1]/[maxx+1]
  
  unsigned short		pad1;
  // Here lies the rub for all
  //  dynamic resize/change of resolution.
  unsigned short		top[SCREENWIDTH];
  unsigned short		pad2;
  unsigned short		pad3;
  // See above.
  unsigned short		bottom[SCREENWIDTH];
  unsigned short		pad4;

} visplane_t;




#endif
//-----------------------------------------------------------------------------
//
// $Log:$
//
//-----------------------------------------------------------------------------
