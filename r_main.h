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
//	System specific interface stuff.
//
//-----------------------------------------------------------------------------


#ifndef __R_MAIN__
#define __R_MAIN__

#include "d_player.h"
#include "r_data.h"


#ifdef __GNUG__
#pragma interface
#endif


//
// POV related.
//
extern double vviewcos;
extern double  vviewsin;

extern int		centerx;
extern int		centery;
extern double		pprojection;

extern int		validcount;

extern int		linecount;
extern int		loopcount;


//
// Lighting LUT.
// Used for z-depth cuing per column/row,
//  and other lighting effects (sector ambient, flash).
//

// Lighting constants.
// Now why not 32 levels here?
#define LIGHTLEVELS	        32
#define LIGHTLEVEL_DIV          (256 / LIGHTLEVELS)

#define MAXLIGHTSCALE		48
#define MAXLIGHTZ	       1024

extern lighttable_t*	scalelight[LIGHTLEVELS][MAXLIGHTSCALE];
extern lighttable_t*	scalelightfixed[MAXLIGHTSCALE];
extern lighttable_t*	zlight[LIGHTLEVELS][MAXLIGHTZ];

extern int		extralight;
extern lighttable_t*	fixedcolormap;


// Number of diminishing brightness levels.
// There a 0-31, i.e. 32 LUT in the COLORMAP lump.
#define NUMCOLORMAPS		32

//
// Function pointers to switch refresh/drawing functions.
// Used to select shadow mode etc.
//
extern void		(*colfunc) (void);
extern void		(*basecolfunc) (void);
extern void		(*fuzzcolfunc) (void);

int
RR_PointOnSegSide(
    double x,
    double y,
    Seg* line );

double RR_ScaleFromGlobalAngle(const Angle& visangle);

SubSector*
RR_PointInSubsector(const Vertex& v);
int R_ViewAngleToX(const Angle& a);

//
// REFRESH - the actual rendering functions.
//

// Called by G_Drawer.
void R_RenderPlayerView (player_t *player);

// Called by startup code.
void R_Init (void);

#endif
//-----------------------------------------------------------------------------
//
// $Log:$
//
//-----------------------------------------------------------------------------
