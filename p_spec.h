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
// DESCRIPTION:  none
//	Implements special effects:
//	Texture animation, height or lighting changes
//	 according to adjacent sectors, respective
//	 utility functions, etc.
//
//-----------------------------------------------------------------------------


#ifndef __P_SPEC__
#define __P_SPEC__


//
// End-level timer (-TIMER option)
//
extern	bool levelTimer;
extern	int	levelTimeCount;


//      Define values for map objects
#define MO_TELEPORTMAN          14


// at game start
void    P_InitPicAnims (void);

// at map load
void    P_SpawnSpecials (void);

// every tic
void    P_UpdateSpecials (void);

// when needed
bool
P_UseSpecialLine
( Mob*	thing,
  line_t*	line,
  int		side );

void
P_ShootSpecialLine
( Mob*	thing,
  line_t*	line );

void
P_CrossSpecialLine
( int		linenum,
  int		side,
  Mob*	thing );

void    P_PlayerInSpecialSector (player_t* player);

int
twoSided
( int		sector,
  int		line );

Sector*
getSector
( int		currentSector,
  int		line,
  int		side );

side_t*
getSide
( int		currentSector,
  int		line,
  int		side );

double PP_FindLowestFloorSurrounding(Sector* sec);
double PP_FindHighestFloorSurrounding(Sector* sec);

double
PP_FindNextHighestFloor(Sector* sec, double currentheight);

double PP_FindLowestCeilingSurrounding(Sector* sec);
double PP_FindHighestCeilingSurrounding(Sector* sec);

int
P_FindSectorFromLineTag
( line_t*	line,
  int		start );

int
P_FindMinSurroundingLight
( Sector*	sector,
  int		max );

Sector*
getNextSector
( line_t*	line,
  Sector*	sec );


//
// SPECIAL
//
int EV_DoDonut(line_t* line);



//
// P_LIGHTS
//
typedef struct
{
    thinker_t	thinker;
    Sector*	sector;
    int		count;
    int		maxlight;
    int		minlight;
    
} fireflicker_t;



typedef struct
{
    thinker_t	thinker;
    Sector*	sector;
    int		count;
    int		maxlight;
    int		minlight;
    int		maxtime;
    int		mintime;
    
} lightflash_t;



typedef struct
{
    thinker_t	thinker;
    Sector*	sector;
    int		count;
    int		minlight;
    int		maxlight;
    int		darktime;
    int		brighttime;
    
} strobe_t;




typedef struct
{
    thinker_t	thinker;
    Sector*	sector;
    int		minlight;
    int		maxlight;
    int		direction;

} glow_t;


#define GLOWSPEED			8
#define STROBEBRIGHT		5
#define FASTDARK			15
#define SLOWDARK			35

void    P_SpawnFireFlicker (Sector* sector);
void    T_LightFlash (lightflash_t* flash);
void    P_SpawnLightFlash (Sector* sector);
void    T_StrobeFlash (strobe_t* flash);

void
P_SpawnStrobeFlash
(Sector*	sector,
  int		fastOrSlow,
  int		inSync );

void    EV_StartLightStrobing(line_t* line);
void    EV_TurnTagLightsOff(line_t* line);

void
EV_LightTurnOn
( line_t*	line,
  int		bright );

void    T_Glow(glow_t* g);
void    P_SpawnGlowingLight(Sector* sector);




//
// P_SWITCH
//
typedef struct
{
    char	name1[9];
    char	name2[9];
    short	episode;
    
} switchlist_t;


typedef enum
{
    top,
    middle,
    bottom

} bwhere_e;


typedef struct
{
    line_t*	line;
    bwhere_e	where;
    int		btexture;
    int		btimer;
    Mob*	soundorg;

} button_t;




 // max # of wall switches in a level
#define MAXSWITCHES		50

 // 4 players, 4 buttons each at once, max.
#define MAXBUTTONS		16

 // 1 second, in ticks. 
#define BUTTONTIME      35             

extern button_t	buttonlist[MAXBUTTONS]; 

void
P_ChangeSwitchTexture
( line_t*	line,
  int		useAgain );

void P_InitSwitchList(void);


//
// P_PLATS
//
typedef enum
{
    up,
    down,
    waiting,
    in_stasis

} plat_e;



typedef enum
{
    perpetualRaise,
    downWaitUpStay,
    raiseAndChange,
    raiseToNearestAndChange,
    blazeDWUS

} plattype_e;



typedef struct
{
    thinker_t	thinker;
    Sector*	sector;
    double	sspeed;
    double	llow;
    double	hhigh;
    int		wait;
    int		count;
    plat_e	status;
    plat_e	oldstatus;
    bool	crush;
    int		tag;
    plattype_e	type;
    
} plat_t;



#define PLATWAIT		3
#define PPLATSPEED		1.0
#define MAXPLATS		30


extern plat_t*	activeplats[MAXPLATS];

void    T_PlatRaise(plat_t*	plat);

int
EV_DoPlat
( line_t*	line,
  plattype_e	type,
  int		amount );

void    P_AddActivePlat(plat_t* plat);
void    P_RemoveActivePlat(plat_t* plat);
void    EV_StopPlat(line_t* line);
void    P_ActivateInStasis(int tag);


//
// P_DOORS
//
typedef enum
{
    normal,
    close30ThenOpen,
    close,
    open,
    raiseIn5Mins,
    blazeRaise,
    blazeOpen,
    blazeClose

} vldoor_e;



typedef struct
{
    thinker_t	thinker;
    vldoor_e	type;
    Sector*	sector;
    double	ttopheight;
    double	sspeed;

    // 1 = up, 0 = waiting at top, -1 = down
    int             direction;
    
    // tics to wait at the top
    int             topwait;
    // (keep in case a door going down is reset)
    // when it reaches 0, start going down
    int             topcountdown;
    
} vldoor_t;



#define VVDOORSPEED		2
#define VDOORWAIT		150

void
EV_VerticalDoor
( line_t*	line,
  Mob*	thing );

int
EV_DoDoor
( line_t*	line,
  vldoor_e	type );

int
EV_DoLockedDoor
( line_t*	line,
  vldoor_e	type,
  Mob*	thing );

void    T_VerticalDoor (vldoor_t* door);
void    P_SpawnDoorCloseIn30 (Sector* sec);

void
P_SpawnDoorRaiseIn5Mins
( Sector*	sec,
  int		secnum );



#if 0 // UNUSED
//
//      Sliding doors...
//
typedef enum
{
    sd_opening,
    sd_waiting,
    sd_closing

} sd_e;



typedef enum
{
    sdt_openOnly,
    sdt_closeOnly,
    sdt_openAndClose

} sdt_e;




typedef struct
{
    thinker_t	thinker;
    sdt_e	type;
    line_t*	line;
    int		frame;
    int		whichDoorIndex;
    int		timer;
    Sector*	frontsector;
    Sector*	backsector;
    sd_e	 status;

} slidedoor_t;



typedef struct
{
    char	frontFrame1[9];
    char	frontFrame2[9];
    char	frontFrame3[9];
    char	frontFrame4[9];
    char	backFrame1[9];
    char	backFrame2[9];
    char	backFrame3[9];
    char	backFrame4[9];
    
} slidename_t;



typedef struct
{
    int             frontFrames[4];
    int             backFrames[4];

} slideframe_t;



// how many frames of animation
#define SNUMFRAMES		4

#define SDOORWAIT		35*3
#define SWAITTICS		4

// how many diff. types of anims
#define MAXSLIDEDOORS	5                            

void P_InitSlidingDoorFrames(void);

void
EV_SlidingDoor
( line_t*	line,
  Mob*	thing );
#endif



//
// P_CEILNG
//
typedef enum
{
    lowerToFloor,
    raiseToHighest,
    lowerAndCrush,
    crushAndRaise,
    fastCrushAndRaise,
    silentCrushAndRaise

} ceiling_e;



typedef struct
{
    thinker_t	thinker;
    ceiling_e	type;
    Sector*	sector;
    double bbottomheight;
    double ttopheight;
    double sspeed;
    bool	crush;

    // 1 = up, 0 = waiting, -1 = down
    int		direction;

    // ID
    int		tag;                   
    int		olddirection;
    
} ceiling_t;





#define CCEILSPEED		1
#define CEILWAIT		150
#define MAXCEILINGS		30

extern ceiling_t*	activeceilings[MAXCEILINGS];

int
EV_DoCeiling
( line_t*	line,
  ceiling_e	type );

void    T_MoveCeiling (ceiling_t* ceiling);
void    P_AddActiveCeiling(ceiling_t* c);
void    P_RemoveActiveCeiling(ceiling_t* c);
int	EV_CeilingCrushStop(line_t* line);
void    P_ActivateInStasisCeiling(line_t* line);


//
// P_FLOOR
//
typedef enum
{
    // lower floor to highest surrounding floor
    lowerFloor,
    
    // lower floor to lowest surrounding floor
    lowerFloorToLowest,
    
    // lower floor to highest surrounding floor VERY FAST
    turboLower,
    
    // raise floor to lowest surrounding CEILING
    raiseFloor,
    
    // raise floor to next highest surrounding floor
    raiseFloorToNearest,

    // raise floor to shortest height texture around it
    raiseToTexture,
    
    // lower floor to lowest surrounding floor
    //  and change floorpic
    lowerAndChange,
  
    raiseFloor24,
    raiseFloor24AndChange,
    raiseFloorCrush,

     // raise to next highest floor, turbo-speed
    raiseFloorTurbo,       
    donutRaise,
    raiseFloor512
    
} floor_e;




typedef enum
{
    build8,	// slowly build by 8
    turbo16	// quickly build by 16
    
} stair_e;



typedef struct
{
    thinker_t	thinker;
    floor_e	type;
    bool	crush;
    Sector*	sector;
    int		direction;
    int		newspecial;
    short	texture;
    double	ffloordestheight;
    double	sspeed;

} floormove_t;



#define FFLOORSPEED		1.0

typedef enum
{
    ok,
    crushed,
    pastdest
    
} result_e;

result_e
TT_MovePlane(Sector*	sector,
             double speed,
             double dest,
             bool crush,
             int floorOrCeiling,
             int direction);

int
EV_BuildStairs
( line_t*	line,
  stair_e	type );

int
EV_DoFloor
( line_t*	line,
  floor_e	floortype );

void T_MoveFloor( floormove_t* floor);

//
// P_TELEPT
//
int
EV_Teleport
( line_t*	line,
  int		side,
  Mob*	thing );

#endif
//-----------------------------------------------------------------------------
//
// $Log:$
//
//-----------------------------------------------------------------------------
