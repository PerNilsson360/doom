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
//	Floor animation: raising stairs.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: p_floor.c,v 1.4 1997/02/03 16:47:54 b1 Exp $";


#include "z_zone.h"
#include "doomdef.h"
#include "p_local.h"

#include "s_sound.h"

// State.
#include "doomstat.h"
#include "r_state.h"
// Data.
#include "sounds.h"
#include <float.h>
#include "Sector.hh"

//
// FLOORS
//

//
// Move a plane (floor or ceiling) and check for crushing
//
result_e
TT_MovePlane(Sector* sector,
             double speed,
             double dest,
             bool crush,
             int floorOrCeiling,
             int direction )
{
    bool	flag;
    double lastpos;

    switch(floorOrCeiling)
    {
    case 0:
        // FLOOR
        switch(direction)
        {
        case -1:
            // DOWN
            if (sector->ffloorheight - speed < dest)
            {
                lastpos = sector->ffloorheight;
                sector->ffloorheight = dest;
                flag = P_ChangeSector(sector,crush);
                if (flag == true)
                {
                    sector->ffloorheight = lastpos;
                    P_ChangeSector(sector,crush);
                    //return crushed;
                }
                return pastdest;
            }
            else
            {
                lastpos = sector->ffloorheight;
                sector->ffloorheight -= speed;
                flag = P_ChangeSector(sector,crush);
                if (flag == true)
                {
                    sector->ffloorheight = lastpos;
                    P_ChangeSector(sector,crush);
                    return crushed;
                }
            }
            break;
						
        case 1:
            // UP
            if (sector->ffloorheight + speed > dest)
            {
                lastpos = sector->ffloorheight;
                sector->ffloorheight = dest;
                flag = P_ChangeSector(sector,crush);
                if (flag == true)
                {
                    sector->ffloorheight = lastpos;
                    P_ChangeSector(sector,crush);
                    //return crushed;
                }
                return pastdest;
            }
            else
            {
                // COULD GET CRUSHED
                lastpos = sector->ffloorheight;
                sector->ffloorheight += speed;
                flag = P_ChangeSector(sector,crush);
                if (flag == true)
                {
                    if (crush == true)
                        return crushed;
                    sector->ffloorheight = lastpos;
                    P_ChangeSector(sector,crush);
                    return crushed;
                }
            }
            break;
        }
        break;
									
    case 1:
        // CEILING
        switch(direction)
        {
        case -1:
            // DOWN
            if (sector->cceilingheight - speed < dest)
            {
                lastpos = sector->cceilingheight;
                sector->cceilingheight = dest;
                flag = P_ChangeSector(sector,crush);

                if (flag == true)
                {
                    sector->cceilingheight = lastpos;
                    P_ChangeSector(sector,crush);
                    //return crushed;
                }
                return pastdest;
            }
            else
            {
                // COULD GET CRUSHED
                lastpos = sector->cceilingheight;
                sector->cceilingheight -= speed;
                flag = P_ChangeSector(sector,crush);

                if (flag == true)
                {
                    if (crush == true)
                        return crushed;
                    sector->cceilingheight = lastpos;
                    P_ChangeSector(sector,crush);
                    return crushed;
                }
            }
            break;
						
        case 1:
            // UP
            if (sector->cceilingheight + speed > dest)
            {
                lastpos = sector->cceilingheight;
                sector->cceilingheight = dest;
                flag = P_ChangeSector(sector,crush);
                if (flag == true)
                {
                    sector->cceilingheight = lastpos;
                    P_ChangeSector(sector,crush);
                    //return crushed;
                }
                return pastdest;
            }
            else
            {
                lastpos = sector->cceilingheight;
                sector->cceilingheight += speed;
                flag = P_ChangeSector(sector,crush);
// UNUSED
#if 0
                if (flag == true)
                {
                    sector->ceilingheight = lastpos;
                    P_ChangeSector(sector,crush);
                    return crushed;
                }
#endif
            }
            break;
        }
        break;
		
    }
    return ok;
}


//
// MOVE A FLOOR TO IT'S DESTINATION (UP OR DOWN)
//
void T_MoveFloor(floormove_t* floor)
{
    result_e	res;
    res = TT_MovePlane(floor->sector,
                       floor->sspeed,
                       floor->ffloordestheight,
                       floor->crush,
                       0,
                       floor->direction);
    
    if (!(leveltime&7))
	S_StartSound((Mob *)&floor->sector->soundorg,
		     sfx_stnmov);
    
    if (res == pastdest)
    {
	floor->sector->specialdata = NULL;

	if (floor->direction == 1)
	{
	    switch(floor->type)
	    {
	      case donutRaise:
		floor->sector->special = floor->newspecial;
		floor->sector->floorpic = floor->texture;
	      default:
		break;
	    }
	}
	else if (floor->direction == -1)
	{
	    switch(floor->type)
	    {
	      case lowerAndChange:
		floor->sector->special = floor->newspecial;
		floor->sector->floorpic = floor->texture;
	      default:
		break;
	    }
	}
	P_RemoveThinker(&floor->thinker);

	S_StartSound((Mob *)&floor->sector->soundorg,
		     sfx_pstop);
    }

}

//
// HANDLE FLOOR TYPES
//
int
EV_DoFloor
( Line*	line,
  floor_e	floortype )
{
    int			secnum;
    int			rtn;
    int			i;
    Sector*		sec;
    floormove_t*	floor;

    secnum = -1;
    rtn = 0;
    while ((secnum = P_FindSectorFromLineTag(line,secnum)) >= 0)
    {
	sec = &sectors[secnum];
		
	// ALREADY MOVING?  IF SO, KEEP GOING...
	if (sec->specialdata)
	    continue;
	
	// new floor thinker
	rtn = 1;
	floor = (floormove_t*)Z_Malloc (sizeof(*floor), PU_LEVSPEC, 0);
	P_AddThinker (&floor->thinker);
	sec->specialdata = floor;
	floor->thinker.function.acp1 = (actionf_p1) T_MoveFloor;
	floor->type = floortype;
	floor->crush = false;

	switch(floortype)
	{
	  case lowerFloor:
	    floor->direction = -1;
	    floor->sector = sec;
	    floor->sspeed = FFLOORSPEED;
	    floor->ffloordestheight = PP_FindHighestFloorSurrounding(sec);
	    break;

	  case lowerFloorToLowest:
	    floor->direction = -1;
	    floor->sector = sec;
	    floor->sspeed = FFLOORSPEED;
	    floor->ffloordestheight = PP_FindLowestFloorSurrounding(sec);
	    break;

	  case turboLower:
	    floor->direction = -1;
	    floor->sector = sec;
	    floor->sspeed = FFLOORSPEED * 4;
	    floor->ffloordestheight = PP_FindHighestFloorSurrounding(sec);
	    if (floor->ffloordestheight != sec->ffloorheight)
            floor->ffloordestheight += 8;
	    break;

	  case raiseFloorCrush:
	    floor->crush = true;
	  case raiseFloor:
	    floor->direction = 1;
	    floor->sector = sec;
	    floor->sspeed = FFLOORSPEED;
	    floor->ffloordestheight =  PP_FindLowestCeilingSurrounding(sec);
	    if (floor->ffloordestheight > sec->cceilingheight)
		floor->ffloordestheight = sec->cceilingheight;
	    floor->ffloordestheight -= 8 * (floortype == raiseFloorCrush);
	    break;

	  case raiseFloorTurbo:
	    floor->direction = 1;
	    floor->sector = sec;
	    floor->sspeed = FFLOORSPEED*4;
	    floor->ffloordestheight = PP_FindNextHighestFloor(sec,sec->ffloorheight);
	    break;

	  case raiseFloorToNearest:
	    floor->direction = 1;
	    floor->sector = sec;
	    floor->sspeed = FFLOORSPEED;
	    floor->ffloordestheight = PP_FindNextHighestFloor(sec,sec->ffloorheight);
	    break;

	  case raiseFloor24:
          printf("raiseFloor24 probaly needs to fixed\n");
	    floor->direction = 1;
	    floor->sector = sec;
	    floor->sspeed = FFLOORSPEED;
        // @todo the fixed numbers 24, 512 seems fishy now that 
	    floor->ffloordestheight = floor->sector->ffloorheight + 24;
	    break;
	  case raiseFloor512:
          printf("raiseFloor512 probaly needs to fixed\n");
	    floor->direction = 1;
	    floor->sector = sec;
	    floor->sspeed = FFLOORSPEED;
	    floor->ffloordestheight = floor->sector->ffloorheight + 512;
	    break;

	  case raiseFloor24AndChange:
          printf("raiseFloor24AndCahnge probaly needs to fixed\n");
	    floor->direction = 1;
	    floor->sector = sec;
	    floor->sspeed = FFLOORSPEED;
	    floor->ffloordestheight = floor->sector->ffloorheight + 24;
	    sec->floorpic = line->frontsector->floorpic;
	    sec->special = line->frontsector->special;
	    break;

	  case raiseToTexture:
	  {
	      double	minsize = DBL_MAX;
	      Side*	side;
				
	      floor->direction = 1;
	      floor->sector = sec;
	      floor->sspeed = FFLOORSPEED;
	      for (i = 0; i < sec->linecount; i++)
	      {
		  if (twoSided (secnum, i) )
		  {
		      side = getSide(secnum,i,0);
		      if (side->bottomtexture >= 0)
			  if (ttextureheight[side->bottomtexture] < minsize)
			      minsize =  ttextureheight[side->bottomtexture];
		      side = getSide(secnum,i,1);
		      if (side->bottomtexture >= 0)
			  if (ttextureheight[side->bottomtexture] < minsize)
			      minsize = ttextureheight[side->bottomtexture];
		  }
	      }
	      floor->ffloordestheight = 
                  floor->sector->ffloorheight + minsize;
	  }
	  break;
	  
	  case lowerAndChange:
	    floor->direction = -1;
	    floor->sector = sec;
	    floor->sspeed = FFLOORSPEED;
	    floor->ffloordestheight = 
            PP_FindLowestFloorSurrounding(sec);
	    floor->texture = sec->floorpic;

	    for (i = 0; i < sec->linecount; i++)
	    {
		if ( twoSided(secnum, i) )
		{
		    if (getSide(secnum,i,0)->sector-&sectors[0] == secnum)
		    {
			sec = getSector(secnum,i,1);

			if (sec->ffloorheight == floor->ffloordestheight)
			{
			    floor->texture = sec->floorpic;
			    floor->newspecial = sec->special;
			    break;
			}
		    }
		    else
		    {
			sec = getSector(secnum,i,0);

			if (sec->ffloorheight == floor->ffloordestheight)
			{
			    floor->texture = sec->floorpic;
			    floor->newspecial = sec->special;
			    break;
			}
		    }
		}
	    }
	  default:
	    break;
	}
    }
    return rtn;
}




//
// BUILD A STAIRCASE!
//
int
EV_BuildStairs
( Line*	line,
  stair_e	type )
{
    int			secnum;
    double			height;
    int			i;
    int			newsecnum;
    int			texture;
    int			ok;
    int			rtn;
    
    Sector*		sec;
    Sector*		tsec;

    floormove_t*	floor;
    
    double stairsize;
    double speed;

    secnum = -1;
    rtn = 0;
    while ((secnum = P_FindSectorFromLineTag(line,secnum)) >= 0)
    {
	sec = &sectors[secnum];
		
	// ALREADY MOVING?  IF SO, KEEP GOING...
	if (sec->specialdata)
	    continue;
	
	// new floor thinker
	rtn = 1;
	floor = (floormove_t*)Z_Malloc (sizeof(*floor), PU_LEVSPEC, 0);
	P_AddThinker (&floor->thinker);
	sec->specialdata = floor;
	floor->thinker.function.acp1 = (actionf_p1) T_MoveFloor;
	floor->direction = 1;
	floor->sector = sec;
	switch(type)
	{
	  case build8:
	    speed = FFLOORSPEED/4;
	    stairsize = 8;
	    break;
	  case turbo16:
	    speed = FFLOORSPEED*4;
	    stairsize = 16;
	    break;
	}
	floor->sspeed = speed;
	height = sec->ffloorheight + stairsize;
	floor->ffloordestheight = height;
		
	texture = sec->floorpic;
	
	// Find next sector to raise
	// 1.	Find 2-sided line with same sector side[0]
	// 2.	Other side is the next sector to raise
	do
	{
	    ok = 0;
	    for (i = 0;i < sec->linecount;i++)
	    {
		if ( !((sec->lines[i])->flags & ML_TWOSIDED) )
		    continue;
					
		tsec = (sec->lines[i])->frontsector;
		newsecnum = tsec - &sectors[0];
		
		if (secnum != newsecnum)
		    continue;

		tsec = (sec->lines[i])->backsector;
		newsecnum = tsec - &sectors[0];

		if (tsec->floorpic != texture)
		    continue;
					
		height += stairsize;

		if (tsec->specialdata)
		    continue;
					
		sec = tsec;
		secnum = newsecnum;
		floor = (floormove_t*)Z_Malloc (sizeof(*floor), PU_LEVSPEC, 0);

		P_AddThinker (&floor->thinker);

		sec->specialdata = floor;
		floor->thinker.function.acp1 = (actionf_p1) T_MoveFloor;
		floor->direction = 1;
		floor->sector = sec;
		floor->sspeed = speed;
		floor->ffloordestheight = height;
		ok = 1;
		break;
	    }
	} while(ok);
    }
    return rtn;
}

