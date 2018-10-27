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
//	Movement/collision utility functions,
//	as used by function in p_map.c. 
//	BLOCKMAP Iterator functions,
//	and some PIT_* functions to use for iteration.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: p_maputl.c,v 1.5 1997/02/03 22:45:11 b1 Exp $";

#include <math.h>
#include <stdlib.h>

#include <limits>

#include "m_bbox.h"

#include "doomdef.h"
#include "p_local.h"


// State.
#include "r_state.h"

//
// P_AproxDistance
// Gives an estimation of distance (not exact)
//

double
PP_AproxDistance(
    double dx,
    double dy)
{
    return sqrt(dx*dx + dy*dy);
}


//
// P_PointOnLineSide
// Returns 0 or 1
//
int
P_PointOnLineSide(
    double x,
    double y,
    line_t* line)
{
    double dx;
    double dy;
    double left;
    double right;

    if (line->ddx == 0)
    {
        if (x <= line->v1->xx)
            return line->ddy > 0;
        
        return line->ddy < 0;
    }

    if (line->ddy == 0)
    {
        if (y <= line->v1->yy)
            return line->ddx < 0;
        
        return line->ddx > 0;
    }
	
    dx = (x - line->v1->xx);
    dy = (y - line->v1->yy);
	
    left = line->ddy * dx;
    right = dy *  line->ddx;
	
    if (right < left)
	return 0;		// front side
    return 1;			// back side
}



//
// P_BoxOnLineSide
// Considers the line to be infinite
// Returns side 0 or 1, -1 if box crosses the line.
//
int
PP_BoxOnLineSide(
    double* tmbox,
    line_t*	ld)
{
    int		p1;
    int		p2;
	
    switch (ld->slopetype)
    {
    case ST_HORIZONTAL:
        p1 = tmbox[BOXTOP] > ld->v1->yy;
        p2 = tmbox[BOXBOTTOM] > ld->v1->yy;
        if (ld->ddx < 0)
        {
            p1 ^= 1;
            p2 ^= 1;
        }
        break;
	
    case ST_VERTICAL:
        p1 = tmbox[BOXRIGHT] < ld->v1->xx;
        p2 = tmbox[BOXLEFT] < ld->v1->xx;
        if (ld->ddy < 0)
        {
            p1 ^= 1;
            p2 ^= 1;
        }
        break;
        
    case ST_POSITIVE:
        p1 = P_PointOnLineSide(tmbox[BOXLEFT], tmbox[BOXTOP], ld);
        p2 = P_PointOnLineSide(tmbox[BOXRIGHT], tmbox[BOXBOTTOM], ld);
	break;
	
    case ST_NEGATIVE:
        p1 = P_PointOnLineSide(tmbox[BOXRIGHT], tmbox[BOXTOP], ld);
        p2 = P_PointOnLineSide(tmbox[BOXLEFT], tmbox[BOXBOTTOM], ld);
        break;
    }
    
    if (p1 == p2)
        return p1;
    return -1;
}

//
// P_MakeDivline
//
void
P_MakeDivline
( line_t*	li,
  DivLine*	dl )
{
    dl->x = li->v1->xx;
    dl->y = li->v1->yy;
    dl->dx = li->ddx;
    dl->dy = li->ddy;
}

//
// P_LineOpening
// Sets opentop and openbottom to the window
// through a two sided line.
// OPTIMIZE: keep this precalculated
//
double oopentop;
double oopenbottom;
double oopenrange;
double llowfloor;


void P_LineOpening (line_t* linedef)
{
    sector_t*	front;
    sector_t*	back;
	
    if (linedef->sidenum[1] == -1)
    {
        // single sided line
        oopenrange = 0;
        return;
    }
	 
    front = linedef->frontsector;
    back = linedef->backsector;
	
    if (front->cceilingheight < back->cceilingheight)
        oopentop = front->cceilingheight;
    else
        oopentop = back->cceilingheight;
    
    if (front->ffloorheight > back->ffloorheight)
    {
        oopenbottom = front->ffloorheight;
        llowfloor = back->ffloorheight;
    }
    else
    {
        oopenbottom = back->ffloorheight;
        llowfloor = front->ffloorheight;
    }
	
    oopenrange = oopentop - oopenbottom;
}


//
// THING POSITION SETTING
//


//
// P_UnsetThingPosition
// Unlinks a thing from block map and sectors.
// On each position change, BLOCKMAP and other
// lookups maintaining lists ot things inside
// these structures need to be updated.
//
void P_UnsetThingPosition (Mob* thing)
{
    int		blockx;
    int		blocky;

    if ( ! (thing->flags & MF_NOSECTOR) )
    {
	// inert things don't need to be in blockmap?
	// unlink from subsector
	if (thing->snext)
	    thing->snext->sprev = thing->sprev;

	if (thing->sprev)
	    thing->sprev->snext = thing->snext;
	else
	    thing->subsector->sector->thinglist = thing->snext;
    }
	
    if ( ! (thing->flags & MF_NOBLOCKMAP) )
    {
	// inert things don't need to be in blockmap
	// unlink from block map
	if (thing->bnext)
	    thing->bnext->bprev = thing->bprev;
	
	if (thing->bprev)
	    thing->bprev->bnext = thing->bnext;
	else
	{
	    blockx = double_to_fixed(thing->xx - blockMap.oorgx)>>MAPBLOCKSHIFT;
	    blocky = double_to_fixed(thing->yy - blockMap.oorgy)>>MAPBLOCKSHIFT;

	    if (blockx>=0 && blockx < blockMap.width
		&& blocky>=0 && blocky <blockMap.height)
	    {
		blockMap.blocklinks[blocky*blockMap.width+blockx] = thing->bnext;
	    }
	}
    }
}


//
// P_SetThingPosition
// Links a thing into both a block and a subsector
// based on it's x y.
// Sets thing->subsector properly
//
void
P_SetThingPosition (Mob* thing)
{
    subsector_t*	ss;
    sector_t*		sec;
    int			blockx;
    int			blocky;
    Mob**		link;
    
    
    // link into subsector
    ss = RR_PointInSubsector(thing->xx, thing->yy);
    thing->subsector = ss;
    
    if ( ! (thing->flags & MF_NOSECTOR) )
    {
        // invisible things don't go into the sector links
        sec = ss->sector;
	
        thing->sprev = NULL;
        thing->snext = sec->thinglist;
        
        if (sec->thinglist)
            sec->thinglist->sprev = thing;
        
        sec->thinglist = thing;
    }
    
    
    // link into blockmap
    if ( ! (thing->flags & MF_NOBLOCKMAP) )
    {
        // inert things don't need to be in blockmap		
        blockx = double_to_fixed(thing->xx - blockMap.oorgx)>>MAPBLOCKSHIFT;
        blocky = double_to_fixed(thing->yy - blockMap.oorgy)>>MAPBLOCKSHIFT;

	if (blockx>=0
	    && blockx < blockMap.width
	    && blocky>=0
	    && blocky < blockMap.height)
	{
	    link = &blockMap.blocklinks[blocky*blockMap.width+blockx];

	    thing->bprev = NULL;
	    thing->bnext = *link;
	    if (*link)
            (*link)->bprev = thing;

	    *link = thing;
	}
	else
	{
	    // thing is off the map
	    thing->bnext = thing->bprev = NULL;
	}
    }
}



//
// BLOCK MAP ITERATORS
// For each line/thing in the given mapblock,
// call the passed PIT_* function.
// If the function returns false,
// exit with false without checking anything else.
//


//
// P_BlockLinesIterator
// The validcount flags are used to avoid checking lines
// that are marked in multiple mapblocks,
// so increment validcount before the first call
// to P_BlockLinesIterator, then make one or more calls
// to it.
//
bool
P_BlockLinesIterator
( int			x,
  int			y,
  bool(*func)(line_t*) )
{
    int			offset;
    short*		list;
    line_t*		ld;
	
    if (x<0
        || y<0
        || x>=blockMap.width
        || y>=blockMap.height)
    {
        return true;
    }
    
    offset = y*blockMap.width+x;
	
    offset = *(blockMap.blockmap+offset);
    
    for ( list = blockMap.lump+offset ; *list != -1 ; list++)
    {
        ld = &lines[*list];
        
        if (ld->validcount == validcount)
            continue; 	// line has already been checked
        
        ld->validcount = validcount;
		
        if ( !func(ld) )
            return false;
    }
    return true;	// everything was checked
}


//
// P_BlockThingsIterator
//
bool
P_BlockThingsIterator
( int			x,
  int			y,
  bool(*func)(Mob*) )
{
    Mob*		mobj;
	
    if ( x<0
	 || y<0
	 || x>=blockMap.width
	 || y>=blockMap.height)
    {
	return true;
    }
    

    for (mobj = blockMap.blocklinks[y*blockMap.width+x] ;
         mobj ;
         mobj = mobj->bnext)
    {
        if (!func( mobj ) )
            return false;
    }
    return true;
}



//
// INTERCEPT ROUTINES
//
intercept_t	intercepts[MAXINTERCEPTS];
intercept_t*	intercept_p;

DivLine trace;
bool 	earlyout;
int		ptflags;

//
// PIT_AddLineIntercepts.
// Looks for lines in the given block
// that intercept the given trace
// to add to the intercepts list.
//
// A line is crossed if its endpoints
// are on opposite sides of the trace.
// Returns true if earlyout and a solid line hit.
//
bool
PIT_AddLineIntercepts (line_t* ld)
{
    int			s1;
    int			s2;
    double		frac;
    DivLine		dl;
	
    // avoid precision problems with two routines
    if (trace.dx > 16 ||        // @todo  these 16 are strange
        trace.dy > 16 || 
        trace.dx < -16 || 
        trace.dy < -16)
    {
        s1 = trace.pointOnSide(ld->v1->xx, ld->v1->yy);
        s2 = trace.pointOnSide(ld->v2->xx, ld->v2->yy);
    }
    else
    {
        s1 = P_PointOnLineSide(trace.x, trace.y, ld);
        s2 = P_PointOnLineSide(trace.x+trace.dx, trace.y+trace.dy, ld);
    }
    
    if (s1 == s2)
	return true;	// line isn't crossed
    
    // hit the line
    P_MakeDivline (ld, &dl);
    frac = trace.interceptVector(dl);

    if (frac < 0)
        return true;	// behind source
	
    // try to early out the check
    if (earlyout
        && frac < 1
        && !ld->backsector)
    {
        return false;	// stop checking
    }
    
	
    intercept_p->fraction = frac;
    intercept_p->isaline = true;
    intercept_p->d.line = ld;
    intercept_p++;

    return true;	// continue
}



//
// PIT_AddThingIntercepts
//
bool PIT_AddThingIntercepts (Mob* thing)
{
    double x1;
    double y1;
    double x2;
    double y2;
    
    bool tracepositive = (trace.dx * trace.dy) > 0;
	
    // check a corner to corner crossection for hit
    if (tracepositive)
    {
        x1 = thing->xx - thing->rradius;
        y1 = thing->yy + thing->rradius;
		
        x2 = thing->xx + thing->rradius;
        y2 = thing->yy - thing->rradius;			
    }
    else
    {
        x1 = thing->xx - thing->rradius;
        y1 = thing->yy - thing->rradius;
		
        x2 = thing->xx + thing->rradius;
        y2 = thing->yy + thing->rradius;			
    }
    
    int s1 = trace.pointOnSide(x1, y1);
    int s2 = trace.pointOnSide(x2, y2);
    
    if (s1 == s2)
        return true;		// line isn't crossed

    DivLine		dl;
    dl.x = x1;
    dl.y = y1;
    dl.dx = x2-x1;
    dl.dy = y2-y1;

    
    double frac = trace.interceptVector(dl);
    
    if (frac < 0)
        return true;		// behind source

    intercept_p->fraction = frac;
    intercept_p->isaline = false;
    intercept_p->d.thing = thing;
    intercept_p++;

    return true;		// keep going
}


//
// P_TraverseIntercepts
// Returns true if the traverser function returns true
// for all lines.
//
bool
PP_TraverseIntercepts(traverser_t func,
		      double maxfrac)
{
    double dist;
    intercept_t* scan;
    intercept_t* in = 0;	
    int count = intercept_p - intercepts;
    while (count--) {
        dist = std::numeric_limits<double>::max();
        for (scan = intercepts; scan < intercept_p; scan++) {
            if (scan->fraction < dist) {
                dist = scan->fraction;
                in = scan;
            }
        }
        if (dist > maxfrac)
            return true;	// checked everything in range		
        if (!func (in))
            return false;	// don't bother going farther
        
        in->fraction = std::numeric_limits<double>::max();
    }
    return true;		// everything was traversed
}

//
// P_PathTraverse
// Traces a line from x1,y1 to x2,y2,
// calling the traverser function for each.
// Returns true if the traverser function returns true
// for all lines.
//
bool
P_PathTraverse
( fixed_t		x1,
  fixed_t		y1,
  fixed_t		x2,
  fixed_t		y2,
  int			flags,
  bool (*trav) (intercept_t *))
{
    fixed_t	xt1;
    fixed_t	yt1;
    fixed_t	xt2;
    fixed_t	yt2;
    
    fixed_t	xstep;
    fixed_t	ystep;
    
    fixed_t	partial;
    
    fixed_t	xintercept;
    fixed_t	yintercept;
    
    int		mapx;
    int		mapy;
    
    int		mapxstep;
    int		mapystep;

    int		count;
		
    earlyout = flags & PT_EARLYOUT;
		
    validcount++;
    intercept_p = intercepts;
	
    if ( ((x1-double_to_fixed(blockMap.oorgx))&(MAPBLOCKSIZE-1)) == 0)
    {
        x1 += FRACUNIT;	// don't side exactly on a line
    }
    
    if ( ((y1-double_to_fixed(blockMap.oorgy))&(MAPBLOCKSIZE-1)) == 0)
	y1 += FRACUNIT;	// don't side exactly on a line

    trace.x = fixed_to_double(x1);
    trace.y = fixed_to_double(y1);
    trace.dx = fixed_to_double(x2 - x1);
    trace.dy = fixed_to_double(y2 - y1);

    x1 -= double_to_fixed(blockMap.oorgx);
    y1 -= double_to_fixed(blockMap.oorgy);
    xt1 = x1>>MAPBLOCKSHIFT;
    yt1 = y1>>MAPBLOCKSHIFT;

    x2 -= double_to_fixed(blockMap.oorgx);
    y2 -= double_to_fixed(blockMap.oorgy);
    xt2 = x2>>MAPBLOCKSHIFT;
    yt2 = y2>>MAPBLOCKSHIFT;

    if (xt2 > xt1)
    {
        mapxstep = 1;
        partial = FRACUNIT - ((x1>>MAPBTOFRAC)&(FRACUNIT-1));
        ystep = FixedDiv (y2-y1,abs(x2-x1));
    }
    else if (xt2 < xt1)
    {
        mapxstep = -1;
        partial = (x1>>MAPBTOFRAC)&(FRACUNIT-1);
        ystep = FixedDiv (y2-y1,abs(x2-x1));
    }
    else
    {
        mapxstep = 0;
        partial = FRACUNIT;
        ystep = 256*FRACUNIT;
    }	

    yintercept = (y1>>MAPBTOFRAC) + FixedMul (partial, ystep);

	
    if (yt2 > yt1)
    {
        mapystep = 1;
        partial = FRACUNIT - ((y1>>MAPBTOFRAC)&(FRACUNIT-1));
        xstep = FixedDiv (x2-x1,abs(y2-y1));
    }
    else if (yt2 < yt1)
    {
        mapystep = -1;
        partial = (y1>>MAPBTOFRAC)&(FRACUNIT-1);
        xstep = FixedDiv (x2-x1,abs(y2-y1));
    }
    else
    {
        mapystep = 0;
        partial = FRACUNIT;
        xstep = 256*FRACUNIT;
    }	
    xintercept = (x1>>MAPBTOFRAC) + FixedMul (partial, xstep);
    
    // Step through map blocks.
    // Count is present to prevent a round off error
    // from skipping the break.
    mapx = xt1;
    mapy = yt1;
	
    for (count = 0 ; count < 64 ; count++)
    {
        if (flags & PT_ADDLINES)
        {
            if (!P_BlockLinesIterator (mapx, mapy,PIT_AddLineIntercepts))
                return false;	// early out
        }
        
        if (flags & PT_ADDTHINGS)
        {
            if (!P_BlockThingsIterator (mapx, mapy,PIT_AddThingIntercepts))
                return false;	// early out
        }
		
        if (mapx == xt2 && mapy == yt2)
        {
            break;
        }
        
        if ( (yintercept >> FRACBITS) == mapy)
        {
            yintercept += ystep;
            mapx += mapxstep;
        }
        else if ( (xintercept >> FRACBITS) == mapx)
        {
            xintercept += xstep;
            mapy += mapystep;
        }
    }
    // go through the sorted list
    return PP_TraverseIntercepts (trav, 1);
}


