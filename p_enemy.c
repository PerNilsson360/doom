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
//	Enemy thinking, AI.
//	Action Pointer Functions
//	that are associated with states/frames. 
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: p_enemy.c,v 1.5 1997/02/03 22:45:11 b1 Exp $";

#include <stdlib.h>

#include "m_random.h"
#include "i_system.h"

#include "doomdef.h"
#include "p_local.h"

#include "s_sound.h"

#include "g_game.h"

// State.
#include "doomstat.h"
#include "r_state.h"

// Data.
#include "sounds.h"
#include "p_mobj.h"

//
// P_NewChaseDir related LUT.
//
dirtype_t opposite[] =
{
  DI_WEST, DI_SOUTHWEST, DI_SOUTH, DI_SOUTHEAST,
  DI_EAST, DI_NORTHEAST, DI_NORTH, DI_NORTHWEST, DI_NODIR
};

dirtype_t diags[] =
{
    DI_NORTHWEST, DI_NORTHEAST, DI_SOUTHWEST, DI_SOUTHEAST
};





void A_Fall (Mob *actor);


//
// ENEMY THINKING
// Enemies are allways spawned
// with targetplayer = -1, threshold = 0
// Most monsters are spawned unaware of all players,
// but some can be made preaware
//


//
// Called by P_NoiseAlert.
// Recursively traverse adjacent sectors,
// sound blocking lines cut off traversal.
//

Mob*		soundtarget;

void
P_RecursiveSound
( sector_t*	sec,
  int		soundblocks )
{
    int		i;
    line_t*	check;
    sector_t*	other;
	
    // wake up all monsters in this sector
    if (sec->validcount == validcount
	&& sec->soundtraversed <= soundblocks+1)
    {
	return;		// already flooded
    }
    
    sec->validcount = validcount;
    sec->soundtraversed = soundblocks+1;
    sec->soundtarget = soundtarget;
	
    for (i=0 ;i<sec->linecount ; i++)
    {
	check = sec->lines[i];
	if (! (check->flags & ML_TWOSIDED) )
	    continue;
	
	P_LineOpening (check);

	if (oopenrange <= 0)
	    continue;	// closed door
	
	if ( sides[ check->sidenum[0] ].sector == sec)
	    other = sides[ check->sidenum[1] ] .sector;
	else
	    other = sides[ check->sidenum[0] ].sector;
	
	if (check->flags & ML_SOUNDBLOCK)
	{
	    if (!soundblocks)
		P_RecursiveSound (other, 1);
	}
	else
	    P_RecursiveSound (other, soundblocks);
    }
}



//
// P_NoiseAlert
// If a monster yells at a player,
// it will alert other monsters to the player.
//
void
P_NoiseAlert
( Mob*	target,
  Mob*	emmiter )
{
    soundtarget = target;
    validcount++;
    P_RecursiveSound (emmiter->subsector->sector, 0);
}




//
// P_CheckMeleeRange
//
bool 
P_CheckMeleeRange(Mob* actor)
{
    Mob*	pl;
    double dist;
	
    if (!actor->target)
        return false;
    
    pl = actor->target;

    dist = PP_AproxDistance(pl->xx - actor->xx, pl->yy - actor->yy);
    
    if (dist >= MMELEERANGE - 20 + pl->info->rradius)
        return false;
	
    if (!P_CheckSight (actor, actor->target) )
        return false;
    
    return true;		
}

//
// P_CheckMissileRange
//
bool P_CheckMissileRange (Mob* actor)
{
    double dist;
	
    if (! P_CheckSight (actor, actor->target) )
        return false;
	
    if ( actor->flags & MF_JUSTHIT )
    {
        // the target just hit the enemy,
        // so fight back!
        actor->flags &= ~MF_JUSTHIT;
        return true;
    }
	
    if (actor->reactiontime)
        return false;	// do not attack yet
    
    // OPTIMIZE: get this from a global checksight
    dist = PP_AproxDistance(actor->xx - actor->target->xx,
                            actor->yy - actor->target->yy) - 64;
    
    if (!actor->info->meleestate)
        dist -= 128;	// no melee attack, so fire more
    
    // @todo these hardcoded dists are 
    // mod likely wrong now with double
    if (actor->type == MT_VILE)
    {
        if (dist > 14*64)	
            return false;	// too far away
    }
	
    
    if (actor->type == MT_UNDEAD)
    {
        if (dist < 196)	
            return false;	// close for fist attack
        dist /= 2;
    }
	
    
    if (actor->type == MT_CYBORG
        || actor->type == MT_SPIDER
        || actor->type == MT_SKULL)
    {
        dist /= 2;
    }
    
    if (dist > 200)
        dist = 200;
    
    if (actor->type == MT_CYBORG && dist > 160)
        dist = 160;
    
    if (P_Random () < dist)
        return false;
    
    return true;
}


//
// P_Move
// Move in the current direction,
// returns false if the move is blocked.
//
double xxspeed[8] = {1,0.717163,0,-0.717163,-1,-0.717163,0,0.717163};
double yyspeed[8] = {0,0.717163,1,0.717163,0,-0.717163,-1,-0.717163};


#define MAXSPECIALCROSS	8

extern	line_t*	spechit[MAXSPECIALCROSS];
extern	int	numspechit;

bool P_Move (Mob*	actor)
{
    line_t*	ld;
    
    // warning: 'catch', 'throw', and 'try'
    // are all C++ reserved words
    bool	try_ok;
    bool	good;
		
    if (actor->movedir == DI_NODIR)
        return false;
    
    if ((unsigned)actor->movedir >= 8)
        I_Error ("Weird actor->movedir!");
		
    double tryx = actor->xx + (actor->info->speed*xxspeed[actor->movedir]);
    double tryy = actor->yy + (actor->info->speed*yyspeed[actor->movedir]);
    try_ok = PP_TryMove (actor, tryx, tryy);
    
    if (!try_ok)
    {
        // open any specials
        if (actor->flags & MF_FLOAT && floatok)
        {
            // must adjust height
            if (actor->zz < ttmfloorz)
                actor->zz += FFLOATSPEED;
            else
                actor->zz -= FFLOATSPEED;
            
            actor->flags |= MF_INFLOAT;
            return true;
        }
		
        if (!numspechit)
            return false;
        
        actor->movedir = DI_NODIR;
        good = false;
        while (numspechit--)
        {
            ld = spechit[numspechit];
            // if the special is not a door
            // that can be opened,
            // return false
            if (P_UseSpecialLine (actor, ld,0))
                good = true;
        }
        return good;
    }
    else
    {
        actor->flags &= ~MF_INFLOAT;
    }
	
	
    if (! (actor->flags & MF_FLOAT) )	
        actor->zz = actor->ffloorz;
    return true; 
}


//
// TryWalk
// Attempts to move actor on
// in its current (ob->moveangle) direction.
// If blocked by either a wall or an actor
// returns FALSE
// If move is either clear or blocked only by a door,
// returns TRUE and sets...
// If a door is in the way,
// an OpenDoor call is made to start it opening.
//
bool P_TryWalk (Mob* actor)
{	
    if (!P_Move (actor))
    {
	return false;
    }

    actor->movecount = P_Random()&15;
    return true;
}




void P_NewChaseDir (Mob*	actor)
{
    dirtype_t	d[3];
    
    int	tdir;
    dirtype_t	olddir;
    
    dirtype_t	turnaround;

    if (!actor->target)
        I_Error ("P_NewChaseDir: called with no target");
		
    olddir = actor->movedir;
    turnaround=opposite[olddir];

    double deltax = actor->target->xx - actor->xx;
    double deltay = actor->target->yy - actor->yy;

    if (deltax>10)
        d[1]= DI_EAST;
    else if (deltax<-10)
        d[1]= DI_WEST;
    else
        d[1]=DI_NODIR;

    if (deltay<-10)
        d[2]= DI_SOUTH;
    else if (deltay>10)
        d[2]= DI_NORTH;
    else
        d[2]=DI_NODIR;

    // try direct route
    if (d[1] != DI_NODIR && 
        d[2] != DI_NODIR)
    {
        actor->movedir = diags[((deltay<0)<<1)+(deltax>0)];
        if (actor->movedir != turnaround && P_TryWalk(actor))
            return;
    }

    // try other directions
    if (P_Random() > 200
	||  abs(deltay)>abs(deltax))
    {
	tdir=d[1];
	d[1]=d[2];
	d[2]=(dirtype_t)tdir;
    }

    if (d[1]==turnaround)
	d[1]=DI_NODIR;
    if (d[2]==turnaround)
	d[2]=DI_NODIR;
	
    if (d[1]!=DI_NODIR)
    {
	actor->movedir = d[1];
	if (P_TryWalk(actor))
	{
	    // either moved forward or attacked
	    return;
	}
    }

    if (d[2]!=DI_NODIR)
    {
	actor->movedir =d[2];

	if (P_TryWalk(actor))
	    return;
    }

    // there is no direct path to the player,
    // so pick another direction.
    if (olddir!=DI_NODIR)
    {
	actor->movedir =olddir;

	if (P_TryWalk(actor))
	    return;
    }

    // randomly determine direction of search
    if (P_Random()&1) 	
    {
	for ( tdir=DI_EAST;
	      tdir<=DI_SOUTHEAST;
	      tdir++ )
	{
	    if (tdir!=turnaround)
	    {
            actor->movedir = (dirtype_t)tdir;
		
		if ( P_TryWalk(actor) )
		    return;
	    }
	}
    }
    else
    {
	for ( tdir=DI_SOUTHEAST;
	      tdir != (DI_EAST-1);
	      tdir-- )
	{
	    if (tdir!=turnaround)
	    {
		actor->movedir = (dirtype_t)tdir;
		
		if ( P_TryWalk(actor) )
		    return;
	    }
	}
    }

    if (turnaround !=  DI_NODIR)
    {
	actor->movedir =turnaround;
	if ( P_TryWalk(actor) )
	    return;
    }

    actor->movedir = DI_NODIR;	// can not move
}



//
// P_LookForPlayers
// If allaround is false, only look 180 degrees in front.
// Returns true if a player is targeted.
//
bool
P_LookForPlayers
( Mob*	actor,
  bool	allaround )
{
    int		c;
    int		stop;
    player_t*	player;
 	
    c = 0;
    stop = (actor->lastlook-1)&3;
	
    for ( ; ; actor->lastlook = (actor->lastlook+1)&3 )
    {
        if (!playeringame[actor->lastlook])
            continue;
        
        if (c++ == 2
            || actor->lastlook == stop)
        {
            // done looking
            return false;	
        }
        
        player = &players[actor->lastlook];
        
        if (player->health <= 0)
            continue;		// dead
        
        if (!P_CheckSight (actor, player->mo)) 
            continue;		// out of sight
        
        if (!allaround)
        {
            double actorPlayerAngle = (double) Angle(actor->xx,
                                                     actor->yy,
                                                     player->mo->xx,
                                                     player->mo->yy);
            double an = actorPlayerAngle - (double) actor->_angle;
            if (an > Angle::A90 && an < Angle::A270)
            {
                double dist = PP_AproxDistance(player->mo->xx - actor->xx,
                                               player->mo->yy - actor->yy);
                // if real close, react anyway
                if (dist > MMELEERANGE)
                    continue;	// behind back
            }
        }
        actor->target = player->mo;
        return true;
    }
    
    return false;
}


//
// A_KeenDie
// DOOM II special, map 32.
// Uses special tag 666.
//
void A_KeenDie (Mob* mo)
{
    thinker_t*	th;
    Mob*	mo2;
    line_t	junk;

    A_Fall (mo);
    
    // scan the remaining thinkers
    // to see if all Keens are dead
    for (th = thinkercap.next ; th != &thinkercap ; th=th->next)
    {
	if (th->function.acp1 != (actionf_p1)P_MobjThinker)
	    continue;

	mo2 = (Mob *)th;
	if (mo2 != mo
	    && mo2->type == mo->type
	    && mo2->health > 0)
	{
	    // other Keen not dead
	    return;		
	}
    }

    junk.tag = 666;
    EV_DoDoor(&junk,open);
}


//
// ACTION ROUTINES
//

//
// A_Look
// Stay in state until a player is sighted.
//
void A_Look (Mob* actor)
{
    Mob*	targ;
	
    actor->threshold = 0;	// any shot will wake up
    targ = actor->subsector->sector->soundtarget;

    if (targ
	&& (targ->flags & MF_SHOOTABLE) )
    {
	actor->target = targ;

	if ( actor->flags & MF_AMBUSH )
	{
	    if (P_CheckSight (actor, actor->target))
        {
            goto seeyou;
        }
	}
	else
	    goto seeyou;
    }
	
	
    if (!P_LookForPlayers (actor, false) ) 
        return;

    // go into chase state
  seeyou:
    if (actor->info->seesound)
    {
	int		sound;
		
	switch (actor->info->seesound)
	{
	  case sfx_posit1:
	  case sfx_posit2:
	  case sfx_posit3:
	    sound = sfx_posit1+P_Random()%3;
	    break;

	  case sfx_bgsit1:
	  case sfx_bgsit2:
	    sound = sfx_bgsit1+P_Random()%2;
	    break;

	  default:
	    sound = actor->info->seesound;
	    break;
	}

	if (actor->type==MT_SPIDER
	    || actor->type == MT_CYBORG)
	{
	    // full volume
	    S_StartSound (NULL, sound);
	}
	else
	    S_StartSound (actor, sound);
    }

    actor->setState((statenum_t)actor->info->seestate);
}


//
// A_Chase
// Actor has a melee attack,
// so it tries to close as fast as possible
//
void A_Chase (Mob*	actor)
{
    if (actor->reactiontime)
        actor->reactiontime--;
    
    
    // modify target threshold
    if  (actor->threshold)
    {
        if (!actor->target
            || actor->target->health <= 0)
        {
            actor->threshold = 0;
        }
        else
            actor->threshold--;
    }
    
    // turn towards movement direction if not there yet
    if (actor->movedir < 8)
    {
        angle_t angle = actor->_angle;
	// @todo this seems strange
	angle &= (7<<29);
        actor->_angle = angle;
	Angle moveAngle((angle_t)(actor->movedir << 29));
        double delta = (double)actor->_angle - (double)moveAngle;
	
        if (delta > Angle::A0)
	    actor->_angle -= Angle(Angle::A45);
        else if (delta < Angle::A0)
	    actor->_angle += Angle(Angle::A45);
    }
    
    if (!actor->target
        || !(actor->target->flags&MF_SHOOTABLE))
    {
        // look for a new target
        if (P_LookForPlayers(actor,true))
            return; 	// got a new target
        
        actor->setState((statenum_t)actor->info->spawnstate);
        return;
    }
    
    // do not attack twice in a row
    if (actor->flags & MF_JUSTATTACKED)
    {
	actor->flags &= ~MF_JUSTATTACKED;
	if (gameskill != sk_nightmare && !fastparm)
	    P_NewChaseDir (actor);
	return;
    }
    
    // check for melee attack
    if (actor->info->meleestate
	&& P_CheckMeleeRange (actor))
    {
	if (actor->info->attacksound)
	    S_StartSound (actor, actor->info->attacksound);

	actor->setState((statenum_t)actor->info->meleestate);
	return;
    }
    
    // check for missile attack
    if (actor->info->missilestate)
    {
	if (gameskill < sk_nightmare
	    && !fastparm && actor->movecount)
	{
	    goto nomissile;
	}
	
	if (!P_CheckMissileRange (actor))
	    goto nomissile;
	
	actor->setState((statenum_t)actor->info->missilestate);
	actor->flags |= MF_JUSTATTACKED;
	return;
    }

    // ?
  nomissile:
    // possibly choose another target
    if (netgame
	&& !actor->threshold
	&& !P_CheckSight (actor, actor->target) )
    {
	if (P_LookForPlayers(actor,true))
	    return;	// got a new target
    }
    
    // chase towards player
    if (--actor->movecount<0
	|| !P_Move (actor))
    {
	P_NewChaseDir (actor);
    }
    
    // make active sound
    if (actor->info->activesound
	&& P_Random () < 3)
    {
	S_StartSound (actor, actor->info->activesound);
    }
}


//
// A_FaceTarget
//
void A_FaceTarget (Mob* actor)
{	
    if (!actor->target)
        return;
    actor->flags &= ~MF_AMBUSH;
    actor->_angle = Angle(actor->xx,
                          actor->yy,
                          actor->target->xx,
                          actor->target->yy);
    if (actor->target->flags & MF_SHADOW)
        actor->_angle += (angle_t)((P_Random()-P_Random()) << 21) * ANG45;
}


//
// A_PosAttack
//
void A_PosAttack (Mob* actor)
{
    if (!actor->target)
	return;
		
    A_FaceTarget (actor);
    Angle angle = actor->_angle;
    double slope = PP_AimLineAttack(actor, angle, MMISSILERANGE);

    S_StartSound (actor, sfx_pistol);
    angle += (angle_t)(P_Random()-P_Random())<<20;
    int damage = ((P_Random()%5)+1)*3;
    PP_LineAttack(actor, angle, MMISSILERANGE, slope, damage);
}

void A_SPosAttack (Mob* actor)
{
    if (!actor->target)
	return;

    S_StartSound (actor, sfx_shotgn);
    A_FaceTarget (actor);
    Angle bangle = actor->_angle;
    double slope = PP_AimLineAttack (actor, bangle, MMISSILERANGE);

    for (int i=0 ; i<3 ; i++)
    {
	Angle angle = bangle + Angle((angle_t)(P_Random()-P_Random())<<20);
	int damage = ((P_Random()%5)+1)*3;
	PP_LineAttack (actor, angle, MMISSILERANGE, slope, damage);
    }
}

void A_CPosAttack (Mob* actor)
{
	
    if (!actor->target)
	return;

    S_StartSound (actor, sfx_shotgn);
    A_FaceTarget (actor);
    Angle bangle = actor->_angle;
    double slope = PP_AimLineAttack (actor, bangle, MMISSILERANGE);

    Angle angle = bangle + Angle((angle_t)((P_Random()-P_Random())<<20));
    int damage = ((P_Random()%5)+1)*3;
    PP_LineAttack(actor, angle, MMISSILERANGE, slope, damage);
}

void A_CPosRefire (Mob* actor)
{	
    // keep firing unless target got out of sight
    A_FaceTarget (actor);

    if (P_Random () < 40)
	return;

    if (!actor->target
        || actor->target->health <= 0
        || !P_CheckSight (actor, actor->target) )
    {
        actor->setState((statenum_t)actor->info->seestate);
}
}


void A_SpidRefire (Mob* actor)
{	
    // keep firing unless target got out of sight
    A_FaceTarget (actor);

    if (P_Random () < 10)
	return;

    if (!actor->target
	|| actor->target->health <= 0
	|| !P_CheckSight (actor, actor->target) )
    {
        actor->setState((statenum_t)actor->info->seestate);
    }
}

void A_BspiAttack (Mob *actor)
{	
    if (!actor->target)
	return;
		
    A_FaceTarget (actor);

    // launch a missile
    P_SpawnMissile (actor, actor->target, MT_ARACHPLAZ);
}


//
// A_TroopAttack
//
void A_TroopAttack (Mob* actor)
{
    int		damage;
	
    if (!actor->target)
	return;
		
    A_FaceTarget (actor);
    if (P_CheckMeleeRange (actor))
    {
	S_StartSound (actor, sfx_claw);
	damage = (P_Random()%8+1)*3;
	P_DamageMobj (actor->target, actor, actor, damage);
	return;
    }

    
    // launch a missile
    P_SpawnMissile(actor, actor->target, MT_TROOPSHOT);
}


void A_SargAttack (Mob* actor)
{
    int		damage;

    if (!actor->target)
	return;
		
    A_FaceTarget (actor);
    if (P_CheckMeleeRange (actor))
    {
	damage = ((P_Random()%10)+1)*4;
	P_DamageMobj (actor->target, actor, actor, damage);
    }
}

void A_HeadAttack (Mob* actor)
{
    int		damage;
	
    if (!actor->target)
	return;
		
    A_FaceTarget (actor);
    if (P_CheckMeleeRange (actor))
    {
	damage = (P_Random()%6+1)*10;
	P_DamageMobj (actor->target, actor, actor, damage);
	return;
    }
    
    // launch a missile
    P_SpawnMissile (actor, actor->target, MT_HEADSHOT);
}

void A_CyberAttack (Mob* actor)
{	
    if (!actor->target)
	return;
		
    A_FaceTarget (actor);
    P_SpawnMissile (actor, actor->target, MT_ROCKET);
}


void A_BruisAttack (Mob* actor)
{
    int		damage;
	
    if (!actor->target)
	return;
		
    if (P_CheckMeleeRange (actor))
    {
	S_StartSound (actor, sfx_claw);
	damage = (P_Random()%8+1)*10;
	P_DamageMobj (actor->target, actor, actor, damage);
	return;
    }
    
    // launch a missile
    P_SpawnMissile (actor, actor->target, MT_BRUISERSHOT);
}


//
// A_SkelMissile
//
void A_SkelMissile (Mob* actor)
{	
    Mob* mo;
	
    if (!actor->target)
        return;
		
    A_FaceTarget(actor);
    actor->zz += 16;	// so missile spawns higher
    mo = P_SpawnMissile (actor, actor->target, MT_TRACER);
    actor->zz -= 16;	// back to normal

    mo->xx += mo->mmomx;
    mo->yy += mo->mmomy;
    mo->tracer = actor->target;
}

namespace {
const Angle TRACEANGLE = Angle::A270;
}

void A_Tracer (Mob* actor)
{
    Mob*	dest;
    Mob*	th;
		
    if (gametic & 3)
	return;
    
    // spawn a puff of smoke behind the rocket		
    PP_SpawnPuff (actor->xx, actor->yy, actor->zz);
	
    th = PP_SpawnMobj(actor->xx - actor->mmomx,
                      actor->yy - actor->mmomy,
                      actor->zz, 
                      MT_SMOKE);
    
    th->mmomz = 1;
    th->tics -= P_Random()&3;
    if (th->tics < 1)
	th->tics = 1;
    
    // adjust direction
    dest = actor->tracer;
	
    if (!dest || dest->health <= 0)
	return;
    
    // change angle	
    Angle exact =  Angle(actor->xx, actor->yy,
			 dest->xx, dest->yy);

    if (exact != actor->_angle)
    {
        if (exact - actor->_angle > Angle::A180)
        {
            actor->_angle -= TRACEANGLE;
            if (exact - actor->_angle <Angle::A180)
                actor->_angle = exact;
        }
        else
        {
            actor->_angle += TRACEANGLE;
            if (exact - actor->_angle > Angle::A180)
                actor->_angle = exact;
        }
    }
    
    actor->mmomx = actor->info->speed * cos(actor->_angle);
    actor->mmomy = actor->info->speed * sin(actor->_angle);
    
    // change slope
    double dist = PP_AproxDistance(dest->xx - actor->xx,
                                   dest->yy - actor->yy);
    
    dist = dist / actor->info->speed;

    if (dist < 1)
        dist = 1;

    double slope = dest->zz + 40 - actor->zz / dist;
    
    if (slope < actor->mmomz)
        actor->mmomz -= 1/8;
    else
        actor->mmomz += 1/8;
}


void A_SkelWhoosh (Mob*	actor)
{
    if (!actor->target)
	return;
    A_FaceTarget (actor);
    S_StartSound (actor,sfx_skeswg);
}

void A_SkelFist (Mob*	actor)
{
    int		damage;

    if (!actor->target)
	return;
		
    A_FaceTarget (actor);
	
    if (P_CheckMeleeRange (actor))
    {
	damage = ((P_Random()%10)+1)*6;
	S_StartSound (actor, sfx_skepch);
	P_DamageMobj (actor->target, actor, actor, damage);
    }
}



//
// PIT_VileCheck
// Detect a corpse that could be raised.
//
Mob*		corpsehit;
Mob*		vileobj;
double vviletryx;
double vviletryy;

bool PIT_VileCheck (Mob*	thing)
{
    printf("PIT_VileCheck\n");
    double maxdist;
    bool check;
	
    if (!(thing->flags & MF_CORPSE) )
        return true;	// not a monster
    
    if (thing->tics != -1)
        return true;	// not lying still yet
    
    if (thing->info->raisestate == S_NULL)
        return true;	// monster doesn't have a raise state
    
    maxdist = thing->info->rradius + mobjinfo[MT_VILE].rradius;
	
    if (fabs(thing->xx - vviletryx) > maxdist
        || fabs(thing->yy - vviletryy) > maxdist)
        return true;		// not actually touching
    
    corpsehit = thing;
    corpsehit->mmomx = corpsehit->mmomy = 0;
    corpsehit->hheight *= 4;
    check = PP_CheckPosition(corpsehit, corpsehit->xx, corpsehit->yy);
    corpsehit->hheight *= 4;

    if (!check)
        return true;		// doesn't fit here
    
    return false;		// got one, so stop checking
}



//
// A_VileChase
// Check for ressurecting a body
//
void A_VileChase (Mob* actor)
{
    int			xl;
    int			xh;
    int			yl;
    int			yh;
    
    int			bx;
    int			by;

    mobjinfo_t*		info;
    Mob*		temp;

    printf("A_VileChase\n");
    
    if (actor->movedir != DI_NODIR)
    {
        // check for corpses to raise
        // @todo check the actor speed thing below
        vviletryx = actor->xx + actor->info->speed*xxspeed[actor->movedir];
        vviletryy = actor->yy + actor->info->speed*yyspeed[actor->movedir];
        
        xl = (vviletryx - blockMap.oorgx - MMAXRADIUS*2) / DOUBLE_MAPBLOCKS_DIV;
        xh = (vviletryx - blockMap.oorgx + MMAXRADIUS*2) / DOUBLE_MAPBLOCKS_DIV;
        yl = (vviletryy - blockMap.oorgy - MMAXRADIUS*2) / DOUBLE_MAPBLOCKS_DIV;
        yh = (vviletryy - blockMap.oorgy + MMAXRADIUS*2) / DOUBLE_MAPBLOCKS_DIV;
        
	vileobj = actor;
	for (bx=xl ; bx<=xh ; bx++)
	{
	    for (by=yl ; by<=yh ; by++)
	    {
		// Call PIT_VileCheck to check
		// whether object is a corpse
		// that canbe raised.
		if (!P_BlockThingsIterator(bx,by,PIT_VileCheck))
		{
		    // got one!
		    temp = actor->target;
		    actor->target = corpsehit;
		    A_FaceTarget (actor);
		    actor->target = temp;
					
		    actor->setState(S_VILE_HEAL1);
		    S_StartSound (corpsehit, sfx_slop);
		    info = corpsehit->info;
		    
		    corpsehit->setState((statenum_t)info->raisestate);
		    corpsehit->hheight *= 4;
		    corpsehit->flags = info->flags;
		    corpsehit->health = info->spawnhealth;
		    corpsehit->target = NULL;

		    return;
		}
	    }
	}
    }

    // Return to normal attack.
    A_Chase (actor);
}


//
// A_VileStart
//
void A_VileStart (Mob* actor)
{
    S_StartSound (actor, sfx_vilatk);
}


//
// A_Fire
// Keep fire in front of player unless out of sight
//
void A_Fire (Mob* actor);

void A_StartFire (Mob* actor)
{
    S_StartSound(actor,sfx_flamst);
    A_Fire(actor);
}

void A_FireCrackle (Mob* actor)
{
    S_StartSound(actor,sfx_flame);
    A_Fire(actor);
}

void A_Fire (Mob* actor)
{
    Mob*	dest;
		
    dest = actor->tracer;
    if (!dest)
	return;
		
    // don't move it if the vile lost sight
    if (!P_CheckSight (actor->target, dest) )
	return;

    P_UnsetThingPosition (actor);
    actor->xx = dest->xx + (24 * cos(dest->_angle));
    actor->yy = dest->yy + (24 * sin(dest->_angle));
    actor->zz = dest->zz;
    P_SetThingPosition (actor);
}



//
// A_VileTarget
// Spawn the hellfire
//
void A_VileTarget (Mob*	actor)
{
    printf("A_VileTarget\n");
    Mob*	fog;
	
    if (!actor->target)
        return;

    A_FaceTarget (actor);

    fog = PP_SpawnMobj(actor->target->xx,
                       actor->target->xx, // @todo strange should it not be y here
                       actor->target->zz, 
                       MT_FIRE);
    
    actor->tracer = fog;
    fog->target = actor;
    fog->tracer = actor->target;
    A_Fire (fog);
}




//
// A_VileAttack
//
void A_VileAttack (Mob* actor)
{	
    Mob*	fire;
	
    if (!actor->target)
	return;
    
    A_FaceTarget (actor);

    if (!P_CheckSight (actor, actor->target))
	return;

    S_StartSound (actor, sfx_barexp);
    P_DamageMobj (actor->target, actor, actor, 20);
    actor->target->mmomz = 1000/actor->target->info->mass;
	
    fire = actor->tracer;

    if (!fire)
	return;
		
    // move the fire between the vile and the player
    fire->xx = actor->target->xx - (24 * cos(actor->_angle));
    fire->yy = actor->target->yy - (24 * sin(actor->_angle));
    P_RadiusAttack (fire, actor, 70);
}




//
// Mancubus attack,
// firing three missiles (bruisers)
// in three different directions?
// Doesn't look like it. 
//
namespace {
    const Angle FATSPREAD(Angle::A90/8);
}

void A_FatRaise (Mob *actor)
{
    A_FaceTarget (actor);
    S_StartSound (actor, sfx_manatk);
}


void A_FatAttack1 (Mob* actor)
{
    Mob*	mo;
	
    A_FaceTarget (actor);
    // Change direction  to ...
    actor->_angle += FATSPREAD;
    P_SpawnMissile (actor, actor->target, MT_FATSHOT);

    mo = P_SpawnMissile (actor, actor->target, MT_FATSHOT);
    mo->_angle += FATSPREAD;
    mo->mmomx = mo->info->speed * cos(mo->_angle);
    mo->mmomy = mo->info->speed * sin(mo->_angle);
}

void A_FatAttack2 (Mob* actor)
{
    Mob*	mo;

    A_FaceTarget (actor);
    // Now here choose opposite deviation.
    actor->_angle -= FATSPREAD;
    P_SpawnMissile (actor, actor->target, MT_FATSHOT);

    mo = P_SpawnMissile (actor, actor->target, MT_FATSHOT);
    mo->_angle -= FATSPREAD*2;
    mo->mmomx = mo->info->speed * cos(mo->_angle);
    mo->mmomy = mo->info->speed * sin(mo->_angle);
}

void A_FatAttack3 (Mob*	actor)
{
    Mob*	mo;

    A_FaceTarget (actor);
    
    mo = P_SpawnMissile (actor, actor->target, MT_FATSHOT);
    mo->_angle -= FATSPREAD/2;
    mo->mmomx = mo->info->speed * cos(mo->_angle);
    mo->mmomy = mo->info->speed * sin(mo->_angle);

    mo = P_SpawnMissile (actor, actor->target, MT_FATSHOT);
    mo->_angle += FATSPREAD/2;
    mo->mmomx = mo->info->speed * cos(mo->_angle);
    mo->mmomy = mo->info->speed * sin(mo->_angle);
}


//
// SkullAttack
// Fly at the player like a missile.
//
#define	SSKULLSPEED		20

void A_SkullAttack (Mob* actor)
{
    if (!actor->target)
        return;
    Mob* dest = actor->target;	
    actor->flags |= MF_SKULLFLY;
    S_StartSound (actor, actor->info->attacksound);
    A_FaceTarget (actor);
    actor->mmomx = SSKULLSPEED * cos(actor->_angle);
    actor->mmomy = SSKULLSPEED * sin(actor->_angle);
    double dist = PP_AproxDistance(dest->xx - actor->xx, dest->yy - actor->yy);
    dist = dist / SSKULLSPEED;
    if (dist < 1)
        dist = 1;
    actor->mmomz = dest->zz + (dest->hheight / 2) - actor->zz / dist;
}


//
// A_PainShootSkull
// Spawn a lost soul and launch it at the target
//
void
AA_PainShootSkull(Mob* actor, double angle)
{
    Mob*	newmobj;
    double	prestep;
    int		count;
    thinker_t*	currentthinker;

    // count total number of skull currently on the level
    count = 0;

    currentthinker = thinkercap.next;
    while (currentthinker != &thinkercap)
    {
        if ((currentthinker->function.acp1 == (actionf_p1)P_MobjThinker)
            && ((Mob *)currentthinker)->type == MT_SKULL)
            count++;
        currentthinker = currentthinker->next;
    }

    // if there are allready 20 skulls on the level,
    // don't spit another one
    if (count > 20)
        return;


    // okay, there's playe for another one
    prestep = 4 + 3*(actor->info->rradius + mobjinfo[MT_SKULL].rradius)/2;
    
    double x = actor->xx + prestep * cos(angle);
    double y = actor->yy + prestep * sin(angle);
    double z = actor->zz + 8; // @todo check this 8 fishy
		
    newmobj = PP_SpawnMobj(x , y, z, MT_SKULL);

    // Check for movements.
    if (!PP_TryMove(newmobj, newmobj->xx, newmobj->yy))
    {
        // kill it immediately
        P_DamageMobj (newmobj,actor,actor,10000);	
        return;
    }
		
    newmobj->target = actor->target;
    A_SkullAttack (newmobj);
}


//
// A_PainAttack
// Spawn a lost soul and launch it at the target
// 
void A_PainAttack (Mob* actor)
{
    if (!actor->target)
	return;

    A_FaceTarget(actor);
    AA_PainShootSkull(actor, actor->_angle);
}


void A_PainDie (Mob* actor)
{
    A_Fall (actor);
    AA_PainShootSkull(actor, actor->_angle + Angle(Angle::A90));
    AA_PainShootSkull(actor, actor->_angle + Angle(Angle::A180));
    AA_PainShootSkull(actor, actor->_angle + Angle(Angle::A270));
}

void A_Scream (Mob* actor)
{
    int		sound;
	
    switch (actor->info->deathsound)
    {
      case 0:
	return;
		
      case sfx_podth1:
      case sfx_podth2:
      case sfx_podth3:
	sound = sfx_podth1 + P_Random ()%3;
	break;
		
      case sfx_bgdth1:
      case sfx_bgdth2:
	sound = sfx_bgdth1 + P_Random ()%2;
	break;
	
      default:
	sound = actor->info->deathsound;
	break;
    }

    // Check for bosses.
    if (actor->type==MT_SPIDER
	|| actor->type == MT_CYBORG)
    {
	// full volume
	S_StartSound (NULL, sound);
    }
    else
	S_StartSound (actor, sound);
}


void A_XScream (Mob* actor)
{
    S_StartSound (actor, sfx_slop);	
}

void A_Pain (Mob* actor)
{
    if (actor->info->painsound)
	S_StartSound (actor, actor->info->painsound);	
}



void A_Fall (Mob *actor)
{
    // actor is on ground, it can be walked over
    actor->flags &= ~MF_SOLID;

    // So change this if corpse objects
    // are meant to be obstacles.
}


//
// A_Explode
//
void A_Explode (Mob* thingy)
{
    P_RadiusAttack ( thingy, thingy->target, 128 );
}


//
// A_BossDeath
// Possibly trigger special effects
// if on first boss level
//
void A_BossDeath (Mob* mo)
{
    thinker_t*	th;
    Mob*	mo2;
    line_t	junk;
    int		i;
		
    if ( gamemode == commercial)
    {
	if (gamemap != 7)
	    return;
		
	if ((mo->type != MT_FATSO)
	    && (mo->type != MT_BABY))
	    return;
    }
    else
    {
	switch(gameepisode)
	{
	  case 1:
	    if (gamemap != 8)
		return;

	    if (mo->type != MT_BRUISER)
		return;
	    break;
	    
	  case 2:
	    if (gamemap != 8)
		return;

	    if (mo->type != MT_CYBORG)
		return;
	    break;
	    
	  case 3:
	    if (gamemap != 8)
		return;
	    
	    if (mo->type != MT_SPIDER)
		return;
	    
	    break;
	    
	  case 4:
	    switch(gamemap)
	    {
	      case 6:
		if (mo->type != MT_CYBORG)
		    return;
		break;
		
	      case 8: 
		if (mo->type != MT_SPIDER)
		    return;
		break;
		
	      default:
		return;
		break;
	    }
	    break;
	    
	  default:
	    if (gamemap != 8)
		return;
	    break;
	}
		
    }

    
    // make sure there is a player alive for victory
    for (i=0 ; i<MAXPLAYERS ; i++)
	if (playeringame[i] && players[i].health > 0)
	    break;
    
    if (i==MAXPLAYERS)
	return;	// no one left alive, so do not end game
    
    // scan the remaining thinkers to see
    // if all bosses are dead
    for (th = thinkercap.next ; th != &thinkercap ; th=th->next)
    {
	if (th->function.acp1 != (actionf_p1)P_MobjThinker)
	    continue;
	
	mo2 = (Mob *)th;
	if (mo2 != mo
	    && mo2->type == mo->type
	    && mo2->health > 0)
	{
	    // other boss not dead
	    return;
	}
    }
	
    // victory!
    if ( gamemode == commercial)
    {
	if (gamemap == 7)
	{
	    if (mo->type == MT_FATSO)
	    {
		junk.tag = 666;
		EV_DoFloor(&junk,lowerFloorToLowest);
		return;
	    }
	    
	    if (mo->type == MT_BABY)
	    {
		junk.tag = 667;
		EV_DoFloor(&junk,raiseToTexture);
		return;
	    }
	}
    }
    else
    {
	switch(gameepisode)
	{
	  case 1:
	    junk.tag = 666;
	    EV_DoFloor (&junk, lowerFloorToLowest);
	    return;
	    break;
	    
	  case 4:
	    switch(gamemap)
	    {
	      case 6:
		junk.tag = 666;
		EV_DoDoor (&junk, blazeOpen);
		return;
		break;
		
	      case 8:
		junk.tag = 666;
		EV_DoFloor (&junk, lowerFloorToLowest);
		return;
		break;
	    }
	}
    }
	
    G_ExitLevel ();
}


void A_Hoof (Mob* mo)
{
    S_StartSound (mo, sfx_hoof);
    A_Chase (mo);
}

void A_Metal (Mob* mo)
{
    S_StartSound (mo, sfx_metal);
    A_Chase (mo);
}

void A_BabyMetal (Mob* mo)
{
    S_StartSound (mo, sfx_bspwlk);
    A_Chase (mo);
}

void
A_OpenShotgun2
( player_t*	player,
  pspdef_t*	psp )
{
    S_StartSound (player->mo, sfx_dbopn);
}

void
A_LoadShotgun2
( player_t*	player,
  pspdef_t*	psp )
{
    S_StartSound (player->mo, sfx_dbload);
}

void
A_ReFire
( player_t*	player,
  pspdef_t*	psp );

void
A_CloseShotgun2
( player_t*	player,
  pspdef_t*	psp )
{
    S_StartSound (player->mo, sfx_dbcls);
    A_ReFire(player,psp);
}



Mob*		braintargets[32];
int		numbraintargets;
int		braintargeton;

void A_BrainAwake (Mob* mo)
{
    thinker_t*	thinker;
    Mob*	m;
	
    // find all the target spots
    numbraintargets = 0;
    braintargeton = 0;
	
    thinker = thinkercap.next;
    for (thinker = thinkercap.next ;
	 thinker != &thinkercap ;
	 thinker = thinker->next)
    {
	if (thinker->function.acp1 != (actionf_p1)P_MobjThinker)
	    continue;	// not a mobj

	m = (Mob *)thinker;

	if (m->type == MT_BOSSTARGET )
	{
	    braintargets[numbraintargets] = m;
	    numbraintargets++;
	}
    }
	
    S_StartSound (NULL,sfx_bossit);
}


void A_BrainPain (Mob*	mo)
{
    S_StartSound (NULL,sfx_bospn);
}


void A_BrainScream (Mob*	mo)
{
    Mob*	th;
    printf("A_BrainScream broken?? \n");
    for (double x = mo->xx  - 196; x < mo->xx + 320; x+= 8)
    {
        double y = mo->yy - 320;
	double z = 128 + P_Random() * 2; // @todo the z is most likely fishy
	th = PP_SpawnMobj(x,y,z, MT_ROCKET);
    // @todo how to change this so it works for double instead of fixed *512*
	th->mmomz = P_Random()*512;

	th->setState(S_BRAINEXPLODE1);

	th->tics -= P_Random()&7;
	if (th->tics < 1)
	    th->tics = 1;
    }
	
    S_StartSound (NULL,sfx_bosdth);
}



void A_BrainExplode (Mob* mo)
{
    int		x;
    int		y;
    int		z;
    Mob*	th;
    printf("A_BrainExplode\n");
    //x = double_to_fixed(mo->xx) + (P_Random () - P_Random ())*2048;
    x = mo->xx + (P_Random () - P_Random ()) / 32;
    y = mo->yy;
    z = 128 + P_Random()*2;
    th = PP_SpawnMobj(x,y,z, MT_ROCKET);
    // @todo how to fix this for double instead of fixed *512*
    //th->mmomz = P_Random() * 512;
    th->mmomz = P_Random() / 128;

    th->setState(S_BRAINEXPLODE1);

    th->tics -= P_Random()&7;
    if (th->tics < 1)
	th->tics = 1;
}


void A_BrainDie (Mob*	mo)
{
    G_ExitLevel ();
}

void A_BrainSpit (Mob*	mo)
{
    Mob*	targ;
    Mob*	newmobj;
    
    static int	easy = 0;
	
    easy ^= 1;
    if (gameskill <= sk_easy && (!easy))
	return;
		
    // shoot a cube at current target
    targ = braintargets[braintargeton];
    braintargeton = (braintargeton+1)%numbraintargets;

    // spawn brain missile
    newmobj = P_SpawnMissile (mo, targ, MT_SPAWNSHOT);
    newmobj->target = targ;
    // @todo think this could be broken
    newmobj->reactiontime = ((targ->yy - mo->yy)/newmobj->mmomy) / newmobj->state->tics;

    S_StartSound(NULL, sfx_bospit);
}



void A_SpawnFly (Mob* mo);

// travelling cube sound
void A_SpawnSound (Mob* mo)	
{
    S_StartSound (mo,sfx_boscub);
    A_SpawnFly(mo);
}

void A_SpawnFly (Mob* mo)
{
    Mob*	newmobj;
    Mob*	fog;
    Mob*	targ;
    int		r;
    mobjtype_t	type;
	
    if (--mo->reactiontime)
	return;	// still flying
	
    targ = mo->target;

    // First spawn teleport fog.
    fog = PP_SpawnMobj(targ->xx, targ->yy, targ->zz, MT_SPAWNFIRE);
    S_StartSound (fog, sfx_telept);

    // Randomly select monster to spawn.
    r = P_Random ();

    // Probability distribution (kind of :),
    // decreasing likelihood.
    if ( r<50 )
	type = MT_TROOP;
    else if (r<90)
	type = MT_SERGEANT;
    else if (r<120)
	type = MT_SHADOWS;
    else if (r<130)
	type = MT_PAIN;
    else if (r<160)
	type = MT_HEAD;
    else if (r<162)
	type = MT_VILE;
    else if (r<172)
	type = MT_UNDEAD;
    else if (r<192)
	type = MT_BABY;
    else if (r<222)
	type = MT_FATSO;
    else if (r<246)
	type = MT_KNIGHT;
    else
	type = MT_BRUISER;		

    newmobj	= PP_SpawnMobj(targ->xx, targ->yy, targ->zz, type);
    if (P_LookForPlayers (newmobj, true) )
        newmobj->setState((statenum_t)newmobj->info->seestate);
	
    // telefrag anything in this spot
    PP_TeleportMove(newmobj, newmobj->xx, newmobj->yy);

    // remove self (i.e., cube).
    P_RemoveMobj (mo);
}



void A_PlayerScream (Mob* mo)
{
    // Default death sound.
    int		sound = sfx_pldeth;
	
    if ( (gamemode == commercial)
	&& 	(mo->health < -50))
    {
	// IF THE PLAYER DIES
	// LESS THAN -50% WITHOUT GIBBING
	sound = sfx_pdiehi;
    }
    
    S_StartSound (mo, sound);
}
