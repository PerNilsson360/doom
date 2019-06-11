#include "Mob.hh"
#include "MapThing.hh"

#include "i_system.h"
#include "z_zone.h"
#include "m_random.h"

#include "doomdef.h"
#include "p_local.h"
#include "sounds.h"

#include "st_stuff.h"
#include "hu_stuff.h"

#include "s_sound.h"

#include "doomstat.h"
#include "p_mobj.h"
#include "d_player.h"
#include "Sector.hh"

void G_PlayerReborn (int player);
void P_SpawnMapThing (MapThing*	mthing);

//
// P_SetMobjState
// Returns true if the mobj is still present.
//
bool
Mob::setState(statenum_t s)
{
    state_t* st;

    do
    {
        if (s == S_NULL)
        {
            state = (state_t *) S_NULL;
            P_RemoveMobj(this);
            return false;
        }
        
        st = &states[s];
        state = st;
        tics = st->tics;
        sprite = st->sprite;
        frame = st->frame;
        
        // Modified handling.
        // Call action functions when the state is set
        if (st->action)		
            ((void(*)(Mob*))st->action)(this);	
        
        s = st->nextstate;
    } while (!tics);
				
    return true;
}


//
// P_ExplodeMissile  
//
void 
Mob::explodeMissile()
{
    mmomx = mmomy = mmomz = 0;

    setState((statenum_t)mobjinfo[type].deathstate);

    tics -= P_Random()&3;

    if (tics < 1)
        tics = 1;

    flags &= ~MF_MISSILE;

    if (info->deathsound)
        S_StartSound(this, info->deathsound);
}


//
// P_XYMovement  
//
#define STOPSPEED 0.0625		//0x1000 //0001 0000 0000 0000 1 * (2 - 1)
#define FRICTION  0.90625		//0xe800 //1110 1000 0000 0000

void 
Mob::xyMovement() 
{ 		
    if (!mmomx && !mmomy)
    {
        if (flags & MF_SKULLFLY)
        {
            // the skull slammed into something
            flags &= ~MF_SKULLFLY;
            mmomx = mmomy = mmomz = 0;
            
            setState((statenum_t)info->spawnstate);
        }
        return;
    }
	
    if (mmomx > MMAXMOVE)
        mmomx = MMAXMOVE;
    else if (mmomx < -MMAXMOVE)
        mmomx = -MMAXMOVE;
    
    if (mmomy > MMAXMOVE)
        mmomy = MMAXMOVE;
    else if (mmomy < -MMAXMOVE)
        mmomy = -MMAXMOVE;
    

    double ptryx;
    double ptryy;
    double xmove = mmomx;
    double ymove = mmomy;

    double x = position.getX();
    double y = position.getY();
    
    do
    {
        if (xmove > MMAXMOVE/2 || ymove > MMAXMOVE/2)
        {
            ptryx = x + xmove/2;
            ptryy = y + ymove/2;
            xmove /= 2;
            ymove /= 2;
        }
        else
        {
            ptryx = x + xmove;
            ptryy = y + ymove;
            xmove = ymove = 0;
        }
	
        if (!PP_TryMove (this, Vertex(ptryx, ptryy)))
        {
            // blocked move
            if (player)
            {	// try to slide along it
                P_SlideMove (this);
            }
            else if (flags & MF_MISSILE)
            {
                // explode a missile
                if (ceilingline &&
                    ceilingline->backsector &&
                    ceilingline->backsector->ceilingpic == skyflatnum)
                {
                    // Hack to prevent missiles exploding
                    // against the sky.
                    // Does not handle sky floors.
                    P_RemoveMobj(this);
                    return;
                }
                explodeMissile();
            }
            else
                mmomx = mmomy = 0;
        }
    } while (xmove || ymove);
    
    // slow down
    if (player && player->cheats & CF_NOMOMENTUM)
    {
        // debug option for no sliding at all
        mmomx = mmomy = 0;
        return;
    }
    
    if (flags & (MF_MISSILE | MF_SKULLFLY) )
        return; 	// no friction for missiles ever
    
    if (zz > ffloorz)
        return;		// no friction when airborne
    
    if (flags & MF_CORPSE)
    {
        // do not stop sliding
        //  if halfway off a step with some momentum
        if (mmomx > 1/4
            || mmomx < -1/4
            || mmomy > 1/4
            || mmomy < -1/4)
        {
            if (ffloorz != subsector->sector->ffloorheight)
                return;
        }
    }
    
    if (mmomx > -STOPSPEED
        && mmomx < STOPSPEED
        && mmomy > -STOPSPEED
        && mmomy < STOPSPEED
        && (!player
            || (player->cmd.forwardmove== 0
                && player->cmd.sidemove == 0 ) ) )
    {
        // if in a walking frame, stop moving
        if ( player&&(unsigned)((player->mo->state - states)- S_PLAY_RUN1) < 4)
            player->mo->setState(S_PLAY);
        
        mmomx = 0;
        mmomy = 0;
    }
    else
    {
        mmomx = mmomx * FRICTION;
        mmomy = mmomy * FRICTION;
    }
}

//
// P_ZMovement
//
void 
Mob::zMovement()
{
    double dist;
    double delta;
    
    // check for smooth step up
    if (player && zz < ffloorz)
    {
        player->vviewheight -= ffloorz - zz;
        // @todo /2 used to be >> 3
        player->ddeltaviewheight = (VVIEWHEIGHT - player->vviewheight)/2;
    }
    
    // adjust height
    zz += mmomz;
	
    if ( flags & MF_FLOAT && 
         target)
    {
        // float down towards target if too close
        if ( !(flags & MF_SKULLFLY) && 
             !(flags & MF_INFLOAT) )
        {

            dist = position.distance(target->position);
            
            delta = target->zz + (hheight / 2) - zz;
            
            if (delta<0 && dist < -(delta*3) )
                zz -= FFLOATSPEED;
            else if (delta>0 && dist < (delta*3) )
                zz += FFLOATSPEED;			
        }
    }
    
    // clip movement
    if (zz <= ffloorz)
    {
        // hit the floor

        // Note (id):
        //  somebody left this after the setting momz to 0,
        //  kinda useless there.
        if (flags & MF_SKULLFLY)
        {
            // the skull slammed into something
            mmomz = -mmomz;
        }
	
        if (mmomz < 0)
        {
            if (player && mmomz < -GGRAVITY*8)	
            {
                // Squat down.
                // Decrease viewheight for a moment
                // after hitting the ground (hard),
                // and utter appropriate sound.
                player->ddeltaviewheight = mmomz / 6;
                S_StartSound (this, sfx_oof);
            }
            mmomz = 0;
        }
        zz = ffloorz;
    
        if ((flags & MF_MISSILE) && !(flags & MF_NOCLIP))
        {
            explodeMissile();
            return;
        }
    }
    else if (! (flags & MF_NOGRAVITY) )
    {
        if (mmomz == 0)
            mmomz = -GGRAVITY*2;
        else
            mmomz -= GGRAVITY;
    }
	
    if (zz + hheight > cceilingz)
    {
        // hit the ceiling
        if (mmomz > 0)
        {
            mmomz = 0;
            zz = cceilingz - hheight;
        }

        if (flags & MF_SKULLFLY)
        {	// the skull slammed into something
            mmomz = -mmomz;
        }
        
        if ( (flags & MF_MISSILE)
             && !(flags & MF_NOCLIP) )
        {
            explodeMissile();
            return;
        }
    }
} 



//
// P_NightmareRespawn
//
void
P_NightmareRespawn (Mob* mobj)
{
    SubSector*	ss; 
    Mob*		mo;
    MapThing*		mthing;
	
    Vertex position(mobj->spawnpoint.x, mobj->spawnpoint.y);
    
    // somthing is occupying it's position?
    if (!PP_CheckPosition(mobj, position))
        return;	// no respwan
    
    // spawn a teleport fog at old spot
    // because of removal of the body?
    mo = PP_SpawnMobj(mobj->position,
		      mobj->subsector->sector->ffloorheight, 
		      MT_TFOG); 
    
    // initiate teleport sound
    S_StartSound (mo, sfx_telept);
    
    // spawn a teleport fog at the new spot
    ss = RR_PointInSubsector(position); 
    
    mo = PP_SpawnMobj(position, ss->sector->ffloorheight, MT_TFOG); 
    
    S_StartSound (mo, sfx_telept);
    
    // spawn the new monster
    mthing = &mobj->spawnpoint;
	
    // spawn it
    // todo is onceiling ... correct now that double is used
    double z;
    if (mobj->info->flags & MF_SPAWNCEILING)
        z = ONCEILINGZ;
    else
        z = ONFLOORZ;
    
    // inherit attributes from deceased one
    mo = PP_SpawnMobj (position, z, mobj->type);
    mo->spawnpoint = mobj->spawnpoint;	
    mo->_angle = mthing->getAngle();

    if (mthing->options & MTF_AMBUSH)
	mo->flags |= MF_AMBUSH;

    mo->reactiontime = 18;
	
    // remove the old monster,
    P_RemoveMobj (mobj);
}


//
// P_MobjThinker
//
void P_MobjThinker (Mob* mobj)
{
    // momentum movement
    if (mobj->mmomx || 
        mobj->mmomy || 
        (mobj->flags&MF_SKULLFLY) )
    {
        mobj->xyMovement();

	// FIXME: decent NOP/NULL/Nil function pointer please.
	if (mobj->thinker.function.acv == (actionf_v) (-1))
	    return;		// mobj was removed
    }

    if ((mobj->zz != mobj->ffloorz) || 
         mobj->mmomz)
    {
        mobj->zMovement();
        
        // FIXME: decent NOP/NULL/Nil function pointer please.
        if (mobj->thinker.function.acv == (actionf_v) (-1))
            return;		// mobj was removed
    }

    
    // cycle through states,
    // calling action functions at transitions
    if (mobj->tics != -1)
    {
	mobj->tics--;
		
	// you can cycle through multiple states in a tic
	if (!mobj->tics)
	    if (!mobj->setState(mobj->state->nextstate))
		return;		// freed itself
    }
    else
    {
	// check for nightmare respawn
	if (! (mobj->flags & MF_COUNTKILL) )
	    return;

	if (!respawnmonsters)
	    return;

	mobj->movecount++;

	if (mobj->movecount < 12*35)
	    return;

	if ( leveltime&31 )
	    return;

	if (P_Random () > 4)
	    return;

	P_NightmareRespawn (mobj);
    }

}


//
// P_SpawnMobj
//
Mob*
PP_SpawnMobj( 
    const Vertex& v,
    double z,
    mobjtype_t type)
{
    Mob*	mobj;
    state_t*	st;
    mobjinfo_t*	info;
	
    mobj = (Mob*)Z_Malloc (sizeof(*mobj), PU_LEVEL, NULL);
    memset (mobj, 0, sizeof (*mobj));
    info = &mobjinfo[type];
	
    mobj->type = type;
    mobj->info = info;
    mobj->position = v;
    mobj->rradius = info->rradius;
    mobj->hheight = info->hheight;
    mobj->flags = info->flags;
    mobj->health = info->spawnhealth;

    if (gameskill != sk_nightmare)
        mobj->reactiontime = info->reactiontime;
    
    mobj->lastlook = P_Random () % MAXPLAYERS;
    // do not set the state with P_SetMobjState,
    // because action routines can not be called yet
    st = &states[info->spawnstate];

    mobj->state = st;
    mobj->tics = st->tics;
    mobj->sprite = st->sprite;
    mobj->frame = st->frame;

    // set subsector and/or block links
    P_SetThingPosition (mobj);
	
    mobj->ffloorz = mobj->subsector->sector->ffloorheight;
    mobj->cceilingz = mobj->subsector->sector->cceilingheight;
    
    if (z == ONFLOORZ) {
        mobj->zz = mobj->ffloorz;
    } else if (z == ONCEILINGZ) {
        mobj->zz = mobj->cceilingz - mobj->info->hheight;
    }else { 
        mobj->zz = z;
    }

    mobj->thinker.function.acp1 = (actionf_p1)P_MobjThinker;
	
    P_AddThinker (&mobj->thinker);
    return mobj;
}


//
// P_RemoveMobj
//
MapThing	itemrespawnque[ITEMQUESIZE];
int		itemrespawntime[ITEMQUESIZE];
int		iquehead;
int		iquetail;


void P_RemoveMobj (Mob* mobj)
{
    if ((mobj->flags & MF_SPECIAL)
	&& !(mobj->flags & MF_DROPPED)
	&& (mobj->type != MT_INV)
	&& (mobj->type != MT_INS))
    {
	itemrespawnque[iquehead] = mobj->spawnpoint;
	itemrespawntime[iquehead] = leveltime;
	iquehead = (iquehead+1)&(ITEMQUESIZE-1);

	// lose one off the end?
	if (iquehead == iquetail)
	    iquetail = (iquetail+1)&(ITEMQUESIZE-1);
    }
	
    // unlink from sector and block lists
    P_UnsetThingPosition (mobj);
    
    // stop any playing sound
    S_StopSound (mobj);
    
    // free block
    P_RemoveThinker ((thinker_t*)mobj);
}




//
// P_RespawnSpecials
//
void P_RespawnSpecials (void)
{
    SubSector*	ss; 
    Mob*		mo;
    MapThing*		mthing;
    
    int			i;

    // only respawn items in deathmatch
    if (!deathmatch)
	return;	 

    // nothing left to respawn?
    if (iquehead == iquetail)
	return;		

    // wait at least 30 seconds
    if (leveltime - itemrespawntime[iquetail] < 30*35)
	return;			

    mthing = &itemrespawnque[iquetail];
	
    Vertex position(mthing->x, mthing->y); 
	  
    // spawn a teleport fog at the new spot
    ss = RR_PointInSubsector(position);
    // @todo fllorheight below is fixed?
    mo = PP_SpawnMobj(position, ss->sector->ffloorheight , MT_IFOG); 
    S_StartSound (mo, sfx_itmbk);
    
    // find which type to spawn
    for (i=0 ; i< NUMMOBJTYPES ; i++)
    {
        if (mthing->type == mobjinfo[i].doomednum)
            break;
    }
    
    // spawn it
    // @todo is onceiling .. correct now with double
    double z;
    if (mobjinfo[i].flags & MF_SPAWNCEILING)
        z = ONCEILINGZ;
    else
        z = ONFLOORZ;

    mo = PP_SpawnMobj(position, z, (mobjtype_t)i);
    mo->spawnpoint = *mthing;	
    mo->_angle = mthing->getAngle();
    
    // pull it from the que
    iquetail = (iquetail+1)&(ITEMQUESIZE-1);
}




//
// P_SpawnPlayer
// Called when a player is spawned on the level.
// Most of the player structure stays unchanged
//  between levels.
//
void P_SpawnPlayer (MapThing* mthing)
{
    player_t*		p;
    Mob*		mobj;
    int			i;

    // not playing?
    if (!playeringame[mthing->type-1])
	return;					
		
    p = &players[mthing->type-1];

    if (p->playerstate == PST_REBORN)
	G_PlayerReborn (mthing->type-1);

    Vertex position(mthing->x, mthing->y);
    double z = ONFLOORZ;
    mobj = PP_SpawnMobj(position, z, MT_PLAYER);
    
    // set color translations for player sprites
    if (mthing->type > 1)		
        mobj->flags |= (mthing->type-1)<<MF_TRANSSHIFT;
    
    mobj->_angle = mthing->getAngle();
    mobj->player = p;
    mobj->health = p->health;

    p->mo = mobj;
    p->playerstate = PST_LIVE;	
    p->refire = 0;
    p->message = NULL;
    p->damagecount = 0;
    p->bonuscount = 0;
    p->extralight = 0;
    p->fixedcolormap = 0;
    p->vviewheight = VVIEWHEIGHT;

    // setup gun psprite
    P_SetupPsprites (p);
    
    // give all cards in death match mode
    if (deathmatch)
        for (i=0 ; i<NUMCARDS ; i++)
            p->cards[i] = true;
    
    if (mthing->type-1 == consoleplayer)
    {
        // wake up the status bar
        ST_Start ();
        // wake up the heads up text
        HU_Start ();		
    }
}


//
// P_SpawnMapThing
// The fields of the mapthing should
// already be in host byte order.
//
void P_SpawnMapThing (MapThing* mthing)
{
    int			i;
    int			bit;
    Mob*		mobj;
		
    // count deathmatch start positions
    if (mthing->type == 11)
    {
        if (deathmatch_p < &deathmatchstarts[10])
        {
            memcpy (deathmatch_p, mthing, sizeof(*mthing));
            deathmatch_p++;
        }
        return;
    }
	
    // check for players specially
    if (mthing->type <= 4)
    {
        // save spots for respawning in network games
        playerstarts[mthing->type-1] = *mthing;
        if (!deathmatch)
            P_SpawnPlayer (mthing);
        
        return;
    }
    
    // check for apropriate skill level
    if (!netgame && (mthing->options & 16) )
        return;
		
    if (gameskill == sk_baby)
        bit = 1;
    else if (gameskill == sk_nightmare)
        bit = 4;
    else
        bit = 1<<(gameskill-1);
    
    if (!(mthing->options & bit) )
        return;
	
    // find which type to spawn
    for (i=0 ; i< NUMMOBJTYPES ; i++)
        if (mthing->type == mobjinfo[i].doomednum)
            break;
	
    if (i==NUMMOBJTYPES)
        I_Error("P_SpawnMapThing: Unknown type %i at (%i, %i)",
                mthing->type,
                mthing->x, 
                mthing->y);
    
    // don't spawn keycards and players in deathmatch
    if (deathmatch && mobjinfo[i].flags & MF_NOTDMATCH)
        return;
    
    // don't spawn any monsters if -nomonsters
    if (nomonsters
        && ( i == MT_SKULL
             || (mobjinfo[i].flags & MF_COUNTKILL)) )
    {
        return;
    }
    
    // spawn it
    Vertex position(mthing->x, mthing->y);
    double z;
    if (mobjinfo[i].flags & MF_SPAWNCEILING)
        z = ONCEILINGZ;
    else
        z = ONFLOORZ;
    
    mobj = PP_SpawnMobj(position, z, (mobjtype_t)i);
    mobj->spawnpoint = *mthing;
    
    if (mobj->tics > 0)
        mobj->tics = 1 + (P_Random () % mobj->tics);
    if (mobj->flags & MF_COUNTKILL)
        totalkills++;
    if (mobj->flags & MF_COUNTITEM)
        totalitems++;
    
    mobj->_angle = mthing->getAngle();
    if (mthing->options & MTF_AMBUSH)
        mobj->flags |= MF_AMBUSH;
}



//
// GAME SPAWN FUNCTIONS
//


//
// P_SpawnPuff
//
extern double aattackrange;

void
PP_SpawnPuff(
    const Vertex& v,
    double z)
{
    Mob*	th;
	
    z += P_Random() - P_Random() / 64;

    th = PP_SpawnMobj(v, z, MT_PUFF);
    th->mmomz = 1;
    th->tics -= P_Random()&3;

    if (th->tics < 1)
	th->tics = 1;
	
    // don't make punches spark on the wall
    if (aattackrange == MMELEERANGE)
        th->setState(S_PUFF3);
}

//
// P_SpawnBlood
// 
void
PP_SpawnBlood(
    const Vertex& v,
    double z,
    int damage )
{
    Mob*	th;
    z += P_Random() - P_Random() / 64;
    th = PP_SpawnMobj(v, z, MT_BLOOD);
    th->mmomz = 1*2;
    th->tics -= P_Random()&3;
    
    if (th->tics < 1)
        th->tics = 1;
    
    if (damage <= 12 && damage >= 9)
        th->setState(S_BLOOD2);
    else if (damage < 9)
        th->setState(S_BLOOD3);
}



//
// P_CheckMissileSpawn
// Moves the missile forward a bit
//  and possibly explodes it right there.
//
void P_CheckMissileSpawn (Mob* th)
{
    th->tics -= P_Random()&3;
    if (th->tics < 1)
        th->tics = 1;
    
    // move a little forward so an angle can
    // be computed if it immediately explodes
    th->position += Vertex(th->mmomx / 2, th->mmomy / 2);
    th->zz += th->mmomz / 2;
    
    if (!PP_TryMove (th, th->position))
        th->explodeMissile();
}


//
// P_SpawnMissile
//
Mob*
P_SpawnMissile
( Mob*	source,
  Mob*	dest,
  mobjtype_t	type )
{
    Mob*	th;
    double dist;

    th = PP_SpawnMobj(source->position,
                      source->zz + 4*8, 
                      type);
    
    if (th->info->seesound)
        S_StartSound (th, th->info->seesound);
    th->target = source;	// where it came from
    Angle an = Angle(source->position, dest->position);	
    if (dest->flags & MF_SHADOW)
        an += (P_Random()-P_Random()) * Angle::A360 / 4096;	
    th->_angle = an;
    th->mmomx = th->info->speed * cos(an);
    th->mmomy= th->info->speed * sin(an);
    dist = source->position.distance(dest->position);
    dist = dist / th->info->speed;
    if (dist < 1)
        dist = 1;
    th->mmomz = (dest->zz - source->zz) / dist;
    P_CheckMissileSpawn (th);
    return th;
}


//
// P_SpawnPlayerMissile
// Tries to aim at a nearby monster
//
void
P_SpawnPlayerMissile
( Mob*	source,
  mobjtype_t	type )
{
    Mob*	th;
    printf("P_SpawnPlayerMissile\n");
    // see which target is to be aimed at
    Angle an = source->_angle;
    double slope = PP_AimLineAttack(source, an, 16*64);
    
    if (!linetarget)
    {
        an += Angle::A360 / 64;
        slope = PP_AimLineAttack(source, an, 16*64);
        
        if (!linetarget)
        {
            an -= 2 * Angle::A360 / 64;
            slope = PP_AimLineAttack(source, an, 16*64);
        }
        
        if (!linetarget)
        {
            an = source->_angle;
            slope = 0;
        }
    }
		
    double z = source->zz + 4*8;
    th = PP_SpawnMobj(source->position, z, type);

    if (th->info->seesound)
        S_StartSound (th, th->info->seesound);
    
    th->target = source;
    th->_angle = an;
    
    th->mmomx = th->info->speed * cos(th->_angle);
    th->mmomy = th->info->speed * sin(th->_angle);
    th->mmomz = th->info->speed * slope;
    
    P_CheckMissileSpawn (th);
}

void
Mob::print()
{
    assert(false && "NOT IMPLEMENTED");
}
