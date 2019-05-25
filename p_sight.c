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
//	LineOfSight/Visibility checks, uses REJECT Lookup Table.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: p_sight.c,v 1.3 1997/01/28 22:08:28 b1 Exp $";


#include "doomdef.h"

#include "i_system.h"
#include "p_local.h"

// State.
#include "r_state.h"

//
// P_CheckSight
//
double ssightzstart;		// eye z of looker
double ttopslope;
double bbottomslope;		// slopes to top and bottom of target

DivLine	strace;			// from t1 to t2
double tt2x;
double tt2y;

int		sightcounts[2];

//
// P_CrossSubsector
// Returns true
//  if strace crosses the given subsector successfully.
//
bool P_CrossSubsector (int num)
{
    seg_t*		seg;
    line_t*		line;
    int			s1;
    int			s2;
    int			count;
    subsector_t*	sub;
    sector_t*		front;
    sector_t*		back;
    double opentop;
    double openbottom;
    DivLine		divl;
    vertex_t*		v1;
    vertex_t*		v2;
	
#ifdef RANGECHECK
    if (num>=numsubsectors)
	I_Error ("P_CrossSubsector: ss %i with numss = %i",
		 num,
		 numsubsectors);
#endif

    sub = &subsectors[num];
    
    // check lines
    count = sub->numlines;
    seg = &segs[sub->firstline];

    for ( ; count ; seg++, count--)
    {
	line = seg->linedef;

	// allready checked other side?
	if (line->validcount == validcount)
	    continue;
	
	line->validcount = validcount;
		
	v1 = line->v1;
	v2 = line->v2;
	s1 = strace.pointOnSide(v1->xx, v1->yy);
	s2 = strace.pointOnSide(v2->xx, v2->yy);

	// line isn't crossed?
	if (s1 == s2)
	    continue;
	
	divl = DivLine(v1->xx, v2->xx, v1->yy, v2->yy);

	s1 = divl.pointOnSide(strace.moveX(0), strace.moveY(0));
	s2 = divl.pointOnSide(tt2x, tt2y);

	// line isn't crossed?
	if (s1 == s2)
	    continue;	

	// stop because it is not two sided anyway
	// might do this after updating validcount?
	if ( !(line->flags & ML_TWOSIDED) )
	    return false;
	
	// crosses a two sided line
	front = seg->frontsector;
	back = seg->backsector;

	// no wall to block sight with?
	if (front->ffloorheight == back->ffloorheight
	    && front->cceilingheight == back->cceilingheight)
	    continue;	

	// possible occluder
	// because of ceiling height differences
	if (front->cceilingheight < back->cceilingheight)
	    opentop = front->cceilingheight;
	else
	    opentop = back->cceilingheight;

	// because of ceiling height differences
	if (front->ffloorheight > back->ffloorheight)
	    openbottom = front->ffloorheight;
	else
	    openbottom = back->ffloorheight;
		
	// quick test for totally closed doors
	if (openbottom >= opentop)	
	    return false;		// stop
	
	double frac = strace.interceptVector(divl);
		
	if (front->ffloorheight != back->ffloorheight)
	{
	    double slope = (openbottom - ssightzstart) / frac;
            if (slope > bbottomslope)
		bbottomslope = slope;
	}
		
	if (front->cceilingheight != back->cceilingheight)
	{
	    double slope = (opentop - ssightzstart) / frac;
	    if (slope < ttopslope)
		ttopslope = slope;
	}
		
	if (ttopslope <= bbottomslope)
	    return false;		// stop				
    }
    // passed the subsector ok
    return true;		
}



//
// P_CrossBSPNode
// Returns true
//  if strace crosses the given node successfully.
//
bool P_CrossBSPNode (int bspnum)
{
    BspNode* bsp;
    int		side;
    
    if (bspnum & NF_SUBSECTOR)
    {
        if (bspnum == -1)
            return P_CrossSubsector (0);
        else
            return P_CrossSubsector (bspnum&(~NF_SUBSECTOR));
    }
	
    bsp = &nodes[bspnum];
    
    // decide which side the start point is on
    side = bsp->pointOnSide (strace.moveX(0), strace.moveY(0));
    if (side == 2)
        side = 0;	// an "on" should cross both sides
    
    // cross the starting side
    if (!P_CrossBSPNode (bsp->children[side]) )
        return false;
	
    // the partition plane is crossed here
    if (side == bsp->pointOnSide (tt2x, tt2y))
    {
        // the line doesn't touch the other side
        return true;
    }
    
    // cross the ending side		
    return P_CrossBSPNode (bsp->children[side^1]);
}


//
// P_CheckSight
// Returns true
//  if a straight line between t1 and t2 is unobstructed.
// Uses REJECT.
//
bool
P_CheckSight
( Mob*	t1,
  Mob*	t2 )
{
    int		s1;
    int		s2;
    int		pnum;
    int		bytenum;
    int		bitnum;
    
    // First check for trivial rejection.

    // Determine subsector entries in REJECT table.
    s1 = (t1->subsector->sector - sectors);
    s2 = (t2->subsector->sector - sectors);
    pnum = s1*numsectors + s2;
    bytenum = pnum>>3;
    bitnum = 1 << (pnum&7);

    // Check in REJECT table.
    if (rejectmatrix[bytenum]&bitnum)
    {
        sightcounts[0]++;
        
        // can't possibly be connected
        return false;	
    }

    // An unobstructed LOS is possible.
    // Now look from eyes of t1 to any part of t2.
    sightcounts[1]++;

    validcount++;
	
    ssightzstart = t1->zz + t1->hheight - (t1->hheight / 4);
    ttopslope = t2->zz + t2->hheight - ssightzstart;
    bbottomslope = t2->zz - ssightzstart;
	
    strace = DivLine(t1->xx, t2->xx, t1->yy, t2->yy);
    tt2x = t2->xx;
    tt2y = t2->yy;

    // the head node is the last node output
    return P_CrossBSPNode (numnodes-1);	
}


