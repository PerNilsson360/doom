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

#include <cmath>
#include <math.h>
#include <stdlib.h>

#include <limits>

#include "m_bbox.h"

#include "doomdef.h"
#include "p_local.h"


// State.
#include "r_state.h"
#include "Sector.hh"

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
    const Vertex& v,
    line_t* line)
{
    double x = v.getX();
    double y = v.getY();
    double left;
    double right;

    if (line->ddx == 0)
    {
        if (x <= line->v1->getX())
            return line->ddy > 0;
        
        return line->ddy < 0;
    }

    if (line->ddy == 0)
    {
        if (y <= line->v1->getY())
            return line->ddx < 0;
        
        return line->ddx > 0;
    }
	
    double dx = (x - line->v1->getX());
    double dy = (y - line->v1->getY());
	
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
        p1 = tmbox[BOXTOP] > ld->v1->getY();
        p2 = tmbox[BOXBOTTOM] > ld->v1->getY();
        if (ld->ddx < 0)
        {
            p1 ^= 1;
            p2 ^= 1;
        }
        break;
	
    case ST_VERTICAL:
        p1 = tmbox[BOXRIGHT] < ld->v1->getX();
        p2 = tmbox[BOXLEFT] < ld->v1->getX();
        if (ld->ddy < 0)
        {
            p1 ^= 1;
            p2 ^= 1;
        }
        break;
        
    case ST_POSITIVE:
        p1 = P_PointOnLineSide(Vertex(tmbox[BOXLEFT], tmbox[BOXTOP]), ld);
        p2 = P_PointOnLineSide(Vertex(tmbox[BOXRIGHT], tmbox[BOXBOTTOM]), ld);
	break;
	
    case ST_NEGATIVE:
        p1 = P_PointOnLineSide(Vertex(tmbox[BOXRIGHT], tmbox[BOXTOP]), ld);
        p2 = P_PointOnLineSide(Vertex(tmbox[BOXLEFT], tmbox[BOXBOTTOM]), ld);
        break;
    }
    
    if (p1 == p2)
        return p1;
    return -1;
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
    if (linedef->sidenum[1] == -1)
    {
        // single sided line
        oopenrange = 0;
        return;
    }
	 
    Sector* front = linedef->frontsector;
    Sector* back = linedef->backsector;
	
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
	    blockx = (thing->position.getX() - blockMap.oorgx) / DOUBLE_MAPBLOCKS_DIV;
	    blocky = (thing->position.getY() - blockMap.oorgy) / DOUBLE_MAPBLOCKS_DIV;

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
    Sector*		sec;
    int			blockx;
    int			blocky;
    Mob**		link;
    
    
    // link into subsector
    ss = RR_PointInSubsector(thing->position);
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
        // insert things don't need to be in blockmap		
        blockx = (thing->position.getX() - blockMap.oorgx) / DOUBLE_MAPBLOCKS_DIV;
        blocky = (thing->position.getY() - blockMap.oorgy) / DOUBLE_MAPBLOCKS_DIV;

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
    // @todo  these 16 are strange
    if (trace.isChangeBiggerThan(16))
    {
        s1 = trace.pointOnSide(*(ld->v1));
        s2 = trace.pointOnSide(*(ld->v2));
    }
    else
    {
        s1 = P_PointOnLineSide(trace.getVertex(0), ld);
        s2 = P_PointOnLineSide(trace.getVertex(1), ld);
    }
    
    if (s1 == s2)
	return true;	// line isn't crossed
    
    // hit the line
    dl = DivLine(*ld);
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
    double x = thing->position.getX();
    double y = thing->position.getY();
    double x1;
    double y1;
    double x2;
    double y2;
    
    // check a corner to corner crossection for hit
    if (trace.isPositive())
    {
        x1 = x - thing->rradius;
        y1 = y + thing->rradius;
		
        x2 = x + thing->rradius;
        y2 = y - thing->rradius;			
    }
    else
    {
        x1 = x - thing->rradius;
        y1 = y - thing->rradius;
		
        x2 = x + thing->rradius;
        y2 = y + thing->rradius;			
    }
    
    int s1 = trace.pointOnSide(x1, y1);
    int s2 = trace.pointOnSide(x2, y2);
    
    if (s1 == s2)
        return true;		// line isn't crossed

    DivLine dl(x1, x2, y1, y2);
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
P_PathTraverse(const Vertex& v1,
	       const Vertex& v2,
	       int flags,
	       bool (*trav) (intercept_t *))
{

    double x1 = v1.getX();
    double y1 = v1.getY();
    double x2 = v2.getX();
    double y2 = v2.getY();
    int		mapx;
    int		mapy;
    
    int		mapxstep;
    int		mapystep;

    int		count;
		
    earlyout = flags & PT_EARLYOUT;
		
    validcount++;
    intercept_p = intercepts;
    
    double intPart;
    if (modf((x1 - blockMap.oorgx) / DOUBLE_MAPBLOCKS_DIV, &intPart) == 0)
    {
	x1 += 1;
    }
    
    if (modf((y1 - blockMap.oorgy) / DOUBLE_MAPBLOCKS_DIV, &intPart) == 0)
    {
	y1 += 1;
    }

    trace = DivLine(x1, x2, y1, y2);

    x1 -= blockMap.oorgx;
    y1 -= blockMap.oorgy;
    double xt1 = x1 / DOUBLE_MAPBLOCKS_DIV;
    double yt1 = y1 / DOUBLE_MAPBLOCKS_DIV;

    x2 -= blockMap.oorgx;
    y2 -= blockMap.oorgy;
    double xt2 = x2 / DOUBLE_MAPBLOCKS_DIV;
    double yt2 = y2 / DOUBLE_MAPBLOCKS_DIV;

    double partial;
    double ystep;
    if (xt2 > xt1)
    {
        mapxstep = 1;
	partial = 1 - modf(x1 / DOUBLE_MAPBLOCKS_DIV, &intPart);
	ystep = (y2-y1)/fabs(x2-x1);
    }
    else if (xt2 < xt1)
    {
        mapxstep = -1;
	partial = modf(x1 / DOUBLE_MAPBLOCKS_DIV, &intPart);
	ystep = (y2-y1)/fabs(x2-x1);
    }
    else
    {
        mapxstep = 0;
        partial = 1;
        ystep = 256;
    }	

    double yintercept = (y1 / DOUBLE_MAPBLOCKS_DIV) + (partial * ystep);

    double xstep;
    if (yt2 > yt1)
    {
        mapystep = 1;
	partial = 1 - modf(y1 / DOUBLE_MAPBLOCKS_DIV, &intPart);
	xstep = (x2-x1)/fabs(y2-y1);
    }
    else if (yt2 < yt1)
    {
        mapystep = -1;
	partial = modf(y1 / DOUBLE_MAPBLOCKS_DIV, &intPart);
	xstep = (x2-x1)/fabs(y2-y1);
    }
    else
    {
        mapystep = 0;
        partial = 1;
        xstep = 256;
    }	
    double xintercept = (x1 / DOUBLE_MAPBLOCKS_DIV) + (partial * xstep);
    
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
        
        if (int(yintercept) == mapy)
        {
            yintercept += ystep;
            mapx += mapxstep;
        }
        else if (int(xintercept) == mapx)
        {
            xintercept += xstep;
            mapy += mapystep;
        }
    }
    // go through the sorted list
    return PP_TraverseIntercepts (trav, 1);
}



