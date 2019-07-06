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
//	Refresh of things, i.e. objects represented by sprites.
//
//-----------------------------------------------------------------------------


static const char
rcsid[] = "$Id: r_things.c,v 1.5 1997/02/03 16:47:56 b1 Exp $";

#include <iostream>
#include <string>
#include <map>

#include <stdio.h>
#include <stdlib.h>
#include <float.h>


#include "doomdef.h"
#include "m_swap.h"

#include "i_system.h"
#include "z_zone.h"
#include "w_wad.h"

#include "r_local.h"

#include "doomstat.h"
#include "Sector.hh"
#include "Picture.hh"


#define MINZ				4
#define BASEYCENTER			SCREENHEIGHT/2
#define BASEXCENTER			SCREENWIDTH/2

//
// Sprite rotation 0 is facing the viewer,
//  rotation 1 is one angle turn CLOCKWISE around the axis.
// This is not the same as the angle,
//  which increases counter clockwise (protractor).
// There was a lot of stuff grabbed wrong, so I changed it...
//
lighttable_t**	spritelights;

// constant arrays
//  used for psprite clipping and initializing clipping
short		negonearray[SCREENWIDTH];
short		screenheightarray[SCREENWIDTH];


//
// INITIALIZATION FUNCTIONS
//

// variables used to look up
//  and range check thing_t sprites patches
std::vector<AnimatedSprite*>  sprites;

//
// R_InitSpriteDefs
// Pass a null terminated list of sprite names
//  (4 chars exactly) to be used.
// Builds the sprite rotation matrixes to account
//  for horizontally flipped sprites.
// Will report an error if the lumps are inconsistant. 
// Only called at startup.
//
// Sprite lump names are 4 characters for the actor,
//  a letter for the frame, and a number for the rotation.
// A sprite that is flippable will have an additional
//  letter/number appended.
// The rotation character can be 0 to signify no rotations.
//
void R_InitSpriteDefs (const std::vector<std::string>& names) 
{
    int		l;
    int		rotation;
    int		start;
    int		end;
    int		patched;

    if (names.empty()) {
	return;
    }
    	
    start = firstspritelump-1;
    end = lastspritelump+1;
	
    // scan all the lump names for each of the names,
    //  noting the highest frame letter.
    for (size_t i = 0; i < names.size(); i++) {
	AnimatedSprite* sprite = new AnimatedSprite();
	const std::string& name = names[i];
	// scan the lumps,
	//  filling in the frames for whatever is found
	for (l=start+1 ; l<end ; l++)
	{
	    // compare four first letters
	    if (name.compare(0, 8, lumpinfo[l].name, 4) == 0) {
		int frame = lumpinfo[l].name[4] - 'A';
		rotation = lumpinfo[l].name[5] - '0';
		
		if (modifiedgame)
		    patched = W_GetNumForName (lumpinfo[l].name);
		else
		    patched = l;
		
		sprite->installLump(name, patched, frame, rotation, false);
		
		if (lumpinfo[l].name[6]) {
		    frame = lumpinfo[l].name[6] - 'A';
		    rotation = lumpinfo[l].name[7] - '0';
		    sprite->installLump(name, l, frame, rotation, true);
		}
	    }
	}

	size_t nFrames = sprite->_frames.size();
	// check the frames that were found for completeness
	if (nFrames == 0) {
	    std::cout << "no frames" << std::endl;
	    continue;
	}
	sprite->validate(name);
	sprites.push_back(sprite);
    }
}




//
// GAME FUNCTIONS
//
std::multimap<double, Sprite*> sortedSprites;

//
// R_InitSprites
// Called at program start.
//
void
R_InitSprites(const std::vector<std::string>& names)
{
    int		i;
	
    for (i=0 ; i<SCREENWIDTH ; i++)
    {
        negonearray[i] = -1;
    }
	
    R_InitSpriteDefs(names);
}



//
// R_ClearSprites
// Called at frame start.
//
void R_ClearSprites (void)
{
    for (std::pair<double, Sprite*> e : sortedSprites) {
	delete e.second;
    }
    sortedSprites.clear();
}

//
// R_DrawMaskedColumn
// Used for sprites and masked mid textures.
// Masked means: partly transparent, i.e. stored
//  in posts/runs of opaque pixels.
//
short*		mfloorclip;
short*		mceilingclip;

double		spryscale;
double		sprtopscreen;

void R_DrawMaskedColumn (column_t* column)
{
    double topscreen;
    double bottomscreen;
	
    double basetexturemid = dc_texturemid;
	
    for ( ; column->topdelta != 0xff ; ) 
    {
	// calculate unclipped screen coordinates
	//  for post
	topscreen = sprtopscreen + spryscale*column->topdelta;
	bottomscreen = topscreen + spryscale*column->length;

	dc_yl = topscreen;
	dc_yh = bottomscreen;

	if (dc_yh >= mfloorclip[dc_x])
	    dc_yh = mfloorclip[dc_x]-1;
	if (dc_yl <= mceilingclip[dc_x])
	    dc_yl = mceilingclip[dc_x]+1;

	if (dc_yl <= dc_yh)
	{
	    dc_source = (byte *)column + 3;
	    dc_texturemid = basetexturemid - column->topdelta;
	    // dc_source = (byte *)column + 3 - column->topdelta;

	    // Drawn by either R_DrawColumn
	    //  or (SHADOW) R_DrawFuzzColumn.
	    colfunc ();	
	}
	column = (column_t *)(  (byte *)column + column->length + 4);
    }
	
    dc_texturemid = basetexturemid;
}



//
// R_DrawVisSprite
//  mfloorclip and mceilingclip should also be set.
//
void
R_DrawVisSprite
( Sprite*		vis,
  int			x1,
  int			x2 )
{
    column_t*		column;
    int			texturecolumn;
    patch_t*		patch;
	
	
    patch = vis->picture->getData();

    dc_colormap = vis->colormap;
    
    if (!dc_colormap)
    {
	// NULL colormap = shadow draw
	colfunc = fuzzcolfunc;
    }
    else if (vis->mobjflags & MF_TRANSLATION)
    {
	colfunc = R_DrawTranslatedColumn;
	dc_translation = translationtables - 256 +
	    ( (vis->mobjflags & MF_TRANSLATION) >> (MF_TRANSSHIFT-8) );
    }
	
    dc_iscale = fabs(vis->xxiscale);
    dc_texturemid = vis->ttexturemid;
    double frac = vis->sstartfrac;
    spryscale = vis->sscale;
    sprtopscreen = centery - (dc_texturemid * spryscale);
    for (dc_x=vis->x1 ; dc_x<=vis->x2 ; dc_x++, frac += vis->xxiscale)
    {
	texturecolumn = frac;
#ifdef RANGECHECK
	if (texturecolumn < 0 || texturecolumn >= SHORT(patch->width)) {
            printf(" texturecolumn %d width %d\n", texturecolumn, SHORT(patch->width));
	    I_Error ("R_DrawSpriteRange: bad texturecolumn");
    }
#endif
	column = (column_t *) ((byte *)patch +
			       LONG(patch->columnofs[texturecolumn]));
	R_DrawMaskedColumn (column);
    }

    colfunc = basecolfunc;
}



//
// R_ProjectSprite
// Generates a vissprite for a thing
//  if it might be visible.
//
void R_ProjectSprite (Mob* thing)
{
    AnimatedSprite*	sprdef;
    
    bool		flip;
    
    int			index;

    Sprite*	vis;
    
    // transform the origin point
    double tr_x = thing->position.getX() - view.getX();
    double tr_y = thing->position.getY() - view.getY();
	
    double gxt = tr_x * vviewcos; 
    double gyt = -(tr_y * vviewsin);
    
    double tz = gxt - gyt; 

    // thing is behind view plane?
    if (tz < MINZ)
	return;
    
    double xscale = pprojection / tz;
	
    gxt = -(tr_x * vviewsin); 
    gyt = tr_y * vviewcos; 
    double tx = -(gyt+gxt); 

    // too far off the side?
    if (fabs(tx)>(tz * 4))
	return;
    
    // decide which patch to use for sprite relative to player
#ifdef RANGECHECK
    if (thing->sprite >= sprites.size())
	I_Error ("R_ProjectSprite: invalid sprite number %i ",
		 thing->sprite);
#endif
    sprdef = sprites[thing->sprite];
#ifdef RANGECHECK
    if ( (thing->frame&FF_FRAMEMASK) >= sprdef->_frames.size() )
	I_Error ("R_ProjectSprite: invalid sprite frame %i : %i ",
		 thing->sprite, thing->frame);
#endif
    const SpriteFrame* sprframe = sprdef->_frames[thing->frame & FF_FRAMEMASK];
    Picture* picture;
    if (*(sprframe->getRotate()))
    {
        Angle ang(view, thing->position);
	double magic((Angle::A45/2)*9);
	int direction = (((double)Angle(ang - thing->_angle + magic)) / Angle::A45);
	
	picture = sprframe->pictures[direction];
	flip = (bool)sprframe->flip[direction];
    }
    else
    {
	// use single rotation for all views
	picture = sprframe->pictures[0];
	flip = (bool)sprframe->flip[0];
    }
    
    // calculate edges of the shape
    tx -= picture->getLeftOffset();	
    int x1 = centerx + (tx * xscale);

    // off the right side?
    if (x1 > SCREENWIDTH)
	return;
    
    tx += picture->getWidth();
    int x2 = int(centerx + (tx * xscale)) - 1;

    // off the left side
    if (x2 < 0)
	return;
    
    // store information in a vissprite
    vis = new Sprite();
    vis->mobjflags = thing->flags;
    vis->sscale = xscale;
    vis->ggx = thing->position.getX();
    vis->ggy = thing->position.getY();
    vis->ggz = thing->zz;
    vis->ggzt = thing->zz + picture->getTopOffset();;
    vis->ttexturemid = vis->ggzt - vviewz;
    vis->x1 = x1 < 0 ? 0 : x1;
    vis->x2 = x2 >= SCREENWIDTH ? SCREENWIDTH-1 : x2;	
    double iscale = 1 / xscale;

    if (flip)
    {
	vis->sstartfrac = picture->getWidth() - 1;
	vis->xxiscale = -iscale;
    }
    else
    {
	vis->sstartfrac = 0;
	vis->xxiscale = iscale;
    }

    if (vis->x1 > x1)
	vis->sstartfrac += vis->xxiscale * (vis->x1-x1);
    vis->picture = picture;
    
    // get light level
    if (thing->flags & MF_SHADOW)
    {
	// shadow draw
	vis->colormap = NULL;
    }
    else if (fixedcolormap)
    {
	// fixed map
	vis->colormap = fixedcolormap;
    }
    else if (thing->frame & FF_FULLBRIGHT)
    {
	// full bright
	vis->colormap = colormaps;
    }
    else
    {
	// diminished light
	index = xscale * DOUBLE_LIGHT_SCALE_MUL;

	if (index >= MAXLIGHTSCALE) 
	    index = MAXLIGHTSCALE-1;

	vis->colormap = spritelights[index];
    }
    sortedSprites.insert(std::make_pair(vis->sscale, vis));
}

//
// R_AddSprites
// During BSP traversal, this adds sprites by sector.
//
void R_AddSprites (Sector* sec)
{
    Mob*		thing;
    int			lightnum;

    // BSP is traversed by subsector.
    // A sector might have been split into several
    //  subsectors during BSP building.
    // Thus we check whether its already added.
    if (sec->validcount == validcount)
	return;		

    // Well, now it will be done.
    sec->validcount = validcount;
	
    lightnum = (sec->lightlevel >> LIGHTSEGSHIFT)+extralight;

    if (lightnum < 0)		
	spritelights = scalelight[0];
    else if (lightnum >= LIGHTLEVELS)
	spritelights = scalelight[LIGHTLEVELS-1];
    else
	spritelights = scalelight[lightnum];

    // Handle all things in sector.
    for (thing = sec->thinglist ; thing ; thing = thing->snext)
	R_ProjectSprite (thing);
}



// @todo a lot magic numbers when it comes to drawing weapons
#define WEAPONSCALE 1.5
//
// R_DrawPSprite
//
void R_DrawPSprite (pspdef_t* psp)
{
    AnimatedSprite*	sprdef;
    Sprite*	vis;
    Sprite		avis;
    
    // decide which patch to use
#ifdef RANGECHECK
    if (psp->state->sprite >= sprites.size())
	I_Error ("R_ProjectSprite: invalid sprite number %i ",
		 psp->state->sprite);
#endif
    sprdef = sprites[psp->state->sprite];
#ifdef RANGECHECK
    if ( (psp->state->frame & FF_FRAMEMASK)  >= sprdef->_frames.size())
	I_Error ("R_ProjectSprite: invalid sprite frame %i : %i ",
		 psp->state->sprite, psp->state->frame);
#endif
    const SpriteFrame* sprframe = sprdef->_frames[ psp->state->frame & FF_FRAMEMASK ];

    Picture* picture = sprframe->pictures[0];
        
    // store information in a vissprite
    vis = &avis;
    vis->mobjflags = 0;
    int weaponoffset = psp->ssy - 1.3 * picture->getTopOffset();
    vis->ttexturemid = BASEYCENTER - weaponoffset;

    double tx = psp->ssx - WEAPONSCALE * 160;
    tx -= WEAPONSCALE * picture->getLeftOffset();
    vis->x1 = centerx + tx;
    tx += WEAPONSCALE * picture->getWidth();
    vis->x2 = int(centerx + tx) - 1;
    
    vis->sscale = WEAPONSCALE;
    vis->xxiscale = 1/vis->sscale;
    vis->sstartfrac = 0;
    vis->picture = picture;

    if (viewplayer->powers[pw_invisibility] > 4*32
	|| viewplayer->powers[pw_invisibility] & 8)
    {
	// shadow draw
	vis->colormap = NULL;
    }
    else if (fixedcolormap)
    {
	// fixed color
	vis->colormap = fixedcolormap;
    }
    else if (psp->state->frame & FF_FULLBRIGHT)
    {
	// full bright
	vis->colormap = colormaps;
    }
    else
    {
	// local light
	vis->colormap = spritelights[MAXLIGHTSCALE-1];
    }
	
    R_DrawVisSprite (vis, vis->x1, vis->x2);
}



//
// R_DrawPlayerSprites
//
void R_DrawPlayerSprites (void)
{
    int		i;
    int		lightnum;
    pspdef_t*	psp;
    
    // get light level
    lightnum =
	(viewplayer->mo->subsector->sector->lightlevel >> LIGHTSEGSHIFT) 
	+extralight;

    if (lightnum < 0)		
	spritelights = scalelight[0];
    else if (lightnum >= LIGHTLEVELS)
	spritelights = scalelight[LIGHTLEVELS-1];
    else
	spritelights = scalelight[lightnum];
    
    // clip to screen bounds
    mfloorclip = screenheightarray;
    mceilingclip = negonearray;
    
    // add all active psprites
    for (i=0, psp=viewplayer->psprites;
	 i<NUMPSPRITES;
	 i++,psp++)
    {
	if (psp->state)
	    R_DrawPSprite (psp);
    }
}

//
// R_DrawSprite
//
void R_DrawSprite(Sprite* spr)
{
    drawseg_t*		ds;
    short		clipbot[SCREENWIDTH];
    short		cliptop[SCREENWIDTH];
    int			x;
    int			r1;
    int			r2;
    int			silhouette;
		
    for (x = spr->x1 ; x<=spr->x2 ; x++)
	clipbot[x] = cliptop[x] = -2;
    
    // Scan drawsegs from end to start for obscuring segs.
    // The first drawseg that has a greater scale
    //  is the clip seg.
    for (ds=ds_p-1 ; ds >= drawsegs ; ds--)
    {
	// determine if the drawseg obscures the sprite
	if (ds->x1 > spr->x2
	    || ds->x2 < spr->x1
	    || (!ds->silhouette
		&& !ds->maskedtexturecol) )
	{
	    // does not cover sprite
	    continue;
	}
			
	r1 = ds->x1 < spr->x1 ? spr->x1 : ds->x1;
	r2 = ds->x2 > spr->x2 ? spr->x2 : ds->x2;

	double lowscale;
	double scale;
	if (ds->sscale1 > ds->sscale2)
	{
	    lowscale = ds->sscale2;
	    scale = ds->sscale1;
	}
	else
	{
	    lowscale = ds->sscale1;
	    scale = ds->sscale2;
	}

	if (scale < spr->sscale ||
	    ((lowscale < spr->sscale) &&
	     !RR_PointOnSegSide(spr->ggx, spr->ggy, ds->curline)))
	{
	    // masked mid texture?
	    if (ds->maskedtexturecol)	
		R_RenderMaskedSegRange (ds, r1, r2);
	    // seg is behind sprite
	    continue;			
	}

	
	// clip this piece of the sprite
	silhouette = ds->silhouette;
	
	if (spr->ggz >= ds->bbsilheight)
	    silhouette &= ~SIL_BOTTOM;

	if (spr->ggzt <= ds->ttsilheight)
	    silhouette &= ~SIL_TOP;
			
	if (silhouette == 1)
	{
	    // bottom sil
	    for (x=r1 ; x<=r2 ; x++)
		if (clipbot[x] == -2)
		    clipbot[x] = ds->sprbottomclip[x];
	}
	else if (silhouette == 2)
	{
	    // top sil
	    for (x=r1 ; x<=r2 ; x++)
		if (cliptop[x] == -2)
		    cliptop[x] = ds->sprtopclip[x];
	}
	else if (silhouette == 3)
	{
	    // both
	    for (x=r1 ; x<=r2 ; x++)
	    {
		if (clipbot[x] == -2)
		    clipbot[x] = ds->sprbottomclip[x];
		if (cliptop[x] == -2)
		    cliptop[x] = ds->sprtopclip[x];
	    }
	}
		
    }
    
    // all clipping has been performed, so draw the sprite

    // check for unclipped columns
    for (x = spr->x1 ; x<=spr->x2 ; x++)
    {
	if (clipbot[x] == -2)		
	    clipbot[x] = SCREENHEIGHT;

	if (cliptop[x] == -2)
	    cliptop[x] = -1;
    }
		
    mfloorclip = clipbot;
    mceilingclip = cliptop;
    R_DrawVisSprite (spr, spr->x1, spr->x2);
}




//
// R_DrawMasked
//
void R_DrawMasked (void)
{
    drawseg_t*		ds;

    for (std::pair<double, Sprite*> e : sortedSprites) {
	R_DrawSprite(e.second);
    }
    
    // render any remaining masked mid textures
    for (ds=ds_p-1 ; ds >= drawsegs ; ds--)
	if (ds->maskedtexturecol)
	    R_RenderMaskedSegRange (ds, ds->x1, ds->x2);
    
    // draw the psprites on top of everything
    R_DrawPlayerSprites ();
}



