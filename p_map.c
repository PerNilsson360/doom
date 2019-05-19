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
//	Movement, collision handling.
//	Shooting and aiming.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: p_map.c,v 1.5 1997/02/03 22:45:11 b1 Exp $";

#include <stdlib.h>

#include "m_bbox.h"
#include "m_random.h"
#include "i_system.h"

#include "doomdef.h"
#include "p_local.h"

#include "s_sound.h"

// State.
#include "doomstat.h"
#include "r_state.h"
// Data.
#include "sounds.h"


static double ttmbbox[4];
static Mob*	tmthing;
static int tmflags;
static double ttmx;
static double ttmy;


// If "floatok" true, move would be ok
// if within "tmfloorz - tmceilingz".
bool		floatok;

double ttmfloorz;
double ttmceilingz;
double ttmdropoffz;

// keep track of the line that lowers the ceiling,
// so missiles don't explode against sky hack walls
line_t*	ceilingline;

// keep track of special lines as they are hit,
// but don't process them until the move is proven valid
#define MAXSPECIALCROSS		8

line_t*		spechit[MAXSPECIALCROSS];
int		numspechit;



//
// TELEPORT MOVE
// 

//
// PIT_StompThing
//
bool PIT_StompThing (Mob* thing)
{
    if (!(thing->flags & MF_SHOOTABLE) )
        return true;
    
    double blockdist = thing->rradius + tmthing->rradius;
    
    if (fabs(thing->xx - ttmx) >= blockdist || 
        fabs(thing->yy - ttmy) >= blockdist)
    {
        // didn't hit it
        return true;
    }
    
    // don't clip against self
    if (thing == tmthing)
        return true;
    
    // monsters don't stomp things except on boss level
    if ( !tmthing->player && gamemap != 30)
        return false;	
		
    P_DamageMobj(thing, tmthing, tmthing, 10000);
	
    return true;
}


//
// P_TeleportMove
//
bool
PP_TeleportMove(
    Mob* thing,
    double x,
    double y)
{
    int			xl;
    int			xh;
    int			yl;
    int			yh;
    int			bx;
    int			by;
    
    subsector_t*	newsubsec;
    
    // kill anything occupying the position
    tmthing = thing;
    tmflags = thing->flags;
	
    ttmx = x;
    ttmy = y;
	
    ttmbbox[BOXTOP] = y + tmthing->rradius;
    ttmbbox[BOXBOTTOM] = y - tmthing->rradius;
    ttmbbox[BOXRIGHT] = x + tmthing->rradius;
    ttmbbox[BOXLEFT] = x - tmthing->rradius;

    newsubsec = RR_PointInSubsector(x,y);
    ceilingline = NULL;
    
    // The base floor/ceiling is from the subsector
    // that contains the point.
    // Any contacted lines the step closer together
    // will adjust them.
    ttmfloorz = ttmdropoffz = newsubsec->sector->ffloorheight;
    ttmceilingz = newsubsec->sector->cceilingheight;
			
    validcount++;
    numspechit = 0;
    
    // stomp on any things contacted
    xl = (ttmbbox[BOXLEFT] - blockMap.oorgx - MMAXRADIUS) / DOUBLE_MAPBLOCKS_DIV;
    xh = (ttmbbox[BOXRIGHT] - blockMap.oorgx + MMAXRADIUS) / DOUBLE_MAPBLOCKS_DIV;
    yl = (ttmbbox[BOXBOTTOM] - blockMap.oorgy - MMAXRADIUS) / DOUBLE_MAPBLOCKS_DIV;
    yh = (ttmbbox[BOXTOP] - blockMap.oorgy + MMAXRADIUS) / DOUBLE_MAPBLOCKS_DIV;
    
    for (bx=xl ; bx<=xh ; bx++)
        for (by=yl ; by<=yh ; by++)
            if (!P_BlockThingsIterator(bx,by,PIT_StompThing))
                return false;
    
    // the move is ok,
    // so link the thing into its new position
    P_UnsetThingPosition (thing);

    thing->ffloorz = ttmfloorz;
    thing->cceilingz = ttmceilingz;	
    thing->xx = x;
    thing->yy = y;

    P_SetThingPosition (thing);
	
    return true;
}


//
// MOVEMENT ITERATOR FUNCTIONS
//


//
// PIT_CheckLine
// Adjusts tmfloorz and tmceilingz as lines are contacted
//
bool PIT_CheckLine (line_t* ld)
{
    if (ttmbbox[BOXRIGHT] <= ld->bbbox[BOXLEFT]
        || ttmbbox[BOXLEFT] >= ld->bbbox[BOXRIGHT]
        || ttmbbox[BOXTOP] <= ld->bbbox[BOXBOTTOM]
        || ttmbbox[BOXBOTTOM] >= ld->bbbox[BOXTOP] )
        return true;
    
    if (PP_BoxOnLineSide (ttmbbox, ld) != -1)
        return true;
		
    // A line has been hit
    
    // The moving thing's destination position will cross
    // the given line.
    // If this should not be allowed, return false.
    // If the line is special, keep track of it
    // to process later if the move is proven ok.
    // NOTE: specials are NOT sorted by order,
    // so two special lines that are only 8 pixels apart
    // could be crossed in either order.
    
    if (!ld->backsector)
	return false;		// one sided line
		
    if (!(tmthing->flags & MF_MISSILE) )
    {
	if ( ld->flags & ML_BLOCKING )
	    return false;	// explicitly blocking everything

	if ( !tmthing->player && ld->flags & ML_BLOCKMONSTERS )
	    return false;	// block monsters only
    }

    // set openrange, opentop, openbottom
    P_LineOpening (ld);	
	
    // adjust floor / ceiling heights
    if (oopentop < ttmceilingz)
    {
        ttmceilingz = oopentop;
        ceilingline = ld;
    }
    
    if (oopenbottom > ttmfloorz)
        ttmfloorz = oopenbottom;	
    
    if (llowfloor < ttmdropoffz)
        ttmdropoffz = llowfloor;
    
    // if contacted a special line, add it to the list
    if (ld->special)
    {
        spechit[numspechit] = ld;
        numspechit++;
    }

    return true;
}

//
// PIT_CheckThing
//
bool PIT_CheckThing (Mob* thing)
{
    bool		solid;
    int			damage;
		
    if (!(thing->flags & (MF_SOLID|MF_SPECIAL|MF_SHOOTABLE) ))
	return true;
    
    double blockdist = thing->rradius + tmthing->rradius;

    if ( fabs(thing->xx - ttmx) >= blockdist
	 || fabs(thing->yy - ttmy) >= blockdist )
    {
	// didn't hit it
	return true;	
    }
    
    // don't clip against self
    if (thing == tmthing)
	return true;
    
    // check for skulls slamming into things
    if (tmthing->flags & MF_SKULLFLY)
    {
	damage = ((P_Random()%8)+1)*tmthing->info->damage;
	
	P_DamageMobj (thing, tmthing, tmthing, damage);
	
	tmthing->flags &= ~MF_SKULLFLY;
	tmthing->mmomx = tmthing->mmomy = tmthing->mmomz = 0;
	
    tmthing->setState((statenum_t)tmthing->info->spawnstate);
	
	return false;		// stop moving
    }

    
    // missiles can hit other things
    if (tmthing->flags & MF_MISSILE)
    {
	// see if it went over / under
        if (tmthing->zz > thing->zz + thing->hheight)
            return true;		// overhead
        if (tmthing->zz+tmthing->hheight < thing->zz)
            return true;		// underneath
		
	if (tmthing->target && (
	    tmthing->target->type == thing->type || 
	    (tmthing->target->type == MT_KNIGHT && thing->type == MT_BRUISER)||
	    (tmthing->target->type == MT_BRUISER && thing->type == MT_KNIGHT) ) )
	{
	    // Don't hit same species as originator.
	    if (thing == tmthing->target)
		return true;

	    if (thing->type != MT_PLAYER)
	    {
		// Explode, but do no damage.
		// Let players missile other players.
		return false;
	    }
	}
	
	if (! (thing->flags & MF_SHOOTABLE) )
	{
	    // didn't do any damage
	    return !(thing->flags & MF_SOLID);	
	}
	
	// damage / explode
	damage = ((P_Random()%8)+1)*tmthing->info->damage;
	P_DamageMobj (thing, tmthing, tmthing->target, damage);

	// don't traverse any more
	return false;				
    }
    
    // check for special pickup
    if (thing->flags & MF_SPECIAL)
    {
	solid = thing->flags&MF_SOLID;
	if (tmflags&MF_PICKUP)
	{
	    // can remove thing
	    P_TouchSpecialThing (thing, tmthing);
	}
	return !solid;
    }
	
    return !(thing->flags & MF_SOLID);
}


//
// MOVEMENT CLIPPING
//

//
// P_CheckPosition
// This is purely informative, nothing is modified
// (except things picked up).
// 
// in:
//  a mobj_t (can be valid or invalid)
//  a position to be checked
//   (doesn't need to be related to the mobj_t->x,y)
//
// during:
//  special things are touched if MF_PICKUP
//  early out on solid lines?
//
// out:
//  newsubsec
//  floorz
//  ceilingz
//  tmdropoffz
//   the lowest point contacted
//   (monsters won't move to a dropoff)
//  speciallines[]
//  numspeciallines
//
bool
PP_CheckPosition(
    Mob* thing,
    double x,
    double y)
{
    int			xl;
    int			xh;
    int			yl;
    int			yh;
    int			bx;
    int			by;
    subsector_t*	newsubsec;
    
    tmthing = thing;
    tmflags = thing->flags;
	
    ttmx = x;
    ttmy = y;
	
    ttmbbox[BOXTOP] = y + tmthing->rradius;
    ttmbbox[BOXBOTTOM] = y - tmthing->rradius;
    ttmbbox[BOXRIGHT] = x + tmthing->rradius;
    ttmbbox[BOXLEFT] = x - tmthing->rradius;

    newsubsec = RR_PointInSubsector (x,y);
    ceilingline = NULL;
    
    // The base floor / ceiling is from the subsector
    // that contains the point.
    // Any contacted lines the step closer together
    // will adjust them.
    ttmfloorz = ttmdropoffz = newsubsec->sector->ffloorheight;
    ttmceilingz = newsubsec->sector->cceilingheight;
    
    validcount++;
    numspechit = 0;
    
    if ( tmflags & MF_NOCLIP )
        return true;
    
    // Check things first, possibly picking things up.
    // The bounding box is extended by MAXRADIUS
    // because mobj_ts are grouped into mapblocks
    // based on their origin point, and can overlap
    // into adjacent blocks by up to MAXRADIUS units.
    xl = (ttmbbox[BOXLEFT] - blockMap.oorgx - MMAXRADIUS) / DOUBLE_MAPBLOCKS_DIV;
    xh = (ttmbbox[BOXRIGHT] - blockMap.oorgx + MMAXRADIUS) / DOUBLE_MAPBLOCKS_DIV;
    yl = (ttmbbox[BOXBOTTOM] - blockMap.oorgy - MMAXRADIUS) / DOUBLE_MAPBLOCKS_DIV;
    yh = (ttmbbox[BOXTOP] - blockMap.oorgy + MMAXRADIUS) / DOUBLE_MAPBLOCKS_DIV;

    for (bx=xl ; bx<=xh ; bx++)
        for (by=yl ; by<=yh ; by++)
            if (!P_BlockThingsIterator(bx,by,PIT_CheckThing))
                return false;
    
    // check lines
    xl = (ttmbbox[BOXLEFT] - blockMap.oorgx) / DOUBLE_MAPBLOCKS_DIV;
    xh = (ttmbbox[BOXRIGHT] - blockMap.oorgx) / DOUBLE_MAPBLOCKS_DIV;
    yl = (ttmbbox[BOXBOTTOM] - blockMap.oorgy) / DOUBLE_MAPBLOCKS_DIV;
    yh = (ttmbbox[BOXTOP] - blockMap.oorgy) / DOUBLE_MAPBLOCKS_DIV;
    
    for (bx=xl ; bx<=xh ; bx++)
        for (by=yl ; by<=yh ; by++)
            if (!P_BlockLinesIterator (bx,by,PIT_CheckLine))
                return false;
    
    return true;
}


//
// P_TryMove
// Attempt to move to a new position,
// crossing special lines unless MF_TELEPORT is set.
//
bool
PP_TryMove( 
    Mob* thing,
    double x,
    double y)
{
    int		side;
    int		oldside;
    line_t*	ld;
    
    floatok = false;
    if (!PP_CheckPosition (thing, x, y))
        return false;		// solid wall or thing
    
    if ( !(thing->flags & MF_NOCLIP) )
    {
        if (ttmceilingz - ttmfloorz < thing->hheight)
            return false;	// doesn't fit
        
        floatok = true;
        
        if ( !(thing->flags&MF_TELEPORT) 
             &&ttmceilingz - thing->zz < thing->hheight)
            return false;	// mobj must lower itself to fit
        
        if (!(thing->flags & MF_TELEPORT) && ttmfloorz - thing->zz > 24)
            return false;	// too big a step up
        
        if ( !(thing->flags & (MF_DROPOFF | MF_FLOAT))
             && ttmfloorz - ttmdropoffz > 24)
            return false;	// don't stand over a dropoff
    }
    
    // the move is ok,
    // so link the thing into its new position
    P_UnsetThingPosition (thing);
    
    double oldx = thing->xx;
    double oldy = thing->yy;
    thing->ffloorz = ttmfloorz;
    thing->cceilingz = ttmceilingz;	
    thing->xx = x;
    thing->yy = y;
    
    P_SetThingPosition (thing);
    
    // if any special lines were hit, do the effect
    if (! (thing->flags&(MF_TELEPORT|MF_NOCLIP)) )
    {
        while (numspechit--)
        {
            // see if the line was crossed
            ld = spechit[numspechit];
            side = P_PointOnLineSide(thing->xx, thing->yy, ld);
            oldside = P_PointOnLineSide(oldx, oldy, ld);
            if (side != oldside)
            {
                if (ld->special)
                    P_CrossSpecialLine (ld-lines, oldside, thing);
            }
        }
    }

    return true;
}


//
// P_ThingHeightClip
// Takes a valid thing and adjusts the thing->floorz,
// thing->ceilingz, and possibly thing->z.
// This is called for all nearby monsters
// whenever a sector changes height.
// If the thing doesn't fit,
// the z will be set to the lowest value
// and false will be returned.
//
bool P_ThingHeightClip (Mob* thing)
{
    bool		onfloor;
	
    onfloor = (thing->zz == thing->ffloorz);
	
    PP_CheckPosition (thing, thing->xx, thing->yy);	
    // what about stranding a monster partially off an edge?
	
    thing->ffloorz = ttmfloorz;
    thing->cceilingz = ttmceilingz;
	
    if (onfloor)
    {
        // walking monsters rise and fall with the floor
        thing->zz = thing->ffloorz;
    }
    else
    {
        // don't adjust a floating monster unless forced to
        if (thing->zz+thing->hheight > thing->cceilingz)
            thing->zz = thing->cceilingz - thing->hheight;
    }
	
    if (thing->cceilingz - thing->ffloorz < thing->hheight)
        return false;
    
    return true;
}



//
// SLIDE MOVE
// Allows the player to slide along any angled walls.
//
double bbestslidefrac;
double ssecondslidefrac;

line_t*		bestslideline;
line_t*		secondslideline;

Mob*		slidemo;

double ttmxmove;
double ttmymove;



//
// P_HitSlideLine
// Adjusts the xmove / ymove
// so that the next move will slide along the wall.
//
void P_HitSlideLine (line_t* ld)
{
    if (ld->slopetype == ST_HORIZONTAL)
    {
	ttmymove = 0;
	return;
    }
    
    if (ld->slopetype == ST_VERTICAL)
    {
	ttmxmove = 0;
	return;
    }
	
    int side = P_PointOnLineSide (slidemo->xx, slidemo->yy, ld);
	
    Angle lineangle(0, 0, ld->ddx, ld->ddy);

    if (side == 1) {
	printf("P_HitSlideLine side 1");
	lineangle += Angle::A180;
    }

    Angle moveangle(0, 0, ttmxmove, ttmymove);
    double deltaangle = moveangle - lineangle;

    if (deltaangle > Angle::A180) {
	printf("P_HitSlideLine deltangle");
	deltaangle += Angle::A180;
    }
    //	I_Error ("SlideLine: ang>ANG180");

    double movelen = PP_AproxDistance (ttmxmove, ttmymove);
    double newlen = movelen * cos(deltaangle);

    ttmxmove = newlen * cos(lineangle);	
    ttmymove = newlen * sin(lineangle);	
}


//
// PTR_SlideTraverse
//
bool PTR_SlideTraverse (intercept_t* in)
{
    line_t*	li;
	
    if (!in->isaline)
        I_Error ("PTR_SlideTraverse: not a line?");
    
    li = in->d.line;
    
    if ( ! (li->flags & ML_TWOSIDED) )
    {
        if (P_PointOnLineSide (slidemo->xx, slidemo->yy, li))
        {
            // don't hit the back side
            return true;		
        }
        goto isblocking;
    }

    // set openrange, opentop, openbottom
    P_LineOpening (li);
    
    if (oopenrange < slidemo->hheight)
        goto isblocking;		// doesn't fit
		
    if (oopentop - slidemo->zz < slidemo->hheight)
        goto isblocking;		// mobj is too high

    if (oopenbottom - slidemo->zz > 24)
        goto isblocking;		// too big a step up
    
    // this line doesn't block movement
    return true;		
	
    // the line does block movement,
    // see if it is closer than best so far
isblocking:
    double frac = in->fraction;
    if (frac < bbestslidefrac)
    {
        ssecondslidefrac = bbestslidefrac;
        secondslideline = bestslideline;
        bbestslidefrac = frac;
        bestslideline = li;
    }
	
    return false;	// stop
}



//
// P_SlideMove
// The momx / momy move is bad, so try to slide
// along a wall.
// Find the first line hit, move flush to it,
// and slide along it
//
// This is a kludgy mess.
//
void P_SlideMove (Mob* mo)
{
    double leadx;
    double leady;
    double trailx;
    double traily;
    double newx;
    double newy;
    int			hitcount;
		
    slidemo = mo;
    hitcount = 0;
    
retry:
    if (++hitcount == 3)
        goto stairstep;		// don't loop forever

    
    // trace along the three leading corners
    if (mo->mmomx > 0)
    {
        leadx = mo->xx + mo->rradius;
        trailx = mo->xx - mo->rradius;
    }
    else
    {
        leadx = mo->xx - mo->rradius;
        trailx = mo->xx + mo->rradius;
    }
	
    if (mo->mmomy > 0)
    {
        leady = mo->yy + mo->rradius;
        traily = mo->yy - mo->rradius;
    }
    else
    {
        leady = mo->yy - mo->rradius;
        traily = mo->yy + mo->rradius;
    }
		
    bbestslidefrac = 1;
	
    P_PathTraverse(leadx, leady, leadx +  mo->mmomx, leady + mo->mmomy, PT_ADDLINES, PTR_SlideTraverse);
    P_PathTraverse(trailx, leady, trailx +  mo->mmomx , leady + mo->mmomy, PT_ADDLINES, PTR_SlideTraverse);
    P_PathTraverse(leadx, traily, leadx +  mo->mmomx, traily + mo->mmomy, PT_ADDLINES, PTR_SlideTraverse);
    
    // move up to the wall
    if (bbestslidefrac == 1)
    {
	// the move most have hit the middle, so stairstep
      stairstep:
        if (!PP_TryMove (mo, mo->xx, mo->yy + mo->mmomy))
            PP_TryMove (mo, mo->xx + mo->mmomx, mo->yy);
	return;
    }

    // fudge a bit to make sure it doesn't hit
    bbestslidefrac -= 0.5;	    // 1000 = 0.5
    if (bbestslidefrac > 0)
    {
        newx = mo->mmomx * bbestslidefrac;
        newy = mo->mmomy * bbestslidefrac;
        
        if (!PP_TryMove(mo, mo->xx+newx, mo->yy+newy))
            goto stairstep;
    }
        
    // Now continue along the wall.
    // First calculate remainder.
    bbestslidefrac = 1 -(bbestslidefrac + 0.5);
    
    if (bbestslidefrac > 1)
        bbestslidefrac = 1;
    
    if (bbestslidefrac <= 0)
        return;
    
    ttmxmove = mo->mmomx * bbestslidefrac;
    ttmymove = mo->mmomy * bbestslidefrac;

    P_HitSlideLine (bestslideline);	// clip the moves

    mo->mmomx = ttmxmove;
    mo->mmomy = ttmymove;
		
    if (!PP_TryMove(mo, mo->xx+ttmxmove, mo->yy+ttmymove))
    {
        goto retry;
    }
}


//
// P_LineAttack
//
Mob*		linetarget;	// who got hit (or NULL)
Mob*		shootthing;

// Height if not aiming up or down
// ???: use slope for monsters?
double		shootzz;	

int		la_damage;
double		aattackrange;

double 	        aaimslope;

// slopes to top and bottom of target
extern double ttopslope;
extern double bbottomslope;	


//
// PTR_AimTraverse
// Sets linetaget and aimslope when a target is aimed at.
//
bool
PTR_AimTraverse (intercept_t* in)
{
    line_t*		li;
    Mob*		th;
		
    if (in->isaline)
    {
        li = in->d.line;
	
        if ( !(li->flags & ML_TWOSIDED) ) {
            return false;		// stop
        }
	
	// Crosses a two sided line.
	// A two sided line will restrict
	// the possible target ranges.
        P_LineOpening (li);
	
        if (oopenbottom >= oopentop) {
            return false;		// stop
        }
        
        double dist = aattackrange * (in->fraction);
        
        if (li->frontsector->ffloorheight != li->backsector->ffloorheight)
        {
            double slope = (oopenbottom - shootzz) / dist;
            if (slope > bbottomslope) {
                bbottomslope = slope;
            }
        }
		
        if (li->frontsector->cceilingheight != li->backsector->cceilingheight)
        {
            double slope = (oopentop - shootzz) / dist;
            if (slope < ttopslope) {
                ttopslope = slope;
            }
        }
        
        if (ttopslope <= bbottomslope) {
            return false;		// stop
        }
        
        return true;			// shot continues
    }
    
    // shoot a thing
    th = in->d.thing;

    if (th == shootthing) {
        return true;			// can't shoot self
    }
    
    if (!(th->flags&MF_SHOOTABLE)) {
        return true;			// corpse or something
    }
    
    // check angles to see if the thing can be aimed at
    double dist = aattackrange * (in->fraction);
    double thingtopslope = (th->zz+th->hheight - shootzz) / dist;
    
    if (thingtopslope < bbottomslope) {
        return true;			// shot over the thing
    }
    
    double thingbottomslope = (th->zz - shootzz) / dist;
    
    if (thingbottomslope > ttopslope) {
        return true;			// shot under the thing
    }
    
    // this thing can be hit!
    if (thingtopslope > ttopslope)
        thingtopslope = ttopslope;
    
    if (thingbottomslope < bbottomslope)
        thingbottomslope = bbottomslope;
    
    aaimslope = (thingtopslope+thingbottomslope)/2;
    linetarget = th;
    
    return false;			// don't go any farther
}


//
// PTR_ShootTraverse
//
bool PTR_ShootTraverse (intercept_t* in)
{
    double x;
    double y;
    double z;
    double frac;
    
    line_t*		li;
    
    Mob*		th;
    double dist;
    
    if (in->isaline)
    {
        li = in->d.line;
        
        if (li->special)
            P_ShootSpecialLine (shootthing, li);
        
        if ( !(li->flags & ML_TWOSIDED) ) 
        {
            goto hitline;
        }
        
        // crosses a two sided line
        P_LineOpening(li);
		
        dist = aattackrange * (in->fraction);
        
        if (li->frontsector->ffloorheight != li->backsector->ffloorheight)
        {
            double slope = (oopenbottom - shootzz) / dist;
            if (slope > aaimslope)
            {
                goto hitline;
            }
        }
		
        if (li->frontsector->cceilingheight != li->backsector->cceilingheight)
        {
            double slope = (oopentop - shootzz) / dist;
            if (slope < aaimslope)
            {
                goto hitline;
            }
        }

        // shot continues
        return true;
        
        
        // hit line
    hitline:
        // position a bit closer
        frac = in->fraction - (4 / aattackrange);
        x = trace.x + (trace.dx * frac);
        y = trace.y + (trace.dy * frac);
        z = shootzz + (aaimslope * frac * aattackrange);
        
        if (li->frontsector->ceilingpic == skyflatnum)
        {
            // don't shoot the sky!
            if (z > li->frontsector->cceilingheight) {
                return false;
            }
	    
            // it's a sky hack wall
            if	(li->backsector && li->backsector->ceilingpic == skyflatnum)
                return false;		
        }
        
        // Spawn bullet puffs.
        PP_SpawnPuff(x, y, z);
        // don't go any farther
        return false;	
    }
    
    // shoot a thing
    th = in->d.thing;
    if (th == shootthing)
    {
        return true;		// can't shoot self
    }
    
    if (!(th->flags&MF_SHOOTABLE))
    {
        return true;		// corpse or something
    }
    
    // check angles to see if the thing can be aimed at
    dist = aattackrange * in->fraction;
    double thingtopslope = (th->zz + th->hheight - shootzz) / dist;
    
    if (thingtopslope < aaimslope)
    {
        return true;		// shot over the thing
    }
    
    double thingbottomslope = (th->zz - shootzz) / dist;
    
    if (thingbottomslope > aaimslope)
    {
            return true;		// shot under the thing
    }
    
    // hit thing
    // position a bit closer
    frac = in->fraction - (10 / aattackrange);
    
    x = trace.x + (trace.dx * frac);
    y = trace.y + (trace.dy * frac);
    z = shootzz + (aaimslope * frac * aattackrange);

    // Spawn bullet puffs or blod spots,
    // depending on target type.
    if (in->d.thing->flags & MF_NOBLOOD)
        PP_SpawnPuff(x, y, z);
    else
        PP_SpawnBlood(x, y, z, la_damage);

    if (la_damage)
        P_DamageMobj (th, shootthing, shootthing, la_damage);

    // don't go any farther
    return false;
	
}


//
// P_AimLineAttack
//
double
PP_AimLineAttack(Mob* t1,
                 Angle angle,
                 double distance)
{
    shootthing = t1;
    
    double x2 = t1->xx + (distance * cos(angle));
    double y2 = t1->yy + (distance * sin(angle));
    shootzz = t1->zz + (t1->hheight / 2) + 8;

    // can't shoot outside view angles
    ttopslope = 100.0/160;	
    bbottomslope = -100.0/160;
    aattackrange = distance;
    linetarget = NULL;
	
    P_PathTraverse(t1->xx, t1->yy,
                   x2, y2,
                   PT_ADDLINES|PT_ADDTHINGS,
                   PTR_AimTraverse );

    if (linetarget)
        return aaimslope;
    
    return 0;
}
 

//
// P_LineAttack
// If damage == 0, it is just a test trace
// that will leave linetarget set.
//
void
PP_LineAttack(Mob* t1,
              Angle angle,
              double distance,
              double slope,
              int damage )
{
    shootthing = t1;
    la_damage = damage;
    double x2 = t1->xx + (distance * cos(angle));
    double y2 = t1->yy + (distance * sin(angle));
    shootzz = t1->zz + (t1->hheight / 2) + 8;
    aattackrange = distance;
    aaimslope = slope;
    P_PathTraverse(t1->xx, t1->yy,
                   x2, y2,
                   PT_ADDLINES|PT_ADDTHINGS,
                   PTR_ShootTraverse );
}
 


//
// USE LINES
//
Mob*		usething;

bool	PTR_UseTraverse (intercept_t* in)
{
    int		side;
	
    if (!in->d.line->special)
    {
        P_LineOpening (in->d.line);
        if (oopenrange <= 0)
        {
            S_StartSound (usething, sfx_noway);
            
            // can't use through a wall
            return false;	
        }
        // not a special line, but keep checking
        return true ;		
    }
	
    side = 0;
    if (P_PointOnLineSide(usething->xx, usething->yy, in->d.line) == 1)
        side = 1;
    
    //	return false;		// don't use back side
	
    P_UseSpecialLine (usething, in->d.line, side);

    // can't use for than one special line in a row
    return false;
}


//
// P_UseLines
// Looks for special lines in front of the player to activate.
//
void P_UseLines (player_t*	player) 
{
    double x1;
    double y1;
    double x2;
    double y2;
	
    usething = player->mo;
		
    x1 = player->mo->xx;
    y1 = player->mo->yy;
    x2 = x1 + (UUSERANGE * cos(player->mo->_angle));
    y2 = y1 + (UUSERANGE * sin(player->mo->_angle));
	
    P_PathTraverse(x1, y1, x2, y2, PT_ADDLINES, PTR_UseTraverse);
}


//
// RADIUS ATTACK
//
Mob*		bombsource;
Mob*		bombspot;
int		bombdamage;


//
// PIT_RadiusAttack
// "bombsource" is the creature
// that caused the explosion at "bombspot".
//
bool PIT_RadiusAttack (Mob* thing)
{
    if (!(thing->flags & MF_SHOOTABLE) )
        return true;

    // Boss spider and cyborg
    // take no damage from concussion.
    if (thing->type == MT_CYBORG
        || thing->type == MT_SPIDER)
        return true;	
    
    double dx = fabs(thing->xx - bombspot->xx);
    double dy = fabs(thing->yy - bombspot->yy);
    
    double dist = dx>dy ? dx : dy;
    dist = (dist - thing->rradius);

    if (dist < 0)
        dist = 0;

    if (dist >= bombdamage)
        return true;	// out of range

    if ( P_CheckSight (thing, bombspot) )
    {
        // must be in direct path
        P_DamageMobj (thing, bombspot, bombsource, bombdamage - dist);
    }
    
    return true;
}


//
// P_RadiusAttack
// Source is the creature that caused the explosion at spot.
//
void
P_RadiusAttack
( Mob*	spot,
  Mob*	source,
  int		damage )
{
    int		x;
    int		y;
    
    int		xl;
    int		xh;
    int		yl;
    int		yh;
    
    double dist;
	
    dist = (damage + MMAXRADIUS);
    yh = (spot->yy + dist - blockMap.oorgy) / DOUBLE_MAPBLOCKS_DIV;
    yl = (spot->yy - dist - blockMap.oorgy) / DOUBLE_MAPBLOCKS_DIV;
    xh = (spot->xx + dist - blockMap.oorgx) / DOUBLE_MAPBLOCKS_DIV;
    xl = (spot->xx - dist - blockMap.oorgx) / DOUBLE_MAPBLOCKS_DIV;
    bombspot = spot;
    bombsource = source;
    bombdamage = damage;
	
    for (y=yl ; y<=yh ; y++)
	for (x=xl ; x<=xh ; x++)
	    P_BlockThingsIterator (x, y, PIT_RadiusAttack );
}



//
// SECTOR HEIGHT CHANGING
// After modifying a sectors floor or ceiling height,
// call this routine to adjust the positions
// of all things that touch the sector.
//
// If anything doesn't fit anymore, true will be returned.
// If crunch is true, they will take damage
//  as they are being crushed.
// If Crunch is false, you should set the sector height back
//  the way it was and call P_ChangeSector again
//  to undo the changes.
//
bool		crushchange;
bool		nofit;


//
// PIT_ChangeSector
//
bool PIT_ChangeSector (Mob*	thing)
{
    Mob*	mo;
	
    if (P_ThingHeightClip (thing))
    {
        // keep checking
        return true;
    }
    

    // crunch bodies to giblets
    if (thing->health <= 0)
    {
        thing->setState(S_GIBS);

	thing->flags &= ~MF_SOLID;
	thing->hheight = 0;
	thing->rradius = 0;

	// keep checking
	return true;		
    }

    // crunch dropped items
    if (thing->flags & MF_DROPPED)
    {
	P_RemoveMobj (thing);
	
	// keep checking
	return true;		
    }

    if (! (thing->flags & MF_SHOOTABLE) )
    {
	// assume it is bloody gibs or something
	return true;			
    }
    
    nofit = true;

    if (crushchange && !(leveltime&3) )
    {
	P_DamageMobj(thing,NULL,NULL,10);

	// spray blood in a random direction
	mo = PP_SpawnMobj(thing->xx,
                     thing->yy,
                     thing->zz + thing->hheight/2, 
                     MT_BLOOD);
	
	mo->mmomx = (P_Random() - P_Random ()) / 2 * 4;
	mo->mmomy = (P_Random() - P_Random ()) / 2 * 4;
    }

    // keep checking (crush other things)	
    return true;	
}



//
// P_ChangeSector
//
bool
P_ChangeSector
( sector_t*	sector,
  bool	crunch )
{
    int		x;
    int		y;
    nofit = false;
    crushchange = crunch;
           
    // re-check heights for all things near the moving sector
    for (x=sector->blockbox[BOXLEFT] ; x<= sector->blockbox[BOXRIGHT] ; x++)
	for (y=sector->blockbox[BOXBOTTOM];y<= sector->blockbox[BOXTOP] ; y++)
	    P_BlockThingsIterator (x, y, PIT_ChangeSector);
	
	
    return nofit;
}

