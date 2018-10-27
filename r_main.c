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


#include "doomdef.h"
#include "d_net.h"

#include "m_bbox.h"

#include "r_local.h"
#include "r_sky.h"
#include "d_player.h"
#include "Angle.hh"



// Fineangles in the SCREENWIDTH wide window.
#define FIELDOFVIEW		2048 //2048	
namespace {
    const Angle ANGLEOFVIEW(Angle::A90);
}

// increment every time a check is made
int			validcount = 1;		


lighttable_t*		fixedcolormap;
extern lighttable_t**	walllights;

int			centerx;
int			centery;

double			ccenterxfrac;
double			ccenteryfrac;
double			pprojection;

// just for profiling purposes
int			framecount;	

int			sscount;
int			linecount;
int			loopcount;

double vviewx;
double vviewy;
double vviewz;

Angle vviewangle;

double vviewcos;
double vviewsin;

player_t*		viewplayer;

//
// precalculated math tables
//
Angle cclipangle;

// The viewangletox[viewangle + FINEANGLES/4] lookup
// maps the visible view angles to screen X coordinates,
// flattening the arc to a flat projection plane.
// There will be many angles mapped to the same X. 
int			viewangletox[FINEANGLES/2];

// The xtoviewangleangle[] table maps a screen pixel
// to the lowest viewangle that maps back to x ranges
// from clipangle to -clipangle.
Angle xxtoviewangle[SCREENWIDTH+1];


// UNUSED.
// The finetangentgent[angle+FINEANGLES/4] table
// holds the fixed_t tangent values for view angles,
// ranging from MININT to 0 to MAXINT.
// fixed_t		finetangent[FINEANGLES/2];

// fixed_t		finesine[5*FINEANGLES/4];
//fixed_t*		finecosine = &finesine[FINEANGLES/4];


lighttable_t*		scalelight[LIGHTLEVELS][MAXLIGHTSCALE];
lighttable_t*		scalelightfixed[MAXLIGHTSCALE];
lighttable_t*		zlight[LIGHTLEVELS][MAXLIGHTZ];

// bumped light from gun blasts
int			extralight;			



void (*colfunc) (void);
void (*basecolfunc) (void);
void (*fuzzcolfunc) (void);
void (*transcolfunc) (void);
void (*spanfunc) (void);

//
// R_PointOnSide
// Traverse BSP (sub) tree,
//  check point against partition plane.
// Returns side 0 (front) or 1 (back).
//
int
R_PointOnSide( 
    double x,
    double y,
    BspNode* node)
{
    double dx;
    double dy;
    double left;
    double right;
	
    //@todo how does this one work really

    if (!node->dx)
    {
        if (x <= node->x)
            return node->dy > 0;
	
        return node->dy < 0;
    }

    if (!node->dy)
    {
        if (y <= node->y)
            return node->dx < 0;
        
        return node->dx > 0;
    }
	
    //Assuming the points are (Ax,Ay) (Bx,By) and (Cx,Cy), you need to compute:
    // (Bx - Ax) * (Cy - Ay) - (By - Ay) * (Cx - Ax)


    dx = (x - node->x);
    dy = (y - node->y);
	
    left = node->dy * dx;
    right = dy * node->dx;

    int result = 1;

    if (right < left)
    {
        //front side
        result = 0;
    }
    // back side
    return result;			
}


int
RR_PointOnSegSide(double x,
                  double y,
                  seg_t* line)
{
    double lx = line->v1->xx;
    double ly = line->v1->yy;
	
    double ldx = line->v2->xx - lx;
    double ldy = line->v2->yy - ly;
	
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


// //
// // R_PointToAngle
// // To get a global angle from cartesian coordinates,
// //  the coordinates are flipped until they are in
// //  the first octant of the coordinate system, then
// //  the y (<=x) is scaled and divided by x to get a
// //  tangent (slope) value which is looked up in the
// //  tantoangle[] table.
// angle_t
// R_PointToAngle
// ( fixed_t	x,
//   fixed_t	y )
// {	
//     x -= double_to_fixed(vviewx);
//     y -= double_to_fixed(vviewy);
    
//     if ( (!x) && (!y) )
//         return 0;
    
//     if (x>= 0)
//     {
//         // x >=0
//         if (y>= 0)
//         {
//             // y>= 0
            
//             if (x>y)
//             {
//                 // octant 0
//                 return tantoangle[ SlopeDiv(y,x)];
//             }
//             else
//             {
//                 // octant 1
//                 return ANG90-1-tantoangle[ SlopeDiv(x,y)];
//             }
//         }
//         else
//         {
//             // y<0
//             y = -y;
            
//             if (x>y)
//             {
//                 // octant 8
//                 return -tantoangle[SlopeDiv(y,x)];
//             }
//             else
//             {
//                 // octant 7
//                 return ANG270+tantoangle[ SlopeDiv(x,y)];
//             }
//         }
//     }
//     else
//     {
//         // x<0
//         x = -x;
        
//         if (y>= 0)
//         {
//             // y>= 0
//             if (x>y)
//             {
//                 // octant 3
//                 return ANG180-1-tantoangle[ SlopeDiv(y,x)];
//             }
//             else
//             {
//                 // octant 2
//                 return ANG90+ tantoangle[ SlopeDiv(x,y)];
//             }
//         }
//         else
//         {
//             // y<0
//             y = -y;
            
//             if (x>y)
//             {
//                 // octant 4
//                 return ANG180+tantoangle[ SlopeDiv(y,x)];
//             }
//             else
//             {
//                 // octant 5
//                 return ANG270-1-tantoangle[ SlopeDiv(x,y)];
//             }
//         }
//     }
//     return 0;
// }


// angle_t
// R_PointToAngle2
// ( fixed_t	x1,
//   fixed_t	y1,
//   fixed_t	x2,
//   fixed_t	y2 )
// {	
//     viewx = x1;
//     viewy = y1;
    
//     return R_PointToAngle (x2, y2);
// }

// Returns an angle between a point and the view coordinates
double
RR_PointToAngle(double x, double y)
{
    return R_PointsToAngle(x, y,
                           vviewx, vviewy);
}

//returns angle used in polar coordinates
double 
R_PointsToAngle(double x1, double y1, double x2, double y2)
{
    double x = x2 - x1;
    double y = y2 - y1;
    double result;
    if (y >= 0) { 
        if (x == 0) {
            result = Angle::A90;
        } else {
            if (x > 0) {
                result = atan(y/x);
            } else {
                x *= -1;
                result = Angle::A180;
                result -= atan(y/x);
            }
        }
    } else {        
        if (x == 0) {
            result = Angle::A270;
        } else {
            y *= -1;
            if (x < 0) {
                x *= -1;
                result = Angle::A180;
                result += atan(y/x);
            } else {
                result = Angle::A360;
                result -= atan(y/x);
            }
        }
    }
    return result;
}

double
RR_PointToDist(double x, double y)
{
    double dx = x - vviewx;
    double dy = y - vviewy;
    return sqrt((dx * dx) + (dy * dy));
}


//
// R_InitPointToAngle
//
void R_InitPointToAngle (void)
{
    // UNUSED - now getting from tables.c
#if 0
    int	i;
    long	t;
    float	f;
//
// slope (tangent) to angle lookup
//
    for (i=0 ; i<=SLOPERANGE ; i++)
    {
	f = atan( (float)i/SLOPERANGE )/(3.141592657*2);
	t = 0xffffffff*f;
	tantoangle[i] = t;
    }
#endif
}


double
RR_ScaleFromGlobalAngle(Angle visangle)
{
    double scale;

    Angle anglea(Angle::A90 + (visangle - vviewangle));
    Angle angleb(Angle::A90 + (visangle - rrw_normalangle));

    // both sines are allways positive
    double sinea = sin(anglea);	
    double sineb = sin(angleb);
    double num = pprojection * sineb;
    double den = fixed_to_double(rw_distance) * sinea;
    scale = num / den;
    if (scale > 64)
        scale = 64;
    else if (scale < fixed_to_double(256)) {
        // @todo this seems to be a unnessasary if branch
        printf("******** fixing scale *******%f\n", scale);
        scale = fixed_to_double(256);
    }
    return scale;
}


//
// R_InitTables
//
void R_InitTables (void)
{
    // UNUSED: now getting from tables.c
#if 0
    int		i;
    float	a;
    float	fv;
    int		t;
    
    // viewangle tangent table
    for (i=0 ; i<FINEANGLES/2 ; i++)
    {
	a = (i-FINEANGLES/4+0.5)*PI*2/FINEANGLES;
	fv = FRACUNIT*tan (a);
	t = fv;
	finetangent[i] = t;
    }
    
    // finesine table
    for (i=0 ; i<5*FINEANGLES/4 ; i++)
    {
	// OPTIMIZE: mirror...
	a = (i+0.5)*PI*2/FINEANGLES;
	t = FRACUNIT*sin (a);
	finesine[i] = t;
    }
#endif

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
    int			i;
    int			x;
    int			t;
//    fixed_t		focallength;
    
    // Use tangent table to generate viewangletox:
    //  viewangletox will give the next greatest x
    //  after the view angle.
    //
    // Calc focallength
    //  so FIELDOFVIEW angles covers SCREENWIDTH.
    // focallength = FixedDiv (double_to_fixed(ccenterxfrac),
    //     		    finetangent[FINEANGLES/4+FIELDOFVIEW/2] );
    double focallength = SCREENWIDTH / (2 * tan((double)ANGLEOFVIEW/2));
    // printf("focalLength %f %f centerx %f %d\n",
    //        fixed_to_double(focallength), ffocalLength,
    //        ccenterxfrac, centerx);

    for (i=0 ; i<FINEANGLES/2 ; i++)
    {
	if (finetangent[i] > FRACUNIT*2)
	    t = -1;
	else if (finetangent[i] < -FRACUNIT*2)
	    t = SCREENWIDTH+1;
	else
	{
	    t = FixedMul (finetangent[i], double_to_fixed(focallength));
	    t = (double_to_fixed(ccenterxfrac) - t+FRACUNIT-1)>>FRACBITS;

	    if (t < -1)
		t = -1;
	    else if (t>SCREENWIDTH+1)
		t = SCREENWIDTH+1;
	}
	viewangletox[i] = t;
    }
    
    // Scan viewangletox[] to generate xtoviewangle[]:
    //  xtoviewangle will give the smallest view angle
    //  that maps to x.	
    for (x=0;x<=SCREENWIDTH;x++)
    {
	i = 0;
	while (viewangletox[i]>x)
	    i++;
	xxtoviewangle[x] = Angle((static_cast<angle_t>(i)<<ANGLETOFINESHIFT)-ANG90);
    }
    
    // Take out the fencepost cases from viewangletox.
    for (i=0 ; i<FINEANGLES/2 ; i++)
    {
	t = FixedMul (finetangent[i], double_to_fixed(focallength));
	t = centerx - t;
	
	if (viewangletox[i] == -1)
	    viewangletox[i] = 0;
	else if (viewangletox[i] == SCREENWIDTH+1)
	    viewangletox[i] = SCREENWIDTH;
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
    int		scale;
    
    // Calculate the light levels to use
    //  for each level / distance combination.
    for (i=0 ; i< LIGHTLEVELS ; i++)
    {
	startmap = ((LIGHTLEVELS-1-i)*2)*NUMCOLORMAPS/LIGHTLEVELS;
	for (j=0 ; j<MAXLIGHTZ ; j++)
	{
	    scale = FixedDiv ((SCREENWIDTH/2*FRACUNIT), (j+1)<<LIGHTZSHIFT);
	    scale >>= LIGHTSCALESHIFT;
	    level = startmap - scale/DISTMAP;
	    
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
    fixed_t	dy;
    int		i;
    int		j;
    int		level;
    int		startmap; 	

    centery = SCREENHEIGHT/2;
    centerx = SCREENWIDTH/2;
    ccenterxfrac = centerx;
    ccenteryfrac = centery;
    pprojection = ccenterxfrac;
    
    colfunc = basecolfunc = R_DrawColumn;
    fuzzcolfunc = R_DrawFuzzColumn;
    transcolfunc = R_DrawTranslatedColumn;
    spanfunc = R_DrawSpan;
    
    R_InitTextureMapping ();
        
    // thing clipping
    for (i=0 ; i<SCREENWIDTH ; i++)
        screenheightarray[i] = SCREENHEIGHT;
    
    // planes
    for (i=0 ; i<SCREENHEIGHT ; i++)
    {
        dy = ((i-SCREENHEIGHT/2)<<FRACBITS)+FRACUNIT/2;
        dy = abs(dy);
        yslope[i] = FixedDiv (SCREENWIDTH/2*FRACUNIT, dy);
    }
	
    for (i=0 ; i<SCREENWIDTH ; i++)
    {
	double cosadj = abs(sin(xxtoviewangle[i]));
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
    R_InitPointToAngle ();
    printf ("\nR_InitPointToAngle");
    R_InitTables ();
    printf ("\nR_InitTables");
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
subsector_t*
RR_PointInSubsector(double x, double y)
{
    BspNode* node;
    int		side;
    int		nodenum;

    // single subsector is a special case
    if (!numnodes)				
        return subsectors;
    
    nodenum = numnodes-1;
    
    while (! (nodenum & NF_SUBSECTOR) )
    {
        node = &nodes[nodenum];
        side = R_PointOnSide (x, y, node);
        nodenum = node->children[side];
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
    vviewx = player->mo->xx;
    vviewy = player->mo->yy;
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
    R_RenderBSPNode (numnodes-1);
    
    // Check for new console commands.
    NetUpdate ();
    
    R_DrawPlanes ();
    
    // Check for new console commands.
    NetUpdate ();
    
    R_DrawMasked ();

    // Check for new console commands.
    NetUpdate ();				
}
