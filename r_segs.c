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
//	All the clipping: columns, horizontal spans, sky columns.
//
//-----------------------------------------------------------------------------


static const char
rcsid[] = "$Id: r_segs.c,v 1.3 1997/01/29 20:10:19 b1 Exp $";


#include <stdlib.h>
#include <limits>
#include <iostream>

#include "i_system.h"

#include "doomdef.h"
#include "doomstat.h"

#include "r_local.h"
#include "r_sky.h"
#include "Sector.hh"
#include "Seg.hh"


// OPTIMIZE: closed two sided lines as single sided

// True if any of the segs textures might be visible.
bool		segtextured;	

// False if the back side is the same plane.
bool		markfloor;	
bool		markceiling;

bool		maskedtexture;
int		toptexture;
int		bottomtexture;
int		midtexture;


Angle		rrw_normalangle;
// angle to line origin
Angle		rrw_angle1;	

//
// regular wall
//
int		rw_x;
int		rw_stopx;
Angle		rrw_centerangle;
double		rw_offset;
double		rw_distance;
double		rw_scale;
double		rw_scalestep;
double		rw_midtexturemid;
double		rw_toptexturemid;
double		rw_bottomtexturemid;

double		worldtop;
double		worldbottom;
double		worldhigh;
double		worldlow;

double		pixhigh;
double		pixlow;
double		pixhighstep;
double		pixlowstep;

double		topfrac;
double		topstep;

double		bottomfrac;
double		bottomstep;


lighttable_t**	walllights;

short*		maskedtexturecol;



//
// R_RenderMaskedSegRange
//
void
R_RenderMaskedSegRange
( drawseg_t*	ds,
  int		x1,
  int		x2 )
{
    unsigned	index;
    column_t*	col;
    int		lightnum;
    int		texnum;
    
    // Calculate light table.
    // Use different light tables
    //   for horizontal / vertical / diagonal. Diagonal?
    // OPTIMIZE: get rid of LIGHTSEGSHIFT globally
    curline = ds->curline;
    frontsector = curline->frontsector;
    backsector = curline->backsector;
    texnum = texturetranslation[curline->sidedef->midtexture];
	
    lightnum = (frontsector->lightlevel >> LIGHTSEGSHIFT)+extralight;

    if (curline->v1->getY() == curline->v2->getY())
	lightnum--;
    else if (curline->v1->getX() == curline->v2->getX())
	lightnum++;

    if (lightnum < 0)		
	walllights = scalelight[0];
    else if (lightnum >= LIGHTLEVELS)
	walllights = scalelight[LIGHTLEVELS-1];
    else
	walllights = scalelight[lightnum];

    maskedtexturecol = ds->maskedtexturecol;

    rw_scalestep = ds->sscalestep;		
    spryscale = ds->sscale1 + (x1 - ds->x1) * rw_scalestep;
    mfloorclip = ds->sprbottomclip;
    mceilingclip = ds->sprtopclip;
    
    // find positioning
    if (curline->linedef->flags & ML_DONTPEGBOTTOM)
    {
	dc_texturemid = frontsector->ffloorheight > backsector->ffloorheight
	    ? frontsector->ffloorheight : backsector->ffloorheight;
	dc_texturemid = dc_texturemid + ttextureheight[texnum] - vviewz;
    }
    else
    {
	dc_texturemid = frontsector->cceilingheight < backsector->cceilingheight
	    ? frontsector->cceilingheight : backsector->cceilingheight;
	dc_texturemid = dc_texturemid - vviewz;
    }
    dc_texturemid += curline->sidedef->rrowoffset;
			
    if (fixedcolormap)
	dc_colormap = fixedcolormap;

    // draw the columns
    for (dc_x = x1 ; dc_x <= x2 ; dc_x++)
    {
	// calculate lighting
	if (maskedtexturecol[dc_x] != SHRT_MAX)
	{
	    if (!fixedcolormap)
	    {
		index = spryscale * DOUBLE_LIGHT_SCALE_MUL;

		if (index >=  MAXLIGHTSCALE )
		    index = MAXLIGHTSCALE-1;

		dc_colormap = walllights[index];
	    }
			
	    sprtopscreen = centery - (dc_texturemid * spryscale);
	    dc_iscale = 1 / spryscale;
	    // draw the texture
	    col = (column_t *)( 
		(byte *)R_GetColumn(texnum,maskedtexturecol[dc_x]) -3);
			
	    R_DrawMaskedColumn (col);
	    maskedtexturecol[dc_x] = SHRT_MAX;
	}
	spryscale += rw_scalestep;
    }
	
}




//
// R_RenderSegLoop
// Draws zero, one, or two textures (and possibly a masked
//  texture) for walls.
// Can draw or mark the starting pixel of floor and ceiling
//  textures.
// CALLED: CORE LOOPING ROUTINE.
//
#define HEIGHTMUL 16 //@todo why do this

void R_RenderSegLoop (void)
{
    unsigned		index;
    int			yl;
    int			yh;
    int			mid;
    double		texturecolumn;
    int			top;
    int			bottom;

    for ( ; rw_x < rw_stopx ; rw_x++)
    {
	// mark floor / ceiling areas
	yl = topfrac * HEIGHTMUL;

	// no space above wall?
	if (yl < ceilingclip[rw_x]+1)
	    yl = ceilingclip[rw_x]+1;
	
	if (markceiling)
	{
	    top = ceilingclip[rw_x]+1;
	    bottom = yl-1;

	    if (bottom >= floorclip[rw_x])
		bottom = floorclip[rw_x]-1;

	    if (top <= bottom)
	    {
		ceilingplane->top[rw_x] = top;
		ceilingplane->bottom[rw_x] = bottom;
	    }
	}
		
	yh = bottomfrac * HEIGHTMUL;

	if (yh >= floorclip[rw_x])
	    yh = floorclip[rw_x]-1;

	if (markfloor)
	{
	    top = yh+1;
	    bottom = floorclip[rw_x]-1;
	    if (top <= ceilingclip[rw_x])
		top = ceilingclip[rw_x]+1;
	    if (top <= bottom)
	    {
		floorplane->top[rw_x] = top;
		floorplane->bottom[rw_x] = bottom;
	    }
	}
	
	// texturecolumn and lighting are independent of wall tiers
	if (segtextured)
	{
	    // calculate texture offset

	    Angle angle = rrw_centerangle + xxtoviewangle[rw_x];
	    // @todo figure out why you need to deduct 90 degrees below
	    texturecolumn = rw_offset - tan((double)angle - Angle::A90) * rw_distance;
	    // calculate lighting
	    index =  rw_scale * DOUBLE_LIGHT_SCALE_MUL;
	    
	    if (index >=  MAXLIGHTSCALE )
	      index = MAXLIGHTSCALE-1;
	    
	    dc_colormap = walllights[index];
	    dc_x = rw_x;
	    dc_iscale = 1 / rw_scale;
	}
	
	// draw the wall tiers
	if (midtexture)
	{
	    // single sided line
	    dc_yl = yl;
	    dc_yh = yh;
	    dc_texturemid = rw_midtexturemid;
	    dc_source = R_GetColumn(midtexture,texturecolumn);
	    colfunc ();
	    ceilingclip[rw_x] = SCREENHEIGHT;
	    floorclip[rw_x] = -1;
	}
	else
	{
	    // two sided line
	    if (toptexture)
	    {
		// top wall
		mid = pixhigh * HEIGHTMUL;
		pixhigh += pixhighstep;

		if (mid >= floorclip[rw_x])
		    mid = floorclip[rw_x]-1;

		if (mid >= yl)
		{
		    dc_yl = yl;
		    dc_yh = mid;
		    dc_texturemid = rw_toptexturemid;
		    dc_source = R_GetColumn(toptexture,texturecolumn);
		    colfunc ();
		    ceilingclip[rw_x] = mid;
		}
		else
		    ceilingclip[rw_x] = yl-1;
	    }
	    else
	    {
		// no top wall
		if (markceiling)
		    ceilingclip[rw_x] = yl-1;
	    }
			
	    if (bottomtexture)
	    {
		// bottom wall
		mid = pixlow * HEIGHTMUL;
		pixlow += pixlowstep;

		// no space above wall?
		if (mid <= ceilingclip[rw_x])
		    mid = ceilingclip[rw_x]+1;
		
		if (mid <= yh)
		{
		    dc_yl = mid;
		    dc_yh = yh;
		    dc_texturemid = rw_bottomtexturemid;
		    dc_source = R_GetColumn(bottomtexture,
					    texturecolumn);
		    colfunc ();
		    floorclip[rw_x] = mid;
		}
		else
		    floorclip[rw_x] = yh+1;
	    }
	    else
	    {
		// no bottom wall
		if (markfloor)
		    floorclip[rw_x] = yh+1;
	    }
			
	    if (maskedtexture)
	    {
		// save texturecol
		//  for backdrawing of masked mid texture
		maskedtexturecol[rw_x] = texturecolumn;
	    }
	}
		
	rw_scale += rw_scalestep;
	topfrac += topstep;
	bottomfrac += bottomstep;
    }
    // @todo
}




//
// R_StoreWallRange
// A wall segment will be drawn
//  between start and stop pixels (inclusive).
//
void
R_StoreWallRange
( int	start,
  int	stop )
{
    double		vtop;
    int			lightnum;

    // don't overflow and crash
    if (ds_p == &drawsegs[MAXDRAWSEGS])
	return;		
		
#ifdef RANGECHECK
    if (start >=SCREENWIDTH || start > stop)
	I_Error ("Bad R_RenderWallRange: %i to %i", start , stop);
#endif
    
    sidedef = curline->sidedef;
    linedef = curline->linedef;

    // mark the segment as visible for auto map
    linedef->flags |= ML_MAPPED;
    
    // calculate rw_distance for scale calculation
    rrw_normalangle = (double)curline->aangle + Angle::A90;
    Angle offsetangle((double)rrw_normalangle - (double)rrw_angle1);

    if (offsetangle > Angle(Angle::A180)) {
	offsetangle = Angle(Angle::A360 - (double)offsetangle);
    }
    
    if (offsetangle > Angle(Angle::A90))
	offsetangle = Angle::A90;

    Angle distangle = Angle(Angle::A90) - offsetangle;
    double hyp = curline->v1->distance(view);
    double sineval = sin(distangle);
    rw_distance = hyp * sineval;
		
    ds_p->x1 = rw_x = start;
    ds_p->x2 = stop;
    ds_p->curline = curline;
    rw_stopx = stop+1;
    
    // calculate scale at both ends and step
    ds_p->sscale1 = RR_ScaleFromGlobalAngle(vviewangle + xxtoviewangle[start]);
    rw_scale = RR_ScaleFromGlobalAngle(vviewangle + xxtoviewangle[start]);
    
    if (stop > start )
    {
	ds_p->sscale2 = RR_ScaleFromGlobalAngle(vviewangle + xxtoviewangle[stop]);
	ds_p->sscalestep = (ds_p->sscale2 - rw_scale) / (stop - start);
	rw_scalestep = (ds_p->sscale2 - rw_scale) / (stop - start);
    }
    else
    {
	ds_p->sscale2 = ds_p->sscale1;
    }
    
    // calculate texture boundaries
    //  and decide if floor / ceiling marks are needed
    worldtop = frontsector->cceilingheight - vviewz;
    worldbottom = frontsector->ffloorheight - vviewz;
	
    midtexture = toptexture = bottomtexture = maskedtexture = 0;
    ds_p->maskedtexturecol = NULL;
	
    if (!backsector)
    {
	// single sided line
	midtexture = texturetranslation[sidedef->midtexture];
	// a single sided line is terminal, so it must mark ends
	markfloor = markceiling = true;
	if (linedef->flags & ML_DONTPEGBOTTOM)
	{
	    vtop = frontsector->ffloorheight + ttextureheight[sidedef->midtexture];
	    // bottom of texture at bottom
	    rw_midtexturemid = vtop - vviewz;	
	}
	else
	{
	    // top of texture at top
	    rw_midtexturemid = worldtop;
	}
	rw_midtexturemid += sidedef->rrowoffset;

	ds_p->silhouette = SIL_BOTH;
	ds_p->sprtopclip = screenheightarray;
	ds_p->sprbottomclip = negonearray;
	ds_p->bbsilheight = std::numeric_limits<double>::max();
	ds_p->ttsilheight = std::numeric_limits<double>::min();
    }
    else
    {
	// two sided line
	ds_p->sprtopclip = ds_p->sprbottomclip = NULL;
	ds_p->silhouette = 0;
	
	if (frontsector->ffloorheight > backsector->ffloorheight)
	{
	    ds_p->silhouette = SIL_BOTTOM;
	    ds_p->bbsilheight = frontsector->ffloorheight;
	}
	else if (backsector->ffloorheight > vviewz)
	{
	    ds_p->silhouette = SIL_BOTTOM;
	    ds_p->bbsilheight = std::numeric_limits<double>::max();
	    // ds_p->sprbottomclip = negonearray;
	}
	
	if (frontsector->cceilingheight < backsector->cceilingheight)
	{
	    ds_p->silhouette |= SIL_TOP;
	    ds_p->ttsilheight = frontsector->cceilingheight;
	}
	else if (backsector->cceilingheight < vviewz)
	{
	    ds_p->silhouette |= SIL_TOP;
	    ds_p->ttsilheight = std::numeric_limits<double>::min();
	    // ds_p->sprtopclip = screenheightarray;
	}
		
	if (backsector->cceilingheight <= frontsector->ffloorheight)
	{
	    ds_p->sprbottomclip = negonearray;
	    ds_p->bbsilheight = std::numeric_limits<double>::max();
	    ds_p->silhouette |= SIL_BOTTOM;
	}
	
	if (backsector->ffloorheight >= frontsector->cceilingheight)
	{
	    ds_p->sprtopclip = screenheightarray;
	    ds_p->ttsilheight = std::numeric_limits<double>::min();
	    ds_p->silhouette |= SIL_TOP;
	}
	
	worldhigh = backsector->cceilingheight - vviewz;
	worldlow = backsector->ffloorheight - vviewz;
		
	// hack to allow height changes in outdoor areas
	if (frontsector->ceilingpic == skyflatnum 
	    && backsector->ceilingpic == skyflatnum)
	{
	    worldtop = worldhigh;
	}
	
			
	if (worldlow != worldbottom 
	    || backsector->floorpic != frontsector->floorpic
	    || backsector->lightlevel != frontsector->lightlevel)
	{
	    markfloor = true;
	}
	else
	{
	    // same plane on both sides
	    markfloor = false;
	}
	
			
	if (worldhigh != worldtop 
	    || backsector->ceilingpic != frontsector->ceilingpic
	    || backsector->lightlevel != frontsector->lightlevel)
	{
	    markceiling = true;
	}
	else
	{
	    // same plane on both sides
	    markceiling = false;
	}
	
	if (backsector->cceilingheight <= frontsector->ffloorheight
	    || backsector->ffloorheight >= frontsector->cceilingheight)
	{
	    // closed door
	    markceiling = markfloor = true;
	}
	

	if (worldhigh < worldtop)
	{
	    // top texture
	    toptexture = texturetranslation[sidedef->toptexture];
	    if (linedef->flags & ML_DONTPEGTOP)
	    {
		// top of texture at top
		rw_toptexturemid = worldtop;
	    }
	    else
	    {
		vtop = backsector->cceilingheight + ttextureheight[sidedef->toptexture];
		
		// bottom of texture
		rw_toptexturemid = vtop - vviewz;	
	    }
	}
	if (worldlow > worldbottom)
	{
	    // bottom texture
	    bottomtexture = texturetranslation[sidedef->bottomtexture];

	    if (linedef->flags & ML_DONTPEGBOTTOM )
	    {
		// bottom of texture at bottom
		// top of texture at top
		rw_bottomtexturemid = worldtop;
	    }
	    else	// top of texture at top
		rw_bottomtexturemid = worldlow;
	}
	rw_toptexturemid += sidedef->rrowoffset;
	rw_bottomtexturemid += sidedef->rrowoffset;
	
	// allocate space for masked texture tables
	if (sidedef->midtexture)
	{
	    // masked midtexture
	    maskedtexture = true;
	    ds_p->maskedtexturecol = maskedtexturecol = lastopening - rw_x;
	    lastopening += rw_stopx - rw_x;
	}
    }
    
    // calculate rw_offset (only needed for textured lines)
    segtextured = midtexture | toptexture | bottomtexture | maskedtexture;

    if (segtextured)
    {
	offsetangle = rrw_normalangle - rrw_angle1;
	
	if (offsetangle > Angle(Angle::A180))
	    offsetangle = -offsetangle;

	if (offsetangle > Angle(Angle::A90))
	    offsetangle = Angle::A90;

	sineval = sin(offsetangle);
	rw_offset = hyp * sineval;

	if (Angle(rrw_normalangle - rrw_angle1) < Angle(Angle::A180))
	    rw_offset = -rw_offset;

	rw_offset += sidedef->ttextureoffset + curline->ooffset;
	rrw_centerangle = Angle(Angle::A90) + vviewangle - rrw_normalangle;
	// calculate light table
	//  use different light tables
	//  for horizontal / vertical / diagonal
	// OPTIMIZE: get rid of LIGHTSEGSHIFT globally
	if (!fixedcolormap)
	{
	    lightnum = (frontsector->lightlevel >> LIGHTSEGSHIFT)+extralight;

	    if (curline->v1->getY() == curline->v2->getY())
		lightnum--;
	    else if (curline->v1->getX() == curline->v2->getX())
		lightnum++;

	    if (lightnum < 0)		
		walllights = scalelight[0];
	    else if (lightnum >= LIGHTLEVELS)
		walllights = scalelight[LIGHTLEVELS-1];
	    else
		walllights = scalelight[lightnum];
	}
    }
    
    // if a floor / ceiling plane is on the wrong side
    //  of the view plane, it is definitely invisible
    //  and doesn't need to be marked.
    
  
    if (frontsector->ffloorheight >= vviewz)
    {
	// above view plane
	markfloor = false;
    }
    
    if (frontsector->cceilingheight <= vviewz 
	&& frontsector->ceilingpic != skyflatnum)
    {
	// below view plane
	markceiling = false;
    }

    
    // calculate incremental stepping values for texture edges
    worldtop /= 16;
    worldbottom /= 16;
    
    topstep = -(rw_scalestep * worldtop);
    topfrac = (centery / 16.0) - (worldtop * rw_scale);
    
    bottomstep = -(rw_scalestep * worldbottom);
    bottomfrac = (centery / 16.0) - (worldbottom *  rw_scale);
	
    if (backsector)
    {	
	worldhigh /= 16;
	worldlow /= 16;

	if (worldhigh < worldtop)
	{
	    pixhigh = (centery / 16.0) - (worldhigh * rw_scale);
	    pixhighstep = -(rw_scalestep * worldhigh);
	}
	
	if (worldlow > worldbottom)
	{
	    pixlow = (centery / 16.0) - (worldlow * rw_scale);
	    pixlowstep = -(rw_scalestep * worldlow);
	}
    }
    
    // render it
    if (markceiling)
	ceilingplane = R_CheckPlane (ceilingplane, rw_x, rw_stopx-1);
    
    if (markfloor)
	floorplane = R_CheckPlane (floorplane, rw_x, rw_stopx-1);

    R_RenderSegLoop ();

    
    // save sprite clipping info
    if ( ((ds_p->silhouette & SIL_TOP) || maskedtexture)
	 && !ds_p->sprtopclip)
    {
	memcpy (lastopening, ceilingclip+start, 2*(rw_stopx-start));
	ds_p->sprtopclip = lastopening - start;
	lastopening += rw_stopx - start;
    }
    
    if ( ((ds_p->silhouette & SIL_BOTTOM) || maskedtexture)
	 && !ds_p->sprbottomclip)
    {
	memcpy (lastopening, floorclip+start, 2*(rw_stopx-start));
	ds_p->sprbottomclip = lastopening - start;
	lastopening += rw_stopx - start;	
    }

    if (maskedtexture && !(ds_p->silhouette&SIL_TOP))
    {
	ds_p->silhouette |= SIL_TOP;
	ds_p->ttsilheight = std::numeric_limits<double>::min();
    }
    if (maskedtexture && !(ds_p->silhouette&SIL_BOTTOM))
    {
	ds_p->silhouette |= SIL_BOTTOM;
	ds_p->bbsilheight = std::numeric_limits<double>::max();
    }
    ds_p++;
}

