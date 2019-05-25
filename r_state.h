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
//	Refresh/render internal state variables (global).
//
//-----------------------------------------------------------------------------


#ifndef __R_STATE__
#define __R_STATE__

// Need data structure definitions.
#include <vector>

#include "BspNode.hh"
#include "d_player.h"
#include "r_data.h"



#ifdef __GNUG__
#pragma interface
#endif



//
// Refresh internal data structures,
//  for rendering.
//

// needed for texture pegging
extern double*		ttextureheight;

// needed for pre rendering (fracs)
extern double*		sspritewidth;

extern double*		sspriteoffset;
extern double*		sspritetopoffset;

extern lighttable_t*	colormaps;

extern int		viewwidth;
extern int		viewheight;

extern int		firstflat;

// for global animation
extern int*		flattranslation;	
extern int*		texturetranslation;	


// Sprite....
extern int		firstspritelump;
extern int		lastspritelump;
extern int		numspritelumps;



//
// Lookup tables for map data.
//
extern int		numsprites;
extern spritedef_t*	sprites;

extern int		numvertexes;
extern vertex_t*	vertexes;

extern int		numsegs;
extern seg_t*		segs;

extern int		numsectors;
extern sector_t*	sectors;

extern int		numsubsectors;
extern subsector_t*	subsectors;

extern int		numnodes;
extern std::vector<BspNode>		nodes;

extern int		numlines;
extern line_t*		lines;

extern int		numsides;
extern side_t*		sides;


//
// POV data.
//
extern double		vviewx;
extern double		vviewy;
extern double		vviewz;

extern Angle		vviewangle;
extern player_t*	viewplayer;


// ?
extern Angle		cclipangle;
extern Angle xxtoviewangle[SCREENWIDTH+1];

extern double		rw_distance;
extern Angle		rrw_normalangle;



// angle to line origin
extern Angle		rrw_angle1;

// Segs count?
extern int		sscount;

extern visplane_t*	floorplane;
extern visplane_t*	ceilingplane;


#endif
//-----------------------------------------------------------------------------
//
// $Log:$
//
//-----------------------------------------------------------------------------
