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
// DESCRIPTION:
//	Play functions, animation, global header.
//
//-----------------------------------------------------------------------------


#ifndef __P_LOCAL__
#define __P_LOCAL__

#ifndef __R_LOCAL__
#include "r_local.h"
#endif

#include "BlockMap.hh"
#include "DivLine.hh"

#define FFLOATSPEED		4


#define MAXHEALTH		100
#define VVIEWHEIGHT		41

// mapblocks are used to check movement
// against lines and things
#define MAPBLOCKUNITS	128
#define MAPBLOCKSIZE	(MAPBLOCKUNITS*FRACUNIT)
#define MAPBLOCKSHIFT	(FRACBITS+7)
#define DOUBLE_MAPBLOCKS_DIV 128 // 2^7 
#define MAPBMASK		(MAPBLOCKSIZE-1)
#define MAPBTOFRAC		(MAPBLOCKSHIFT-FRACBITS)  // 16 + 7 - 16


// player radius for movement checking
#define PPLAYERRADIUS	16

// MAXRADIUS is for precalculated sector block boxes
// the spider demon is larger,
// but we do not have any moving sectors nearby
#define MMAXRADIUS		32

#define GGRAVITY		1
#define MMAXMOVE		30

#define USERANGE		(64*FRACUNIT)
#define UUSERANGE		(64)
#define MMELEERANGE		64
#define MMISSILERANGE	        (32*64)

// follow a player exlusively for 3 seconds
#define	BASETHRESHOLD	 	100



//
// P_TICK
//

// both the head and tail of the thinker list
extern	thinker_t	thinkercap;	


void P_InitThinkers (void);
void P_AddThinker (thinker_t* thinker);
void P_RemoveThinker (thinker_t* thinker);


//
// P_PSPR
//
void P_SetupPsprites (player_t* curplayer);
void P_MovePsprites (player_t* curplayer);
void P_DropWeapon (player_t* player);


//
// P_USER
//
void	P_PlayerThink (player_t* player);


//
// P_MOBJ
//

#define ONFLOORZ		INT_MAX
#define ONCEILINGZ		INT_MAX

// Time interval for item respawning.
#define ITEMQUESIZE		128

extern MapThing  	itemrespawnque[ITEMQUESIZE];
extern int		itemrespawntime[ITEMQUESIZE];
extern int		iquehead;
extern int		iquetail;


void P_RespawnSpecials (void);

Mob* PP_SpawnMobj(const Vertex& v, double z, mobjtype_t type);

void P_RemoveMobj (Mob* th);
bool P_SetMobjState (Mob* mobj, statenum_t state);
void P_MobjThinker (Mob* mobj);

void PP_SpawnPuff(const Vertex& v, double z);
void PP_SpawnBlood(const Vertex& v, double z, int damage);
Mob* P_SpawnMissile (Mob* source, Mob* dest, mobjtype_t type);
void P_SpawnPlayerMissile (Mob* source, mobjtype_t type);


//
// P_ENEMY
//
void P_NoiseAlert (Mob* target, Mob* emmiter);


//
// P_MAPUTL
//

typedef struct
{
    double fraction;		// along trace line
    bool	isaline;
    union {
	Mob*	thing;
	Line*	line;
    }			d;
} intercept_t;

#define MAXINTERCEPTS	128

extern intercept_t	intercepts[MAXINTERCEPTS];
extern intercept_t*	intercept_p;

typedef bool (*traverser_t) (intercept_t *in);

double PP_AproxDistance(double dx, double dy);
int P_PointOnLineSide (const Vertex& v, Line* line);
int PP_BoxOnLineSide (double* tmbox, Line* ld);

extern double oopentop;
extern double oopenbottom;
extern double oopenrange;
extern double llowfloor;

void P_LineOpening (Line* linedef);

bool P_BlockLinesIterator (int x, int y, bool(*func)(Line*) );
bool P_BlockThingsIterator (int x, int y, bool(*func)(Mob*) );

#define PT_ADDLINES		1
#define PT_ADDTHINGS	2
#define PT_EARLYOUT		4

extern DivLine trace;

bool
P_PathTraverse(
    const Vertex& v1,
    const Vertex& v2,
    int flags,
    bool (*trav) (intercept_t *));

void P_UnsetThingPosition (Mob* thing);
void P_SetThingPosition (Mob* thing);


//
// P_MAP
//

// If "floatok" true, move would be ok
// if within "tmfloorz - tmceilingz".
extern bool		floatok;
extern double		ttmfloorz;
extern double		ttmceilingz;


extern Line*		ceilingline;

bool PP_CheckPosition (Mob *thing, const Vertex& v);
bool PP_TryMove (Mob* thing, const Vertex& v);
bool PP_TeleportMove (Mob* thing, const Vertex& v);
void P_SlideMove (Mob* mo);
bool P_CheckSight (Mob* t1, Mob* t2);
void P_UseLines (player_t* player);

bool P_ChangeSector (Sector* sector, bool crunch);

extern Mob*	linetarget;	// who got hit (or NULL)

double
PP_AimLineAttack(Mob* t1,
                 Angle angle,
                 double distance);

void
PP_LineAttack(Mob* t1,
              Angle angle,
              double distance,
              double slope,
              int damage );

void
P_RadiusAttack
( Mob*	spot,
  Mob*	source,
  int		damage );



//
// P_SETUP
//
extern byte* rejectmatrix;	// for fast sight rejection
extern BlockMap blockMap;

//
// P_INTER
//
extern int		maxammo[NUMAMMO];
extern int		clipammo[NUMAMMO];

void
P_TouchSpecialThing
( Mob*	special,
  Mob*	toucher );

void
P_DamageMobj
( Mob*	target,
  Mob*	inflictor,
  Mob*	source,
  int		damage );


//
// P_SPEC
//
#include "p_spec.h"


#endif	// __P_LOCAL__
//-----------------------------------------------------------------------------
//
// $Log:$
//
//-----------------------------------------------------------------------------


