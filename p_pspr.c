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
//	Weapon sprite animation, weapon objects.
//	Action functions for weapons.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: p_pspr.c,v 1.5 1997/02/03 22:45:12 b1 Exp $";

#include "doomdef.h"
#include "d_event.h"


#include "m_random.h"
#include "p_local.h"
#include "s_sound.h"

// State.
#include "doomstat.h"

// Data.
#include "sounds.h"

#include "p_pspr.h"

#define LLOWERSPEED		4*6
#define RRAISESPEED		4*6

#define WWEAPONBOTTOM	        12 * 128
#define WWEAPONTOP	        12 * 32


// plasma cells for a bfg attack
#define BFGCELLS		40		


//
// P_SetPsprite
//
void
P_SetPsprite
( player_t*	player,
  int		position,
  statenum_t	stnum ) 
{
    pspdef_t*	psp;
    state_t*	state;
	
    psp = &player->psprites[position];
	
    do
    {
	if (!stnum)
	{
	    // object removed itself
	    psp->state = NULL;
	    break;	
	}
	
	state = &states[stnum];
	psp->state = state;
	psp->tics = state->tics;	// could be 0

	if (state->misc1)
	{
	    // coordinate set
	    psp->ssx = state->misc1;
	    psp->ssy = state->misc2;
	}
	
	// Call action routine.
	// Modified handling.
	if (state->action)
	{
	    ((actionf_p2)state->action)(player, psp);
	    if (!psp->state)
		break;
	}
	
	stnum = psp->state->nextstate;
	
    } while (!psp->tics);
    // an initial state of 0 could cycle through
}

//
// P_BringUpWeapon
// Starts bringing the pending weapon up
// from the bottom of the screen.
// Uses player
//
void P_BringUpWeapon (player_t* player)
{
    statenum_t	newstate;
	
    if (player->pendingweapon == wp_nochange)
	player->pendingweapon = player->readyweapon;
		
    if (player->pendingweapon == wp_chainsaw)
	S_StartSound (player->mo, sfx_sawup);
		
    newstate = weaponinfo[player->pendingweapon].upstate;

    player->pendingweapon = wp_nochange;
    player->psprites[ps_weapon].ssy = WWEAPONBOTTOM;

    P_SetPsprite (player, ps_weapon, newstate);
}

//
// P_CheckAmmo
// Returns true if there is enough ammo to shoot.
// If not, selects the next weapon to use.
//
bool P_CheckAmmo (player_t* player)
{
    ammotype_t		ammo;
    int			count;

    ammo = weaponinfo[player->readyweapon].ammo;

    // Minimal amount for one shot varies.
    if (player->readyweapon == wp_bfg)
	count = BFGCELLS;
    else if (player->readyweapon == wp_supershotgun)
	count = 2;	// Double barrel.
    else
	count = 1;	// Regular.

    // Some do not need ammunition anyway.
    // Return if current ammunition sufficient.
    if (ammo == am_noammo || player->ammo[ammo] >= count)
	return true;
		
    // Out of ammo, pick a weapon to change to.
    // Preferences are set here.
    do
    {
	if (player->weaponowned[wp_plasma]
	    && player->ammo[am_cell]
	    && (gamemode != shareware) )
	{
	    player->pendingweapon = wp_plasma;
	}
	else if (player->weaponowned[wp_supershotgun] 
		 && player->ammo[am_shell]>2
		 && (gamemode == commercial) )
	{
	    player->pendingweapon = wp_supershotgun;
	}
	else if (player->weaponowned[wp_chaingun]
		 && player->ammo[am_clip])
	{
	    player->pendingweapon = wp_chaingun;
	}
	else if (player->weaponowned[wp_shotgun]
		 && player->ammo[am_shell])
	{
	    player->pendingweapon = wp_shotgun;
	}
	else if (player->ammo[am_clip])
	{
	    player->pendingweapon = wp_pistol;
	}
	else if (player->weaponowned[wp_chainsaw])
	{
	    player->pendingweapon = wp_chainsaw;
	}
	else if (player->weaponowned[wp_missile]
		 && player->ammo[am_misl])
	{
	    player->pendingweapon = wp_missile;
	}
	else if (player->weaponowned[wp_bfg]
		 && player->ammo[am_cell]>40
		 && (gamemode != shareware) )
	{
	    player->pendingweapon = wp_bfg;
	}
	else
	{
	    // If everything fails.
	    player->pendingweapon = wp_fist;
	}
	
    } while (player->pendingweapon == wp_nochange);

    // Now set appropriate weapon overlay.
    P_SetPsprite (player,
		  ps_weapon,
		  weaponinfo[player->readyweapon].downstate);

    return false;	
}


//
// P_FireWeapon.
//
void P_FireWeapon (player_t* player)
{
    statenum_t	newstate;
	
    if (!P_CheckAmmo (player))
        return;
	
    player->mo->setState(S_PLAY_ATK1);
    newstate = weaponinfo[player->readyweapon].atkstate;
    P_SetPsprite (player, ps_weapon, newstate);
    P_NoiseAlert (player->mo, player->mo);
}



//
// P_DropWeapon
// Player died, so put the weapon away.
//
void P_DropWeapon (player_t* player)
{
    P_SetPsprite (player,
		  ps_weapon,
		  weaponinfo[player->readyweapon].downstate);
}



//
// A_WeaponReady
// The player can fire the weapon
// or change to another weapon at this time.
// Follows after getting weapon up,
// or after previous attack/fire sequence.
//
void
A_WeaponReady
( player_t*	player,
  pspdef_t*	psp )
{	
    statenum_t	newstate;
    
    // get out of attack state
    if (player->mo->state == &states[S_PLAY_ATK1]
	|| player->mo->state == &states[S_PLAY_ATK2] )
    {
        player->mo->setState(S_PLAY);
    }
    
    if (player->readyweapon == wp_chainsaw
	&& psp->state == &states[S_SAW])
    {
	S_StartSound (player->mo, sfx_sawidl);
    }
    
    // check for change
    //  if player is dead, put the weapon away
    if (player->pendingweapon != wp_nochange || !player->health)
    {
	// change weapon
	//  (pending weapon should allready be validated)
	newstate = weaponinfo[player->readyweapon].downstate;
	P_SetPsprite (player, ps_weapon, newstate);
	return;	
    }
    
    // check for fire
    //  the missile launcher and bfg do not auto fire
    if (player->cmd.buttons & BT_ATTACK)
    {
	if ( !player->attackdown
	     || (player->readyweapon != wp_missile
		 && player->readyweapon != wp_bfg) )
	{
	    player->attackdown = true;
	    P_FireWeapon (player);		
	    return;
	}
    }
    else
	player->attackdown = false;
    
    psp->ssx = 1 + (24 * cos(player->bbob));
    psp->ssy = WWEAPONTOP + (8 * INV_ASPECT_RATIO * sin(2 * player->bbob));
}



//
// A_ReFire
// The player can re-fire the weapon
// without lowering it entirely.
//
void A_ReFire
( player_t*	player,
  pspdef_t*	psp )
{
    
    // check for fire
    //  (if a weaponchange is pending, let it go through instead)
    if ( (player->cmd.buttons & BT_ATTACK) 
	 && player->pendingweapon == wp_nochange
	 && player->health)
    {
	player->refire++;
	P_FireWeapon (player);
    }
    else
    {
	player->refire = 0;
	P_CheckAmmo (player);
    }
}


void
A_CheckReload
( player_t*	player,
  pspdef_t*	psp )
{
    P_CheckAmmo (player);
    // @todo what is this
#if 0
    if (player->ammo[am_shell]<2)
	P_SetPsprite (player, ps_weapon, S_DSNR1);
#endif
}



//
// A_Lower
// Lowers current weapon,
//  and changes weapon at bottom.
//
void
A_Lower
( player_t*	player,
  pspdef_t*	psp )
{	
    psp->ssy += LLOWERSPEED;

    // Is already down.
    if (psp->ssy < WWEAPONBOTTOM )
	return;

    // Player is dead.
    if (player->playerstate == PST_DEAD)
    {
	psp->ssy = WWEAPONBOTTOM;

	// don't bring weapon back up
	return;		
    }
    
    // The old weapon has been lowered off the screen,
    // so change the weapon and start raising it
    if (!player->health)
    {
	// Player is dead, so keep the weapon off screen.
	P_SetPsprite (player,  ps_weapon, S_NULL);
	return;	
    }
	
    player->readyweapon = player->pendingweapon; 

    P_BringUpWeapon (player);
}


//
// A_Raise
//
void
A_Raise
( player_t*	player,
  pspdef_t*	psp )
{
    statenum_t	newstate;
	
    psp->ssy -= RRAISESPEED;

    if (psp->ssy > WWEAPONTOP )
	return;
    
    psp->ssy = WWEAPONTOP;
    
    // The weapon has been raised all the way,
    //  so change to the ready state.
    newstate = weaponinfo[player->readyweapon].readystate;

    P_SetPsprite (player, ps_weapon, newstate);
}



//
// A_GunFlash
//
void
A_GunFlash
( player_t*	player,
  pspdef_t*	psp ) 
{
    player->mo->setState(S_PLAY_ATK2);
    P_SetPsprite (player,ps_flash,weaponinfo[player->readyweapon].flashstate);
}



//
// WEAPON ATTACKS
//


//
// A_Punch
//
void
A_Punch
( player_t*	player,
  pspdef_t*	psp ) 
{
    int	damage = (P_Random ()%10+1)<<1;

    if (player->powers[pw_strength])	
	damage *= 10;

    Angle angle = player->mo->_angle;
    angle += (double)(P_Random()-P_Random()) * 4; // 2^2 (<< 18)
    double slope = PP_AimLineAttack(player->mo, angle, MMELEERANGE);
    PP_LineAttack(player->mo, angle, MMELEERANGE, slope, damage);

    // turn to face target
    if (linetarget)
    {
        S_StartSound (player->mo, sfx_punch);
        player->mo->_angle = Angle(player->mo->xx,
                                   player->mo->yy,
                                   linetarget->xx,
                                   linetarget->yy);
    }
}


//
// A_Saw
//
void
A_Saw
( player_t*	player,
  pspdef_t*	psp ) 
{
    int damage = 2*(P_Random ()%10+1);
    Angle angle = player->mo->_angle;
    angle += (angle_t)(P_Random()-P_Random()) <<18;
    
    // use meleerange + 1 se the puff doesn't skip the flash
    double slope = PP_AimLineAttack(player->mo, angle, MMELEERANGE+1);
    PP_LineAttack (player->mo, angle, MMELEERANGE+1, slope, damage);

    if (!linetarget)
    {
	S_StartSound (player->mo, sfx_sawful);
	return;
    }
    S_StartSound (player->mo, sfx_sawhit);
	
    // turn to face target
    angle = Angle(player->mo->xx, 
                  player->mo->yy,
                  linetarget->xx, 
                  linetarget->yy);

    if (angle - player->mo->_angle > Angle::A180)
    {
        if (angle - player->mo->_angle < -Angle::A90/20)
            player->mo->_angle = angle + Angle(Angle::A90)/21;
	else
	    player->mo->_angle -= Angle(Angle::A90)/20;
    }
    else
    {
        if (angle - player->mo->_angle > Angle::A90/20)
            player->mo->_angle = angle - Angle(Angle::A90)/21;
	else
	    player->mo->_angle += Angle(Angle::A90)/20;
    }
    player->mo->flags |= MF_JUSTATTACKED;
}



//
// A_FireMissile
//
void
A_FireMissile
( player_t*	player,
  pspdef_t*	psp ) 
{
    player->ammo[weaponinfo[player->readyweapon].ammo]--;
    P_SpawnPlayerMissile (player->mo, MT_ROCKET);
}


//
// A_FireBFG
//
void
A_FireBFG
( player_t*	player,
  pspdef_t*	psp ) 
{
    player->ammo[weaponinfo[player->readyweapon].ammo] -= BFGCELLS;
    P_SpawnPlayerMissile (player->mo, MT_BFG);
}



//
// A_FirePlasma
//
void
A_FirePlasma
( player_t*	player,
  pspdef_t*	psp ) 
{
    player->ammo[weaponinfo[player->readyweapon].ammo]--;

    P_SetPsprite (player,
                  ps_flash,
                  (statenum_t)(weaponinfo[player->readyweapon].flashstate+(P_Random ()&1)));

    P_SpawnPlayerMissile (player->mo, MT_PLASMA);
}



//
// P_BulletSlope
// Sets a slope so a near miss is at aproximately
// the height of the intended target
//
double bbulletslope;


void P_BulletSlope (Mob* mo)
{
    // see which target is to be aimed at
    Angle an = mo->_angle;
    bbulletslope = PP_AimLineAttack(mo, an, 16*64);

    if (!linetarget)
    {
	an += (angle_t)1<<26;
	bbulletslope = PP_AimLineAttack(mo, an, 16*64);
	if (!linetarget)
	{
	    an -= (angle_t)2<<26;
	    bbulletslope = PP_AimLineAttack(mo, an, 16*64);
	}
    }
}


//
// P_GunShot
//
void
P_GunShot
( Mob*	mo,
  bool	accurate )
{
    int damage = 5*(P_Random ()%3+1);
    Angle angle = mo->_angle;

    if (!accurate) {
        // @todo should be 4 "<<18 -> * 2^2"
        angle += (angle_t)((P_Random()-P_Random()) << 18);
    }
    PP_LineAttack(mo, angle, MMISSILERANGE, bbulletslope, damage);
}


//
// A_FirePistol
//
void
A_FirePistol
( player_t*	player,
  pspdef_t*	psp ) 
{
    S_StartSound (player->mo, sfx_pistol);

    player->mo->setState(S_PLAY_ATK2);
    player->ammo[weaponinfo[player->readyweapon].ammo]--;

    P_SetPsprite (player,
		  ps_flash,
		  weaponinfo[player->readyweapon].flashstate);

    P_BulletSlope (player->mo);
    P_GunShot (player->mo, !player->refire);
}


//
// A_FireShotgun
//
void
A_FireShotgun
( player_t*	player,
  pspdef_t*	psp ) 
{
    int		i;
	
    S_StartSound (player->mo, sfx_shotgn);
    player->mo->setState(S_PLAY_ATK2);
    
    player->ammo[weaponinfo[player->readyweapon].ammo]--;
    
    P_SetPsprite(player,
                 ps_flash,
                 weaponinfo[player->readyweapon].flashstate);
    
    P_BulletSlope (player->mo);
	
    for (i=0 ; i<7 ; i++)
        P_GunShot (player->mo, false);
}



//
// A_FireShotgun2
//
void
A_FireShotgun2
( player_t*	player,
  pspdef_t*	psp ) 
{
    int		i;
    int		damage;
		
    S_StartSound (player->mo, sfx_dshtgn);
    player->mo->setState(S_PLAY_ATK2);

    player->ammo[weaponinfo[player->readyweapon].ammo]-=2;

    P_SetPsprite (player,
		  ps_flash,
		  weaponinfo[player->readyweapon].flashstate);

    P_BulletSlope (player->mo);
	
    for (i=0 ; i<20 ; i++)
    {
	damage = 5*(P_Random ()%3+1);
	Angle angle = player->mo->_angle;
	angle += (angle_t)((P_Random()-P_Random()) << 19);
	PP_LineAttack(player->mo,
                      angle,
		      MMISSILERANGE,
		      /* << 5 on fixed == >> 11 = /2^11 = 2048*/
		      bbulletslope + (P_Random()-P_Random()) / 2048.0,
		      damage);
    }
}


//
// A_FireCGun
//
void
A_FireCGun
( player_t*	player,
  pspdef_t*	psp ) 
{
    S_StartSound (player->mo, sfx_pistol);

    if (!player->ammo[weaponinfo[player->readyweapon].ammo])
        return;
		
    player->mo->setState(S_PLAY_ATK2);
    player->ammo[weaponinfo[player->readyweapon].ammo]--;

    P_SetPsprite (player,
                  ps_flash,
                  (statenum_t)(weaponinfo[player->readyweapon].flashstate
                               + psp->state
                               - &states[S_CHAIN1]));

    P_BulletSlope (player->mo);
	
    P_GunShot (player->mo, !player->refire);
}



//
// Methods used for bumping up the light i.e. gun flashes etc. 
//
void A_Light0 (player_t *player, pspdef_t *psp)
{
    player->extralight = 0;
}

void A_Light1 (player_t *player, pspdef_t *psp)
{
    player->extralight = 1;
}

void A_Light2 (player_t *player, pspdef_t *psp)
{
    player->extralight = 2;
}


//
// A_BFGSpray
// Spawn a BFG explosion on every monster in view
//
void A_BFGSpray (Mob* mo) 
{
    int			i;
    int			j;
    int			damage;
	
    // offset angles from its attack angle
    for (i=0 ; i<40 ; i++)
    {
        Angle angle((double)mo->_angle - Angle::A45 + Angle::A90/40*i);

	// mo->target is the originator (player)
	//  of the missile
	PP_AimLineAttack(mo->target, angle, 16*64);

	if (!linetarget)
	    continue;

	PP_SpawnMobj(linetarget->xx,
                 linetarget->yy,
                 linetarget->zz + (linetarget->hheight / 4),
                 MT_EXTRABFG);
	
	damage = 0;
	for (j=0;j<15;j++)
	    damage += (P_Random()&7) + 1;

	P_DamageMobj (linetarget, mo->target,mo->target, damage);
    }
}


//
// A_BFGsound
//
void
A_BFGsound
( player_t*	player,
  pspdef_t*	psp )
{
    S_StartSound (player->mo, sfx_bfg);
}



//
// P_SetupPsprites
// Called at start of level for each player.
//
void P_SetupPsprites (player_t* player) 
{
    int	i;
	
    // remove all psprites
    for (i=0 ; i<NUMPSPRITES ; i++)
	player->psprites[i].state = NULL;
		
    // spawn the gun
    player->pendingweapon = player->readyweapon;
    P_BringUpWeapon (player);
}




//
// P_MovePsprites
// Called every tic by player thinking routine.
//
void P_MovePsprites (player_t* player) 
{
    int		i;
    pspdef_t*	psp;
    state_t*	state;
	
    psp = &player->psprites[0];
    for (i=0 ; i<NUMPSPRITES ; i++, psp++)
    {
	// a null state means not active
	if ( (state = psp->state) )	
	{
	    // drop tic count and possibly change state

	    // a -1 tic count never changes
	    if (psp->tics != -1)	
	    {
		psp->tics--;
		if (!psp->tics)
		    P_SetPsprite (player, i, psp->state->nextstate);
	    }				
	}
    }
    
    player->psprites[ps_flash].ssx = player->psprites[ps_weapon].ssx;
    player->psprites[ps_flash].ssy = player->psprites[ps_weapon].ssy;
}


