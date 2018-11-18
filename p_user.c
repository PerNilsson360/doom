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
//	Player related stuff.
//	Bobbing POV/weapon, movement.
//	Pending weapon.
//
//-----------------------------------------------------------------------------


static const char
rcsid[] = "$Id: p_user.c,v 1.3 1997/01/28 22:08:29 b1 Exp $";


#include "doomdef.h"
#include "d_event.h"

#include "p_local.h"

#include "doomstat.h"



// Index of the special effects (INVUL inverse) map.
#define INVERSECOLORMAP		32


//
// Movement.
//

// 16 pixels of bob (old max bob was 16 pixels)

#define MAXBOB 8	
#define BOB_ANGLE 0.1
#define MAX_BOB_ANGLE 0.5


bool		onground;

void
PP_Thrust(player_t* player, double angle, double move ) 
{
    player->mo->mmomx += move * cos(angle);
    player->mo->mmomy += move * sin(angle);
}

//
// P_CalcHeight
// Calculate the walking / running height adjustment
//
void P_CalcHeight (player_t* player) 
{
    // Regular movement bobbing
    // (needs to be calculated for gun swing
    // even if not on ground)
    // OPTIMIZE: tablify angle
    // Note: a LUT allows for effects
    //  like a ramp with low health.
    // @todo bob shold be dependent on player time
    player->bbob += 
        fmod(fabs((player->mo->mmomx * player->mo->mmomx) + 
                  (player->mo->mmomy * player->mo->mmomy)) / 400,
             2 * M_PI);

    if ((player->cheats & CF_NOMOMENTUM) || !onground)
    {
        player->vviewz = player->mo->zz + VVIEWHEIGHT;

        if (player->vviewz > player->mo->cceilingz - 4)
            player->vviewz = player->mo->cceilingz - 4;

        // @todo this looks funny is vviews allready set above
        player->vviewz = player->mo->zz + player->vviewheight;
        return;
    }

    double bob = MAXBOB * sin(player->bbob);
    
    // move viewheight
    if (player->playerstate == PST_LIVE)
    {
	player->vviewheight += player->ddeltaviewheight;

	if (player->vviewheight > VVIEWHEIGHT)
	{
	    player->vviewheight = VVIEWHEIGHT;
	    player->ddeltaviewheight = 0;
	}

	if (player->vviewheight < VVIEWHEIGHT/2)
	{
	    player->vviewheight = VVIEWHEIGHT/2;
	    if (player->ddeltaviewheight <= 0)
            player->ddeltaviewheight = 1;
	}
	
	if (player->ddeltaviewheight)	
	{
	    player->ddeltaviewheight += 1/4;
	    if (!player->ddeltaviewheight)
            player->ddeltaviewheight = 1;
	}
    }

    player->vviewz = player->mo->zz + player->vviewheight + bob;
    if (player->vviewz > player->mo->cceilingz - 4)
        player->vviewz = player->mo->cceilingz - 4;
}



//
// P_MovePlayer
//
void P_MovePlayer (player_t* player)
{
    ticcmd_t*		cmd;
	
    cmd = &player->cmd;
    
    player->mo->_angle += Angle((angle_t)(cmd->angleturn<<16));
    
    double a = player->mo->_angle;

    // Do not let the player control movement
    //  if not onground.
    onground = (player->mo->zz <= player->mo->ffloorz);
	
    if (cmd->forwardmove && onground)
        PP_Thrust(player, a, cmd->forwardmove / 32.0);
        
    if (cmd->sidemove && onground)
        PP_Thrust(player, a - (M_PI/2), cmd->sidemove / 32.0);
    
    if ( (cmd->forwardmove || cmd->sidemove) 
         && player->mo->state == &states[S_PLAY] )
    {
        player->mo->setState(S_PLAY_RUN1);
    }
}	



//
// P_DeathThink
// Fall on your face when dying.
// Decrease POV height to floor height.
//
#define ANG5   	(ANG90/18)

void P_DeathThink (player_t* player)
{
    P_MovePsprites (player);
	
    // fall to the ground
    if (player->vviewheight > 6)
        player->vviewheight -= 1;

    if (player->vviewheight < 6)
        player->vviewheight = 6;

    player->ddeltaviewheight = 0;
    onground = (player->mo->zz <= player->mo->ffloorz);
    P_CalcHeight (player);
	
    if (player->attacker && player->attacker != player->mo)
    {
        Angle angle(player->mo->xx,
                    player->mo->yy,
                    player->attacker->xx,
                    player->attacker->yy);

        // @todo introduce a DiffAngle ?
        double delta = (double)angle - (double)player->mo->_angle;
	
        if (delta < (double)Angle::A5 || delta > -(double)Angle::A5)
        {
            // Looking at killer,
            //  so fade damage flash down.
            player->mo->_angle = angle;
            
            if (player->damagecount)
                player->damagecount--;
        }
        else if (delta < (double)Angle::A180)
            player->mo->_angle += Angle::A5;
        else
	    player->mo->_angle -= Angle::A5;
    }
    else if (player->damagecount)
        player->damagecount--;
	
    
    if (player->cmd.buttons & BT_USE)
        player->playerstate = PST_REBORN;
}



//
// P_PlayerThink
//
void P_PlayerThink (player_t* player)
{
    ticcmd_t*		cmd;
    weapontype_t	newweapon;
	
    // fixme: do this in the cheat code
    if (player->cheats & CF_NOCLIP)
	player->mo->flags |= MF_NOCLIP;
    else
	player->mo->flags &= ~MF_NOCLIP;
    
    // chain saw run forward
    cmd = &player->cmd;
    if (player->mo->flags & MF_JUSTATTACKED)
    {
	cmd->angleturn = 0;
	cmd->forwardmove = 0xc800/512;
	cmd->sidemove = 0;
	player->mo->flags &= ~MF_JUSTATTACKED;
    }
			
	
    if (player->playerstate == PST_DEAD)
    {
	P_DeathThink (player);
	return;
    }
    
    // Move around.
    // Reactiontime is used to prevent movement
    //  for a bit after a teleport.
    if (player->mo->reactiontime)
	player->mo->reactiontime--;
    else
	P_MovePlayer (player);
    
    P_CalcHeight (player);

    if (player->mo->subsector->sector->special)
	P_PlayerInSpecialSector (player);
    
    // Check for weapon change.

    // A special event has no other buttons.
    if (cmd->buttons & BT_SPECIAL)
	cmd->buttons = 0;			
		
    if (cmd->buttons & BT_CHANGE)
    {
	// The actual changing of the weapon is done
	//  when the weapon psprite can do it
	//  (read: not in the middle of an attack).
        newweapon = (weapontype_t)((cmd->buttons&BT_WEAPONMASK)>>BT_WEAPONSHIFT);
	
	if (newweapon == wp_fist
	    && player->weaponowned[wp_chainsaw]
	    && !(player->readyweapon == wp_chainsaw
		 && player->powers[pw_strength]))
	{
	    newweapon = wp_chainsaw;
	}
	
	if ( (gamemode == commercial)
	    && newweapon == wp_shotgun 
	    && player->weaponowned[wp_supershotgun]
	    && player->readyweapon != wp_supershotgun)
	{
	    newweapon = wp_supershotgun;
	}
	

	if (player->weaponowned[newweapon]
	    && newweapon != player->readyweapon)
	{
	    // Do not go to plasma or BFG in shareware,
	    //  even if cheated.
	    if ((newweapon != wp_plasma
		 && newweapon != wp_bfg)
		|| (gamemode != shareware) )
	    {
		player->pendingweapon = newweapon;
	    }
	}
    }
    
    // check for use
    if (cmd->buttons & BT_USE)
    {
	if (!player->usedown)
	{
	    P_UseLines (player);
	    player->usedown = true;
	}
    }
    else
	player->usedown = false;
    
    // cycle psprites
    P_MovePsprites (player);
    
    // Counters, time dependend power ups.

    // Strength counts up to diminish fade.
    if (player->powers[pw_strength])
	player->powers[pw_strength]++;	
		
    if (player->powers[pw_invulnerability])
	player->powers[pw_invulnerability]--;

    if (player->powers[pw_invisibility])
	if (! --player->powers[pw_invisibility] )
	    player->mo->flags &= ~MF_SHADOW;
			
    if (player->powers[pw_infrared])
	player->powers[pw_infrared]--;
		
    if (player->powers[pw_ironfeet])
	player->powers[pw_ironfeet]--;
		
    if (player->damagecount)
	player->damagecount--;
		
    if (player->bonuscount)
	player->bonuscount--;

    
    // Handling colormaps.
    if (player->powers[pw_invulnerability])
    {
	if (player->powers[pw_invulnerability] > 4*32
	    || (player->powers[pw_invulnerability]&8) )
	    player->fixedcolormap = INVERSECOLORMAP;
	else
	    player->fixedcolormap = 0;
    }
    else if (player->powers[pw_infrared])	
    {
	if (player->powers[pw_infrared] > 4*32
	    || (player->powers[pw_infrared]&8) )
	{
	    // almost full bright
	    player->fixedcolormap = 1;
	}
	else
	    player->fixedcolormap = 0;
    }
    else
	player->fixedcolormap = 0;
}


