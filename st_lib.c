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
//	The status bar widget code.
//
//-----------------------------------------------------------------------------


static const char
rcsid[] = "$Id: st_lib.c,v 1.4 1997/02/03 16:47:56 b1 Exp $";

#include <ctype.h>

#include "doomdef.h"

#include "z_zone.h"
#include "v_video.h"

#include "m_swap.h"

#include "i_system.h"

#include "w_wad.h"

#include "st_stuff.h"
#include "st_lib.h"
#include "r_local.h"
#include "ImageScaler.hh"


// in AM_map.c
extern bool		automapactive; 




//
// Hack display negative frags.
//  Loads and store the stminus lump.
//
patch_t*		sttminus;

void STlib_init(void)
{
    sttminus = (patch_t *) W_CacheLumpName("STTMINUS", PU_STATIC);
}


// ?
void
STlib_initNum
( st_number_t*		n,
  int			x,
  int			y,
  patch_t**		pl,
  int*			num,
  bool*		on,
  int			width )
{
    n->x	= x;
    n->y	= y;
    n->oldnum	= 0;
    n->width	= width;
    n->num	= num;
    n->on	= on;
    n->p	= pl;
}

void
STlib_drawNum(st_number_t* n,
	      ImageScaler& imageScaler)
{
    int		numdigits = n->width;
    int		num = *n->num;    
    int		w = SHORT(n->p[0]->width);

    n->oldnum = *n->num;

    int neg = num < 0;

    if (neg)
    {
	if (numdigits == 2 && num < -9)
	    num = -9;
	else if (numdigits == 3 && num < -99)
	    num = -99;
	
	num = -num;
    }

    // if non-number, do not draw it
    if (num == 1994)
	return;

    int x = n->x;

    // in the special case of 0, you draw 0
    if (!num)
	imageScaler.drawPatch(x - w, n->y, n->p[0]);

    // draw the new number
    while (num && numdigits--)
    {
	x -= w;
	imageScaler.drawPatch(x, n->y, n->p[num % 10]);
	num /= 10;
    }

    // draw a minus sign if necessary
    if (neg)
	imageScaler.drawPatch(x - 8, n->y, sttminus);
}


void
STlib_updateNum(
    st_number_t* n,
    ImageScaler& imageScaler)
{
    if (*n->on) STlib_drawNum(n, imageScaler);
}

//
void
STlib_initPercent
( st_percent_t*		p,
  int			x,
  int			y,
  patch_t**		pl,
  int*			num,
  bool*		on,
  patch_t*		percent )
{
    STlib_initNum(&p->n, x, y, pl, num, on, 3);
    p->p = percent;
}




void
STlib_updatePercent(st_percent_t* per,
    ImageScaler& imageScaler)
{
    if (*per->n.on)
	imageScaler.drawPatch(per->n.x, per->n.y, per->p);
    
    STlib_updateNum(&per->n, imageScaler);
}



void
STlib_initMultIcon
( st_multicon_t*	i,
  int			x,
  int			y,
  patch_t**		il,
  int*			inum,
  bool*		on )
{
    i->x	= x;
    i->y	= y;
    i->oldinum 	= -1;
    i->inum	= inum;
    i->on	= on;
    i->p	= il;
}



void
STlib_updateMultIcon(
    st_multicon_t* mi,
    ImageScaler& imageScaler)
{
    if (*mi->on
        && (*mi->inum!=-1))
    {
        imageScaler.drawPatch(mi->x, mi->y, mi->p[*mi->inum]);
        mi->oldinum = *mi->inum;
    }
}



void
STlib_initBinIcon
( st_binicon_t*		b,
  int			x,
  int			y,
  patch_t*		i,
  bool*		val,
  bool*		on )
{
    b->x	= x;
    b->y	= y;
    b->oldval	= 0;
    b->val	= val;
    b->on	= on;
    b->p	= i;
}

void
STlib_updateBinIcon(
    st_binicon_t* bi,
    ImageScaler& imageScaler)
{
    if (*bi->on)
    {
	if (*bi->val)
	    imageScaler.drawPatch(bi->x, bi->y, bi->p);
	bi->oldval = *bi->val;
    }
}

