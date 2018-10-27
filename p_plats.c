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
//	Plats (i.e. elevator platforms) code, raising/lowering.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: p_plats.c,v 1.5 1997/02/03 22:45:12 b1 Exp $";


#include "i_system.h"
#include "z_zone.h"
#include "m_random.h"

#include "doomdef.h"
#include "p_local.h"

#include "s_sound.h"

// State.
#include "doomstat.h"
#include "r_state.h"

// Data.
#include "sounds.h"


plat_t*		activeplats[MAXPLATS];



//
// Move a plat up and down
//
void T_PlatRaise(plat_t* plat)
{
    result_e	res;
	
    switch(plat->status)
    {
      case up:
	res = TT_MovePlane(plat->sector,
                       plat->sspeed,
                       plat->hhigh,
                       plat->crush,
                       0,
                       1);
					
	if (plat->type == raiseAndChange
	    || plat->type == raiseToNearestAndChange)
	{
	    if (!(leveltime&7))
            S_StartSound((Mob *)&plat->sector->soundorg,
			     sfx_stnmov);
	}
	
				
	if (res == crushed && (!plat->crush))
	{
	    plat->count = plat->wait;
	    plat->status = down;
	    S_StartSound((Mob *)&plat->sector->soundorg,
			 sfx_pstart);
	}
	else
	{
	    if (res == pastdest)
	    {
		plat->count = plat->wait;
		plat->status = waiting;
		S_StartSound((Mob *)&plat->sector->soundorg,
			     sfx_pstop);

		switch(plat->type)
		{
		  case blazeDWUS:
		  case downWaitUpStay:
		    P_RemoveActivePlat(plat);
		    break;
		    
		  case raiseAndChange:
		  case raiseToNearestAndChange:
		    P_RemoveActivePlat(plat);
		    break;
		    
		  default:
		    break;
		}
	    }
	}
	break;
	
      case	down:
	res = TT_MovePlane(plat->sector,
                      plat->sspeed,
                      plat->llow,
                      false,
                      0,
                      -1);

	if (res == pastdest)
	{
	    plat->count = plat->wait;
	    plat->status = waiting;
	    S_StartSound((Mob *)&plat->sector->soundorg,sfx_pstop);
	}
	break;
	
      case	waiting:
	if (!--plat->count)
	{
	    if (plat->sector->ffloorheight == plat->llow)
		plat->status = up;
	    else
		plat->status = down;
	    S_StartSound((Mob *)&plat->sector->soundorg,sfx_pstart);
	}
      case	in_stasis:
	break;
    }
}


//
// Do Platforms
//  "amount" is only used for SOME platforms.
//
int
EV_DoPlat
( line_t*	line,
  plattype_e	type,
  int		amount )
{
    plat_t*	plat;
    int		secnum;
    int		rtn;
    sector_t*	sec;
	
    secnum = -1;
    rtn = 0;

    
    //	Activate all <type> plats that are in_stasis
    switch(type)
    {
      case perpetualRaise:
	P_ActivateInStasis(line->tag);
	break;
	
      default:
	break;
    }
	
    while ((secnum = P_FindSectorFromLineTag(line,secnum)) >= 0)
    {
	sec = &sectors[secnum];

	if (sec->specialdata)
	    continue;
	
	// Find lowest & highest floors around sector
	rtn = 1;
	plat = (plat_t*)Z_Malloc( sizeof(*plat), PU_LEVSPEC, 0);
	P_AddThinker(&plat->thinker);
		
	plat->type = type;
	plat->sector = sec;
	plat->sector->specialdata = plat;
	plat->thinker.function.acp1 = (actionf_p1) T_PlatRaise;
	plat->crush = false;
	plat->tag = line->tag;
	
	switch(type)
	{
	  case raiseToNearestAndChange:
	    plat->sspeed = PPLATSPEED/2;
	    sec->floorpic = sides[line->sidenum[0]].sector->floorpic;
	    plat->hhigh = PP_FindNextHighestFloor(sec, sec->ffloorheight);
	    plat->wait = 0;
	    plat->status = up;
	    // NO MORE DAMAGE, IF APPLICABLE
	    sec->special = 0;		

	    S_StartSound((Mob *)&sec->soundorg,sfx_stnmov);
	    break;
	    
	  case raiseAndChange:
	    plat->sspeed = PPLATSPEED/2;
	    sec->floorpic = sides[line->sidenum[0]].sector->floorpic;
	    plat->hhigh = sec->ffloorheight + amount;
	    plat->wait = 0;
	    plat->status = up;

	    S_StartSound((Mob *)&sec->soundorg,sfx_stnmov);
	    break;
	    
	  case downWaitUpStay:
	    plat->sspeed = PPLATSPEED * 4;
	    plat->llow = PP_FindLowestFloorSurrounding(sec);

	    if (plat->llow > sec->ffloorheight)
            plat->llow = sec->ffloorheight;

	    plat->hhigh = sec->ffloorheight;
	    plat->wait = 35*PLATWAIT;
	    plat->status = down;
	    S_StartSound((Mob *)&sec->soundorg,sfx_pstart);
	    break;
	    
	  case blazeDWUS:
	    plat->sspeed = PPLATSPEED * 8;
	    plat->llow = PP_FindLowestFloorSurrounding(sec);

	    if (plat->llow > sec->ffloorheight)
		plat->llow = sec->ffloorheight;

	    plat->hhigh = sec->ffloorheight;
	    plat->wait = 35*PLATWAIT;
	    plat->status = down;
	    S_StartSound((Mob *)&sec->soundorg,sfx_pstart);
	    break;
	    
	  case perpetualRaise:
	    plat->sspeed = PPLATSPEED;
	    plat->llow = PP_FindLowestFloorSurrounding(sec);

	    if (plat->llow > sec->ffloorheight)
		plat->llow = sec->ffloorheight;

	    plat->hhigh = PP_FindHighestFloorSurrounding(sec);

	    if (plat->hhigh < sec->ffloorheight)
		plat->hhigh = sec->ffloorheight;

	    plat->wait = 35*PLATWAIT;
	    plat->status = (plat_e)(P_Random()&1);

	    S_StartSound((Mob *)&sec->soundorg,sfx_pstart);
	    break;
	}
	P_AddActivePlat(plat);
    }
    return rtn;
}



void P_ActivateInStasis(int tag)
{
    int		i;
	
    for (i = 0;i < MAXPLATS;i++)
	if (activeplats[i]
	    && (activeplats[i])->tag == tag
	    && (activeplats[i])->status == in_stasis)
	{
	    (activeplats[i])->status = (activeplats[i])->oldstatus;
	    (activeplats[i])->thinker.function.acp1
	      = (actionf_p1) T_PlatRaise;
	}
}

void EV_StopPlat(line_t* line)
{
    int		j;
	
    for (j = 0;j < MAXPLATS;j++)
	if (activeplats[j]
	    && ((activeplats[j])->status != in_stasis)
	    && ((activeplats[j])->tag == line->tag))
	{
	    (activeplats[j])->oldstatus = (activeplats[j])->status;
	    (activeplats[j])->status = in_stasis;
	    (activeplats[j])->thinker.function.acv = (actionf_v)NULL;
	}
}

void P_AddActivePlat(plat_t* plat)
{
    int		i;
    
    for (i = 0;i < MAXPLATS;i++)
	if (activeplats[i] == NULL)
	{
	    activeplats[i] = plat;
	    return;
	}
    I_Error ("P_AddActivePlat: no more plats!");
}

void P_RemoveActivePlat(plat_t* plat)
{
    int		i;
    for (i = 0;i < MAXPLATS;i++)
	if (plat == activeplats[i])
	{
	    (activeplats[i])->sector->specialdata = NULL;
	    P_RemoveThinker(&(activeplats[i])->thinker);
	    activeplats[i] = NULL;
	    
	    return;
	}
    I_Error ("P_RemoveActivePlat: can't find plat!");
}
