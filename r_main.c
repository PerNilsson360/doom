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
// $Log:$
//
// DESCRIPTION:
//	Rendering main loop and setup functions,
//	 utility functions (BSP, geometry, trigonometry).
//	See tables.c, too.
//
//-----------------------------------------------------------------------------


static const char rcsid[] = "$Id: r_main.c,v 1.5 1997/02/03 22:45:12 b1 Exp $";



#include <stdlib.h>
#include <math.h>
#include <iostream>

#include "doomdef.h"
#include "d_net.h"
#include "r_local.h"
#include "r_sky.h"
#include "d_player.h"
#include "Angle.hh"
#include "Vertex.hh"
#include "Seg.hh"


namespace {
    const Angle ANGLEOFVIEW(Angle::A90);
}

// increment every time a check is made
int			validcount = 1;		


lighttable_t*		fixedcolormap;
extern lighttable_t**	walllights;

int			centerx;
int			centery;
double			pprojection;

// just for profiling purposes
int			framecount;	

int			sscount;
int			linecount;
int			loopcount;

Vertex view;			// x and y cordinates
double vviewz;

Angle vviewangle;

double vviewcos;
double vviewsin;

player_t*		viewplayer;

//
// precalculated math tables
//
Angle cclipangle;

// The xtoviewangleangle[] table maps a screen pixel
// to the lowest viewangle that maps back to x ranges
// from clipangle to -clipangle.
Angle xxtoviewangle[SCREENWIDTH+1];

lighttable_t*		scalelight[LIGHTLEVELS][MAXLIGHTSCALE];
lighttable_t*		scalelightfixed[MAXLIGHTSCALE];
lighttable_t*		zlight[LIGHTLEVELS][MAXLIGHTZ];

// bumped light from gun blasts
int			extralight;			



void (*colfunc) (void);
void (*basecolfunc) (void);
void (*fuzzcolfunc) (void);
void (*transcolfunc) (void);


int
RR_PointOnSegSide(double x,
                  double y,
                  Seg* line)
{
    double lx = line->v1->getX();
    double ly = line->v1->getY();
	
    double ldx = line->v2->getX() - lx;
    double ldy = line->v2->getY() - ly;
	
    if (ldx == 0.0)
    {
	if (x <= lx)
	    return ldy > 0;
	
	return ldy < 0;
    }
    else if (ldy == 0.0)
    {
	if (y <= ly)
	    return ldx < 0;
	
	return ldx > 0;
    }
	
    double dx = (x - lx);
    double dy = (y - ly);
	
    // // Try to quickly decide by looking at sign bits.
    // if ( (ldy ^ ldx ^ dx ^ dy)&0x80000000 )
    // {
    //     if  ( (ldy ^ dx) & 0x80000000 )
    //     {
    //         // (left is negative)
    //         return 1;
    //     }
    //     return 0;
    // }
    
    // @todo understand why the fractionl part needs
    // be removed?
    double left = static_cast<int>(ldy) * dx ;
    double right = dy  * static_cast<int>(ldx);
	
    if (right < left)
    {
	// front side
	return 0;
    }
    // back side
    return 1;			
}

double
RR_ScaleFromGlobalAngle(const Angle& visangle)
{
    double scale;

    Angle anglea(Angle::A90 + (visangle - vviewangle));
    Angle angleb(Angle::A90 + (visangle - rrw_normalangle));

    // both sines are allways positive
    double sinea = sin(anglea);	
    double sineb = sin(angleb);
    double num = pprojection * sineb;
    double den = rw_distance * sinea;
    scale = num / den;

    if (scale > 64) {
        scale = 64;
    } else if (scale < 0.001) {
        scale = 0.001;
    }
    
    return scale;
}

int
R_ViewAngleToX(const Angle& a)
{
    double focalLength = SCREENWIDTH / (2 * tan(((double)ANGLEOFVIEW)/2));
    double t = tan(a);
    int x;
    if (t > Angle::A90)
        x = 0;
    else if (t < -Angle::A90)
        x = SCREENWIDTH + 1;
    else {
        x = centerx - (t * focalLength) + 1;
        if (x > SCREENWIDTH)
            x = SCREENWIDTH;
        else if (x < 0)
            x = 0;
    }
    return x;
}



//
// R_InitTextureMapping
//
void R_InitTextureMapping (void)
{
    for (int x = 0, y = SCREENWIDTH ; x < SCREENWIDTH/2; x++, y--){
	double a = 0;
	while (R_ViewAngleToX(Angle(a)) > x) {
	    a += 0.0001;
	}
	xxtoviewangle[x] = Angle(a);
	xxtoviewangle[y] = Angle(Angle::A360 - a);
    }
    cclipangle = xxtoviewangle[0];
}

//
// R_InitLightTables
// Only inits the zlight table,
//  because the scalelight table changes with view size.
//
#define DISTMAP		2

void R_InitLightTables (void)
{
    int		i;
    int		j;
    int		level;
    int		startmap; 	
    
    // Calculate the light levels to use
    //  for each level / distance combination.
    for (i=0 ; i< LIGHTLEVELS ; i++)
    {
	startmap = ((LIGHTLEVELS-1-i)*2)*NUMCOLORMAPS/LIGHTLEVELS;
	for (j=0 ; j<MAXLIGHTZ ; j++)
	{
	    double sscale = (SCREENWIDTH/2) / double(((j+1) * DOUBLE_LIGHT_SCALE_MUL));
	    sscale = sscale * DOUBLE_LIGHT_SCALE_MUL;
	    
	    level = startmap - sscale/DISTMAP;
	    
	    if (level < 0)
		level = 0;

	    if (level >= NUMCOLORMAPS)
		level = NUMCOLORMAPS-1;

	    zlight[i][j] = colormaps + level*256;
	}
    }
}

//
// R_ExecuteSetViewSize
//
void R_ExecuteSetViewSize (void)
{
    int		i;
    int		j;
    int		level;
    int		startmap; 	

    centery = SCREENHEIGHT/2;
    centerx = SCREENWIDTH/2;
    pprojection = centerx;
    
    colfunc = basecolfunc = R_DrawColumn;
    fuzzcolfunc = R_DrawFuzzColumn;
    transcolfunc = R_DrawTranslatedColumn;
    
    R_InitTextureMapping ();
        
    // thing clipping
    for (i=0 ; i<SCREENWIDTH ; i++)
        screenheightarray[i] = SCREENHEIGHT;
    
    // planes
    for (i=0 ; i<SCREENHEIGHT ; i++)
    {
	// todo fishy 1 / 2 (integer division always 0??)
        double dy = i - (SCREENHEIGHT / 2) + (1.0 / 2);
        dy = fabs(dy);
        yslope[i] = (SCREENWIDTH / 2) / dy;
    }
	
    for (i=0 ; i<SCREENWIDTH ; i++)
    {
	double cosadj = fabs(cos(xxtoviewangle[i]));
        ddistscale[i] = 1 / cosadj;
    }
    
    // Calculate the light levels to use
    //  for each level / scale combination.
    for (i=0 ; i< LIGHTLEVELS ; i++)
    {
        startmap = ((LIGHTLEVELS-1-i)*2)*NUMCOLORMAPS/LIGHTLEVELS;
        for (j=0 ; j<MAXLIGHTSCALE ; j++)
        {
            level = startmap - j/DISTMAP;
            
            if (level < 0)
                level = 0;
            
            if (level >= NUMCOLORMAPS)
                level = NUMCOLORMAPS-1;
            
            scalelight[i][j] = colormaps + level*256;
        }
    }
}



//
// R_Init
//
void R_Init (void)
{
    R_InitData ();
    printf ("\nR_InitData");
    R_InitPlanes ();
    printf ("\nR_InitPlanes");
    R_InitLightTables ();
    printf ("\nR_InitLightTables");
    R_InitSkyMap ();
    printf ("\nR_InitSkyMap");
    R_InitTranslationTables ();
    printf ("\nR_InitTranslationsTables");
    framecount = 0;
}


//
// R_PointInSubsector
//
SubSector*
RR_PointInSubsector(const Vertex& v)
{
    // single subsector is a special case
    int numnodes = nodes.size();
    if (!numnodes) {
	printf("RR_PointInSubsector: size 0 ?????");
	return &subsectors[0];
    }
    
    int nodenum = numnodes-1;
    
    while (!(nodenum & NF_SUBSECTOR)) {
        const BspNode& node = nodes[nodenum];
        int side = node.pointOnSide(v);
        nodenum = node.getChild(side);
    }
	
    return &subsectors[nodenum & ~NF_SUBSECTOR];
}



//
// R_SetupFrame
//
void R_SetupFrame (player_t* player)
{		
    int		i;
    
    viewplayer = player;
    view = player->mo->position;
    vviewangle = player->mo->_angle;
    extralight = player->extralight;

    vviewz = player->vviewz;
    vviewsin = sin(vviewangle);
    vviewcos = cos(vviewangle);

    sscount = 0;
	
    if (player->fixedcolormap)
    {
	fixedcolormap =
	    colormaps
	    + player->fixedcolormap*256*sizeof(lighttable_t);
	
	walllights = scalelightfixed;

	for (i=0 ; i<MAXLIGHTSCALE ; i++)
	    scalelightfixed[i] = fixedcolormap;
    }
    else
	fixedcolormap = 0;
		
    framecount++;
    validcount++;
}



//
// R_RenderView
//
void R_RenderPlayerView (player_t* player)
{	
    R_SetupFrame (player);

    // Clear buffers.
    R_ClearClipSegs ();
    R_ClearDrawSegs ();
    R_ClearPlanes ();
    R_ClearSprites ();
    
    // check for new console commands.
    NetUpdate ();

    // The head node is the last node output.
    R_RenderBSPNode(nodes.size() - 1);
    
    // Check for new console commands.
    NetUpdate ();
    
    R_DrawPlanes ();
    
    // Check for new console commands.
    NetUpdate ();
    
    R_DrawMasked ();

    // Check for new console commands.
    NetUpdate ();				
}
