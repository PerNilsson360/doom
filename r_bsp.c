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
//	BSP traversal, handling of LineSegs for rendering.
//
//-----------------------------------------------------------------------------


static const char
rcsid[] = "$Id: r_bsp.c,v 1.4 1997/02/03 22:45:12 b1 Exp $";


#include "doomdef.h"

#include "m_bbox.h"

#include "i_system.h"

#include "r_main.h"
#include "r_plane.h"
#include "r_things.h"

// State.
#include "doomstat.h"
#include "r_state.h"
#include "Sector.hh"



seg_t*		curline;
Side*		sidedef;
Line*		linedef;
Sector*	frontsector;
Sector*	backsector;

drawseg_t	drawsegs[MAXDRAWSEGS];
drawseg_t*	ds_p;


void
R_StoreWallRange
( int	start,
  int	stop );




//
// R_ClearDrawSegs
//
void R_ClearDrawSegs (void)
{
    ds_p = drawsegs;
}



//
// ClipWallSegment
// Clips the given range of columns
// and includes it in the new clip list.
//
typedef	struct
{
    int32_t first;
    int32_t last;
    
} cliprange_t;


#define MAXSEGS		32

// newend is one past the last valid seg
cliprange_t*	newend;
cliprange_t	solidsegs[MAXSEGS];




//
// R_ClipSolidWallSegment
// Does handle solid walls,
//  e.g. single sided LineDefs (middle texture)
//  that entirely block the view.
// 
void
R_ClipSolidWallSegment
( int			first,
  int			last )
{
    cliprange_t*	next;
    cliprange_t*	start;

    // Find the first range that touches the range
    //  (adjacent pixels are touching).
    start = solidsegs;
    while (start->last < first-1)
	start++;

    if (first < start->first)
    {
	if (last < start->first-1)
	{
	    // Post is entirely visible (above start),
	    //  so insert a new clippost.
	    R_StoreWallRange (first, last);
	    next = newend;
	    newend++;
	    
	    while (next != start)
	    {
		*next = *(next-1);
		next--;
	    }
	    next->first = first;
	    next->last = last;
	    return;
	}
		
	// There is a fragment above *start.
	R_StoreWallRange (first, start->first - 1);
	// Now adjust the clip size.
	start->first = first;	
    }

    // Bottom contained in start?
    if (last <= start->last)
	return;			
		
    next = start;
    while (last >= (next+1)->first-1)
    {
	// There is a fragment between two posts.
	R_StoreWallRange (next->last + 1, (next+1)->first - 1);
	next++;
	
	if (last <= next->last)
	{
	    // Bottom is contained in next.
	    // Adjust the clip size.
	    start->last = next->last;	
	    goto crunch;
	}
    }
	
    // There is a fragment after *next.
    R_StoreWallRange (next->last + 1, last);
    // Adjust the clip size.
    start->last = last;
	
    // Remove start+1 to next from the clip list,
    // because start now covers their area.
  crunch:
    if (next == start)
    {
	// Post just extended past the bottom of one post.
	return;
    }
    

    while (next++ != newend)
    {
	// Remove a post.
	*++start = *next;
    }

    newend = start+1;
}



//
// R_ClipPassWallSegment
// Clips the given range of columns,
//  but does not includes it in the clip list.
// Does handle windows,
//  e.g. LineDefs with upper and lower texture.
//
void
R_ClipPassWallSegment
( int	first,
  int	last )
{
    cliprange_t*	start;
        
    // Find the first range that touches the range
    //  (adjacent pixels are touching).
    start = solidsegs;
    while (start->last < first-1)
	start++;

    if (first < start->first)
    {
	if (last < start->first-1)
	{
	    // Post is entirely visible (above start).
	    R_StoreWallRange (first, last);
	    return;
	}
		
	// There is a fragment above *start.
	R_StoreWallRange (first, start->first - 1);
    }

    // Bottom contained in start?
    if (last <= start->last)
	return;			
		
    while (last >= (start+1)->first-1)
    {
	// There is a fragment between two posts.
	R_StoreWallRange (start->last + 1, (start+1)->first - 1);
	start++;
	
	if (last <= start->last)
	    return;
    }
	
    // There is a fragment after *next.
    R_StoreWallRange (start->last + 1, last);
}



//
// R_ClearClipSegs
//
void R_ClearClipSegs (void)
{
    solidsegs[0].first = -0x7fffffff;
    solidsegs[0].last = -1;
    solidsegs[1].first = SCREENWIDTH;
    solidsegs[1].last = 0x7fffffff;
    newend = solidsegs+2;
}

//
// R_AddLine
// Clips the given segment
// and adds any visible pieces to the line list.
//
void R_AddLine (seg_t*	line)
{
    curline = line;

    // OPTIMIZE: quickly reject orthogonal back sides.
    Angle angle1(view, *(line->v1));
    Angle angle2(view, *(line->v2));
    // OPTIMIZE: make constant out of 2*clipangle (FIELDOFVIEW).
    Angle span = angle1 - angle2;
    
    // Back side? I.e. backface culling?
    if (span >= Angle(Angle::A180))
        return;

    // Global angle needed by segcalc.
    rrw_angle1 = angle1;
    angle1 -= vviewangle;
    angle2 -= vviewangle;

    Angle tspan = angle1 + cclipangle;
    if (tspan > 2*cclipangle)
    {
        tspan -= 2*cclipangle;

        // Totally off the left edge?
        if (tspan >= span)
            return;
	
        angle1 = cclipangle;
    }
    tspan = cclipangle - angle2;
    if (tspan > 2*cclipangle)
    {
        tspan -= 2*cclipangle;

        // Totally off the left edge?
        if (tspan >= span)
            return;	
        angle2 = -cclipangle;
    }
    
    int x1 = R_ViewAngleToX(angle1);
    int x2 = R_ViewAngleToX(angle2);
    
    // Does not cross a pixel?
    if (x1 == x2)
	return;				
	
    backsector = line->backsector;

    // Single sided line?
    if (!backsector)
	goto clipsolid;		

    // Closed door.
    if (backsector->cceilingheight <= frontsector->ffloorheight
        || backsector->ffloorheight >= frontsector->cceilingheight)
        goto clipsolid;		

    // Window.
    if (backsector->cceilingheight != frontsector->cceilingheight
        || backsector->ffloorheight != frontsector->ffloorheight)
        goto clippass;	
		
    // Reject empty lines used for triggers
    //  and special events.
    // Identical floor and ceiling on both sides,
    // identical light levels on both sides,
    // and no middle texture.
    if (backsector->ceilingpic == frontsector->ceilingpic
	&& backsector->floorpic == frontsector->floorpic
	&& backsector->lightlevel == frontsector->lightlevel
	&& curline->sidedef->midtexture == 0)
    {
	return;
    }

  clippass:
    R_ClipPassWallSegment (x1, x2-1);	
    return;
		
  clipsolid:
    R_ClipSolidWallSegment (x1, x2-1);
}


//
// R_CheckBBox
// Checks BSP node/subtree bounding box.
// Returns true
//  if some part of the bbox might be visible.
//
int	checkcoord[12][4] =
{
    {3,0,2,1},
    {3,0,2,0},
    {3,1,2,0},
    {0},
    {2,0,2,1},
    {0,0,0,0},
    {3,1,3,0},
    {0},
    {2,0,3,1},
    {2,1,3,1},
    {2,1,3,0}
};


bool
RR_CheckBBox(const double* bspcoord)
{
    int			boxx;
    int			boxy;
    int			boxpos;

    cliprange_t*	start;

    // Find the corners of the box
    // that define the edges from current viewpoint.
    if (view.getX() <= bspcoord[BOXLEFT])
	boxx = 0;
    else if (view.getX() < bspcoord[BOXRIGHT])
	boxx = 1;
    else
	boxx = 2;
		
    if (view.getY() >= bspcoord[BOXTOP])
	boxy = 0;
    else if (view.getY() > bspcoord[BOXBOTTOM])
	boxy = 1;
    else
	boxy = 2;
		
    boxpos = (boxy<<2)+boxx;
    if (boxpos == 5)
	return true;
	
    double x1 = bspcoord[checkcoord[boxpos][0]];
    double y1 = bspcoord[checkcoord[boxpos][1]];
    double x2 = bspcoord[checkcoord[boxpos][2]];
    double y2 = bspcoord[checkcoord[boxpos][3]];
    
    // check clip list for an open space
    Angle angle1 = Angle(view.getX(), view.getY(), x1, y1) - vviewangle;
    Angle angle2 = Angle(view.getX(), view.getY(), x2, y2) - vviewangle;
    Angle span = angle1 - angle2;
    
    // Sitting on a line?
    if (span >= Angle(Angle::A180))
	return true;

    Angle tspan = angle1 + cclipangle;
    if (tspan > 2*cclipangle)
    {
	tspan -= 2*cclipangle;

	// Totally off the left edge?
	if (tspan >= span)
	    return false;	

	angle1 = cclipangle;
    }
    tspan = cclipangle - angle2;
    if (tspan > 2*cclipangle)
    {
	tspan -= 2*cclipangle;

	// Totally off the left edge?
	if (tspan >= span)
	    return false;
	
	angle2 = -cclipangle;
    }

    // Find the first clippost
    //  that touches the source post
    //  (adjacent pixels are touching).

    int sx1 = R_ViewAngleToX(angle1);
    int sx2 = R_ViewAngleToX(angle2);
    
    // Does not cross a pixel.
    if (sx1 == sx2)
	return false;			
    sx2--;
	
    start = solidsegs;
    while (start->last < sx2)
	start++;
    
    if (sx1 >= start->first
	&& sx2 <= start->last)
    {
	// The clippost contains the new span.
	return false;
    }

    return true;
}



//
// R_Subsector
// Determine floor/ceiling planes.
// Add sprites of things in sector.
// Draw one or more line segments.
//
void R_Subsector (int num)
{
    int			count;
    seg_t*		line;
    SubSector*	sub;
#ifdef RANGECHECK
    if (num>=subsectors.size())
	I_Error ("R_Subsector: ss %i with numss = %i",
		 num,
		 subsectors.size());
#endif

    sscount++;
    sub = &subsectors[num];
    frontsector = sub->sector;
    count = sub->numlines;
    line = &segs[sub->firstline];

    if (frontsector->ffloorheight < vviewz)
    {
        floorplane = R_FindPlane (frontsector->ffloorheight,
                                  frontsector->floorpic,
                                  frontsector->lightlevel);
    }
    else
	floorplane = NULL;
    
    if (frontsector->cceilingheight > vviewz 
	|| frontsector->ceilingpic == skyflatnum)
    {
        ceilingplane = R_FindPlane (frontsector->cceilingheight,
				    frontsector->ceilingpic,
				    frontsector->lightlevel);
    }
    else
	ceilingplane = NULL;
		
    R_AddSprites (frontsector);	

    while (count--)
    {
	R_AddLine (line);
	line++;
    }
}




//
// RenderBSPNode
// Renders all subsectors below a given node,
//  traversing subtree recursively.
// Just call with BSP root.
void R_RenderBSPNode (int bspnum)
{
    // Found a subsector?
    if (bspnum & NF_SUBSECTOR) {
        if (bspnum == -1)			
            R_Subsector (0);
        else
            R_Subsector(bspnum&(~NF_SUBSECTOR));
        return;
    }
	
    const BspNode& bsp = nodes[bspnum];
    
    // Decide which side the view point is on.
    int side = bsp.pointOnSide(view);
    
    // Recursively divide front space.
    R_RenderBSPNode(bsp.getChild(side)); 
    
    // Possibly divide back space.
    if (RR_CheckBBox(bsp.getBBox(side^1)))	
        R_RenderBSPNode(bsp.getChild(side^1));
}


