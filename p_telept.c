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
//	Teleportation.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: p_telept.c,v 1.3 1997/01/28 22:08:29 b1 Exp $";



#include "doomdef.h"

#include "s_sound.h"

#include "p_local.h"


// Data.
#include "sounds.h"

// State.
#include "r_state.h"



//
// TELEPORTATION
//
int
EV_Teleport
( line_t*	line,
  int		side,
  Mob*	thing )
{
    int		i;
    int		tag;
    Mob*	m;
    Mob*	fog;
    thinker_t*	thinker;
    sector_t*	sector;

    // don't teleport missiles
    if (thing->flags & MF_MISSILE)
	return 0;		

    // Don't teleport if hit back of line,
    //  so you can get out of teleporter.
    if (side == 1)		
	return 0;	

    
    tag = line->tag;
    for (i = 0; i < numsectors; i++)
    {
	if (sectors[ i ].tag == tag )
	{
	    thinker = thinkercap.next;
	    for (thinker = thinkercap.next;
		 thinker != &thinkercap;
		 thinker = thinker->next)
	    {
		// not a mobj
		if (thinker->function.acp1 != (actionf_p1)P_MobjThinker)
		    continue;	

		m = (Mob *)thinker;
		
		// not a teleportman
		if (m->type != MT_TELEPORTMAN )
		    continue;		

		sector = m->subsector->sector;
		// wrong sector
		if (sector-sectors != i )
		    continue;	

		double oldx = thing->xx;
		double oldy = thing->yy;
		double oldz = thing->zz;
				
		if (!PP_TeleportMove (thing, m->xx, m->yy))
		    return 0;
		
		thing->zz = thing->ffloorz;  //fixme: not needed?
		if (thing->player)
		    thing->player->vviewz = thing->zz + thing->player->vviewheight;
				
		// spawn teleport fog at source and destination
		fog = PP_SpawnMobj(oldx, oldy, oldz, MT_TFOG);
		S_StartSound (fog, sfx_telept);
		fog = PP_SpawnMobj(m->xx + (20 * cos(m->_angle)), 
				   m->yy + (20 * sin(m->_angle)),
				   thing->zz, 
				   MT_TFOG);

		// emit sound, where?
		S_StartSound (fog, sfx_telept);
		
		// don't move for a bit
		if (thing->player)
		    thing->reactiontime = 18;	

		thing->_angle = m->_angle;
		thing->mmomx = thing->mmomy = thing->mmomz = 0;
		return 1;
	    }	
	}
    }
    return 0;
}

