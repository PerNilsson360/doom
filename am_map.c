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
//
// $Log:$
//
// DESCRIPTION:  the automap code
//
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <limits.h>
#include <limits>
#include <iostream>
#include <memory>

#include "z_zone.h"
#include "doomdef.h"
#include "st_stuff.h"
#include "p_local.h"
#include "w_wad.h"

#include "m_cheat.h"
#include "i_system.h"

// Needs access to LFB.
#include "v_video.h"

// State.
#include "doomstat.h"
#include "r_state.h"

// Data.
#include "dstrings.h"
#include "m_swap.h"
#include "am_map.h"

#include "ImageScaler.hh"


// For use if I do walls with outsides/insides
#define REDS		(256-5*16)
#define REDRANGE	16
#define BLUES		(256-4*16+8)
#define BLUERANGE	8
#define GREENS		(7*16)
#define GREENRANGE	16
#define GRAYS		(6*16)
#define GRAYSRANGE	16
#define BROWNS		(4*16)
#define BROWNRANGE	16
#define YELLOWS		(256-32+7)
#define YELLOWRANGE	1
#define BLACK		0
#define WHITE		(256-47)

// Automap colors
#define BACKGROUND	BLACK
#define YOURCOLORS	WHITE
#define YOURRANGE	0
#define WALLCOLORS	REDS
#define WALLRANGE	REDRANGE
#define TSWALLCOLORS	GRAYS
#define TSWALLRANGE	GRAYSRANGE
#define FDWALLCOLORS	BROWNS
#define FDWALLRANGE	BROWNRANGE
#define CDWALLCOLORS	YELLOWS
#define CDWALLRANGE	YELLOWRANGE
#define THINGCOLORS	GREENS
#define THINGRANGE	GREENRANGE
#define SECRETWALLCOLORS WALLCOLORS
#define SECRETWALLRANGE WALLRANGE
#define GRIDCOLORS	(GRAYS + GRAYSRANGE/2)
#define GRIDRANGE	0
#define XHAIRCOLORS	GRAYS

// drawing stuff

#define AM_PANDOWNKEY	KEY_DOWNARROW
#define AM_PANUPKEY	KEY_UPARROW
#define AM_PANRIGHTKEY	KEY_RIGHTARROW
#define AM_PANLEFTKEY	KEY_LEFTARROW
#define AM_ZOOMINKEY	'='
#define AM_ZOOMOUTKEY	'-'
#define AM_STARTKEY	KEY_TAB
#define AM_ENDKEY	KEY_TAB
#define AM_GOBIGKEY	'0'
#define AM_FOLLOWKEY	'f'
#define AM_GRIDKEY	'g'
#define AM_MARKKEY	'm'
#define AM_CLEARMARKKEY	'c'

#define AM_NUMMARKPOINTS 10

// scale on entry
#define INITSCALEMTOF 0.2
// how much the automap moves window per tic in frame-buffer coordinates
// moves 140 pixels in 1 second
#define F_PANINC	4
// how much zoom-in per tic
// goes to 2x in 1 second
#define MM_ZOOMIN (1.02)
// how much zoom-out per tic
// pulls out to 0.5x in 1 second
#define MM_ZOOMOUT (1/1.02)

// the following is crap
#define LINE_NEVERSEE ML_DONTDRAW

typedef struct
{
    int x, y;
} fpoint_t;

typedef struct
{
    fpoint_t a, b;
} fline_t;

class MPoint
{
public:
    MPoint() : _x(0), _y(0) {}
    MPoint(double x, double y) : _x(x), _y(y) {}
    void setXX(double x) { _x = x; }
    void setYY(double y) { _y = y; }
    void incX(double x) { _x += x; }
    void incY(double y) { _y += y; }
    double getXX() const { return _x; }
    double getYY() const { return _y; }
    void rotate(Angle a) {
	double x = _x * cos(a) - _y * sin(a);
	_y   = _x * sin(a) + _y * cos(a);
	_x = x;
    }
    void scale(double scale) {
	_x *= scale;
	_y *= scale;
    }
private:
    friend std::ostream& operator << (std::ostream& out, const MPoint& p);
    double _x;
    double _y;
};

std::ostream& operator << (std::ostream& out, const MPoint& p) {
    out << "<"<< p._x << ", " << p._y << ">";
    return out;
}

class MLine
{
public:
    MLine() : _a(), _b() {}
    MLine(const MPoint& a, const MPoint& b) : _a(a), _b(b) {}
    MPoint _a;
    MPoint _b;
};

std::ostream& operator << (std::ostream& out, const MLine& l) {
    out << "[" << l._a << ", " << l._b; 
    return out;
}

#define PLAYERRADIUS	16.0

//
// The vector graphics for the automap.
//  A line drawing of the player pointing right,
//   starting from the middle.
//
#define R ((8*PLAYERRADIUS)/7)
MLine player_arrow[] = {
    { { -R+R/8, 0 }, { R, 0 } }, // -----
    { { R, 0 }, { R-R/2, R/4 } },  // ----->
    { { R, 0 }, { R-R/2, -R/4 } },
    { { -R+R/8, 0 }, { -R-R/8, R/4 } }, // >---->
    { { -R+R/8, 0 }, { -R-R/8, -R/4 } },
    { { -R+3*R/8, 0 }, { -R+R/8, R/4 } }, // >>--->
    { { -R+3*R/8, 0 }, { -R+R/8, -R/4 } }
};
#undef R
#define NUMPLYRLINES (sizeof(player_arrow)/sizeof(MLine))

#define R ((8*PLAYERRADIUS)/7)
MLine cheat_player_arrow[] = {
    { { -R+R/8, 0 }, { R, 0 } }, // -----
    { { R, 0 }, { R-R/2, R/6 } },  // ----->
    { { R, 0 }, { R-R/2, -R/6 } },
    { { -R+R/8, 0 }, { -R-R/8, R/6 } }, // >----->
    { { -R+R/8, 0 }, { -R-R/8, -R/6 } },
    { { -R+3*R/8, 0 }, { -R+R/8, R/6 } }, // >>----->
    { { -R+3*R/8, 0 }, { -R+R/8, -R/6 } },
    { { -R/2, 0 }, { -R/2, -R/6 } }, // >>-d--->
    { { -R/2, -R/6 }, { -R/2+R/6, -R/6 } },
    { { -R/2+R/6, -R/6 }, { -R/2+R/6, R/4 } },
    { { -R/6, 0 }, { -R/6, -R/6 } }, // >>-dd-->
    { { -R/6, -R/6 }, { 0, -R/6 } },
    { { 0, -R/6 }, { 0, R/4 } },
    { { R/6, R/4 }, { R/6, -R/7 } }, // >>-ddt->
    { { R/6, -R/7 }, { R/6+R/32, -R/7-R/32 } },
    { { R/6+R/32, -R/7-R/32 }, { R/6+R/10, -R/7 } }
};
#undef R
#define NUMCHEATPLYRLINES (sizeof(cheat_player_arrow)/sizeof(MLine))

MLine triangle_guy[] = {
    { { -.867, -.5 }, { .867, -.5 } },
    { { .867, -.5 } , { 0, 1 } },
    { { 0, 1 }, { -.867, -.5 } }
};
#define NUMTRIANGLEGUYLINES (sizeof(triangle_guy)/sizeof(MLine))

MLine thintriangle_guy[] = {
    { { -.5, -.7 }, { 1, 0 } },
    { { 1, 0 }, { -.5, .7 } },
    { { -.5, .7 }, { -.5, -.7 } }
};
#define NUMTHINTRIANGLEGUYLINES (sizeof(thintriangle_guy)/sizeof(MLine))

static int 	cheating = 0;
static int 	grid = 0;
static int 	leveljuststarted = 1; 	// kluge until AM_LevelInit() is called
bool    	automapactive = false;
static int 	lightlev; 		// used for funky strobing effect

namespace
{
std::unique_ptr<ImageScaler> mapView(
    new ImageScaler(BASE_WIDTH, BASE_HEIGHT));
}

static int 	amclock;

static MPoint m_paninc; // how far the window pans each tic (map coords)
static double mmtof_zoommul; // how far the window zooms in each tic (map coords)
static double fftom_zoommul; // how far the window zooms in each tic (fb coords)

static double mm_x, mm_y;   // LL x,y where the window is on the map (map coords)
static double mm_x2, mm_y2; // UR x,y where the window is on the map (map coords)

//
// width/height of window on map (map coords)
//
static double mm_w;
static double mm_h;

// based on level size
static double mmin_x;
static double mmin_y; 
static double mmax_x;
static double mmax_y;

static double mmin_scale_mtof; // used to tell when to stop zooming out
static double mmax_scale_mtof; // used to tell when to stop zooming in

// old stuff for recovery later
static double oold_m_w, oold_m_h;
static double oold_m_x, oold_m_y;

// old location used by the Follower routine
static MPoint f_oldloc;

// used by MTOF to scale from map-to-frame-buffer coords
static double sscale_mtof = INITSCALEMTOF;
// used by FTOM to scale from frame-buffer-to-map coords (=1/scale_mtof)
static double sscale_ftom;

// translates between frame-buffer and map distances
inline double FFTOM(int x) { return x * sscale_ftom; }
inline int MMTOF(double x) { return x * sscale_mtof; }

static player_t *plr; // the player represented by an arrow

static patch_t *marknums[10]; // numbers used for marking by the automap
static MPoint markpoints[AM_NUMMARKPOINTS]; // where the points are
static int markpointnum = 0; // next point to be assigned

static int followplayer = 1; // specifies whether to follow the player around

static unsigned char cheat_amap_seq[] = { 0xb2, 0x26, 0x26, 0x2e, 0xff };
static cheatseq_t cheat_amap = { cheat_amap_seq, 0 };

static bool stopped = true;

extern bool viewactive;

// translates between frame-buffer and map coordinates
inline int CCYMTOF(double y) { return BASE_HEIGHT - MMTOF(y - mm_y); }
inline int CCXMTOF(double x) { return MMTOF( x - mm_x); }

//
//
//
void AM_activateNewScale(void)
{
    mm_x += mm_w/2;
    mm_y += mm_h/2;
    mm_w = FFTOM(BASE_WIDTH);
    mm_h = FFTOM(BASE_HEIGHT);
    mm_x -= mm_w/2;
    mm_y -= mm_h/2;
    mm_x2 = mm_x + mm_w;
    mm_y2 = mm_y + mm_h;
}

//
//
//
void AM_saveScaleAndLoc(void)
{
    oold_m_x = mm_x;
    oold_m_y = mm_y;
    oold_m_w = mm_w;
    oold_m_h = mm_h;
}

//
//
//
void AM_restoreScaleAndLoc(void)
{

    mm_w = oold_m_w;
    mm_h = oold_m_h;
    if (!followplayer)
    {
	mm_x = oold_m_x;
	mm_y = oold_m_y;
    } else {
	mm_x = plr->mo->position.getX() - mm_w/2;
	mm_y = plr->mo->position.getY() - mm_h/2;
    }
    mm_x2 = mm_x + mm_w;
    mm_y2 = mm_y + mm_h;

    // Change the scaling multipliers
    sscale_mtof = BASE_WIDTH / mm_w;
    sscale_ftom = 1  / sscale_mtof;
}

//
// adds a marker at the current location
//
void AM_addMark(void)
{
    markpoints[markpointnum] = MPoint(mm_x + mm_w/2, mm_y + mm_h/2);
    markpointnum = (markpointnum + 1) % AM_NUMMARKPOINTS;
    
}

//
// Determines bounding box of all vertices,
// sets global variables controlling zoom range.
//
void AM_findMinMaxBoundaries(void)
{
    mmin_x = mmin_y =  std::numeric_limits<double>::max();
    mmax_x = mmax_y = -std::numeric_limits<double>::max();
  
    for (size_t i = 0; i < vertexes.size(); i++)
    {
	if (vertexes[i].getX() < mmin_x)
	    mmin_x = vertexes[i].getX();
	else if (vertexes[i].getX() > mmax_x)
	    mmax_x = vertexes[i].getX();
    
	if (vertexes[i].getY() < mmin_y)
	    mmin_y = vertexes[i].getY();
	else if (vertexes[i].getY() > mmax_y)
	    mmax_y = vertexes[i].getY();
    }
  
    double mmax_w = mmax_x - mmin_x;
    double mmax_h = mmax_y - mmin_y;

    double a = BASE_WIDTH / mmax_w;
    double b = BASE_HEIGHT / mmax_h;
  
    mmin_scale_mtof = a < b ? a : b;

    mmax_scale_mtof = BASE_HEIGHT / 2*PLAYERRADIUS;
}

//
//
//
void AM_changeWindowLoc(void)
{
    if (m_paninc.getXX() != 0.0 || m_paninc.getYY() != 0.0)
    {
	followplayer = 0;
	f_oldloc.setXX(std::numeric_limits<double>::max());
    }

    mm_x += m_paninc.getXX();
    mm_y += m_paninc.getYY();

    if (mm_x + mm_w/2 > mmax_x)
	mm_x = mmax_x - mm_w/2;
    else if (mm_x + mm_w/2 < mmin_x)
	mm_x = mmin_x - mm_w/2;
  
    if (mm_y + mm_h/2 > mmax_y)
	mm_y = mmax_y - mm_h/2;
    else if (mm_y + mm_h/2 < mmin_y)
	mm_y = mmin_y - mm_h/2;

    mm_x2 = mm_x + mm_w;
    mm_y2 = mm_y + mm_h;
}


//
//
//
void AM_initVariables(void)
{
    int pnum;
    static event_t st_notify = { ev_keyup, AM_MSGENTERED };

    automapactive = true;

    f_oldloc.setXX(std::numeric_limits<double>::max());
    amclock = 0;
    lightlev = 0;

    m_paninc = MPoint(0, 0);
    fftom_zoommul = 1;
    mmtof_zoommul = 1;

    mm_w = FFTOM(BASE_WIDTH);
    mm_h = FFTOM(BASE_HEIGHT);

    // find player to center on initially
    if (!playeringame[pnum = consoleplayer])
	for (pnum=0;pnum<MAXPLAYERS;pnum++)
	    if (playeringame[pnum])
		break;
  
    plr = &players[pnum];
    mm_x = plr->mo->position.getX() - mm_w/2;
    mm_y = plr->mo->position.getY()  - mm_h/2;
    AM_changeWindowLoc();

    // for saving & restoring
    oold_m_x = mm_x;
    oold_m_y = mm_y;
    oold_m_w = mm_w;
    oold_m_h = mm_h;

    // inform the status bar of the change
    ST_Responder(&st_notify);
}

//
// 
//
void AM_loadPics(void)
{
    int i;
    char namebuf[9];
  
    for (i=0;i<10;i++)
    {
	sprintf(namebuf, "AMMNUM%d", i);
	marknums[i] = (patch_t*)W_CacheLumpName(namebuf, PU_STATIC);
    }

}

void AM_unloadPics(void)
{
    int i;
  
    for (i=0;i<10;i++)
	Z_ChangeTag(marknums[i], PU_CACHE);

}

void AM_clearMarks(void)
{
    int i;

    for (i=0;i<AM_NUMMARKPOINTS;i++)
	markpoints[i].setXX(-1); // means empty
    markpointnum = 0;
}

//
// should be called at the start of every level
// right now, i figure it out myself
//
void AM_LevelInit(void)
{
    leveljuststarted = 0;

    AM_clearMarks();

    AM_findMinMaxBoundaries();
    sscale_mtof = mmin_scale_mtof / 0.7;
    if (sscale_mtof > mmax_scale_mtof)
	sscale_mtof = mmin_scale_mtof;
    sscale_ftom = 1 / sscale_mtof;
}

//
//
//
void AM_Stop (void)
{
    AM_unloadPics();
    automapactive = false;
    stopped = true;
}

ImageScaler&
AM_GetImageScaler()
{
    return *mapView; 
}

//
//
//
void AM_Start (void)
{
    static int lastlevel = -1, lastepisode = -1;

    if (!stopped) AM_Stop();
    stopped = false;
    if (lastlevel != gamemap || lastepisode != gameepisode)
    {
	AM_LevelInit();
	lastlevel = gamemap;
	lastepisode = gameepisode;
    }
    AM_initVariables();
    AM_loadPics();
}

//
// set the window scale to the maximum size
//
void AM_minOutWindowScale(void)
{
    sscale_mtof = mmin_scale_mtof;
    sscale_ftom = 1 / sscale_mtof;
    AM_activateNewScale();
}

//
// set the window scale to the minimum size
//
void AM_maxOutWindowScale(void)
{
    sscale_mtof = mmax_scale_mtof;
    sscale_ftom = 1 / sscale_mtof;
    AM_activateNewScale();
}


//
// Handle events (user inputs) in automap mode
//
bool
AM_Responder
( event_t*	ev )
{

    int rc;
    static int bigstate=0;
    static char buffer[20];

    rc = false;

    if (!automapactive)
    {
	if (ev->type == ev_keydown && ev->data1 == AM_STARTKEY)
	{
	    AM_Start ();
	    viewactive = false;
	    rc = true;
	}
    }

    else if (ev->type == ev_keydown)
    {

	rc = true;
	switch(ev->data1)
	{
	  case AM_PANRIGHTKEY: // pan right
	      if (!followplayer) m_paninc.setXX(FFTOM(F_PANINC));
	    else rc = false;
	    break;
	  case AM_PANLEFTKEY: // pan left
	      if (!followplayer) m_paninc.setXX(-FFTOM(F_PANINC));
	    else rc = false;
	    break;
	  case AM_PANUPKEY: // pan up
	      if (!followplayer) m_paninc.setYY(FFTOM(F_PANINC));
	    else rc = false;
	    break;
	  case AM_PANDOWNKEY: // pan down
	      if (!followplayer) m_paninc.setYY(-FFTOM(F_PANINC));
	    else rc = false;
	    break;
	  case AM_ZOOMOUTKEY: // zoom out
	    mmtof_zoommul = MM_ZOOMOUT;
	    fftom_zoommul = MM_ZOOMIN;
	    break;
	  case AM_ZOOMINKEY: // zoom in
	    mmtof_zoommul = MM_ZOOMIN;
	    fftom_zoommul = MM_ZOOMOUT;
	    break;
	  case AM_ENDKEY:
	    bigstate = 0;
	    viewactive = true;
	    AM_Stop ();
	    break;
	  case AM_GOBIGKEY:
	    bigstate = !bigstate;
	    if (bigstate)
	    {
		AM_saveScaleAndLoc();
		AM_minOutWindowScale();
	    }
	    else AM_restoreScaleAndLoc();
	    break;
	  case AM_FOLLOWKEY:
	    followplayer = !followplayer;
	    f_oldloc.setXX(std::numeric_limits<double>::max());
	    plr->message = followplayer ? AMSTR_FOLLOWON : AMSTR_FOLLOWOFF;
	    break;
	  case AM_GRIDKEY:
	    grid = !grid;
	    plr->message = grid ? AMSTR_GRIDON : AMSTR_GRIDOFF;
	    break;
	  case AM_MARKKEY:
	    sprintf(buffer, "%s %d", AMSTR_MARKEDSPOT, markpointnum);
	    plr->message = buffer;
	    AM_addMark();
	    break;
	  case AM_CLEARMARKKEY:
	    AM_clearMarks();
	    plr->message = AMSTR_MARKSCLEARED;
	    break;
	  default:
	    rc = false;
	}
	if (!deathmatch && cht_CheckCheat(&cheat_amap, ev->data1))
	{
	    rc = false;
	    cheating = (cheating+1) % 3;
	}
    }

    else if (ev->type == ev_keyup)
    {
	rc = false;
	switch (ev->data1)
	{
	  case AM_PANRIGHTKEY:
	      if (!followplayer) m_paninc.setXX(0);
	    break;
	  case AM_PANLEFTKEY:
	      if (!followplayer) m_paninc.setXX(0);
	    break;
	  case AM_PANUPKEY:
	      if (!followplayer) m_paninc.setYY(0);
	    break;
	  case AM_PANDOWNKEY:
	      if (!followplayer) m_paninc.setYY(0);
	    break;
	  case AM_ZOOMOUTKEY:
	  case AM_ZOOMINKEY:
	    mmtof_zoommul = 1;
	    fftom_zoommul = 1;
	    break;
	}
    }

    return rc;

}


//
// Zooming
//
void AM_changeWindowScale(void)
{

    // Change the scaling multipliers
    sscale_mtof = sscale_mtof * mmtof_zoommul;
    sscale_ftom = 1 / sscale_mtof;

    if (sscale_mtof < mmin_scale_mtof)
	AM_minOutWindowScale();
    else if (sscale_mtof > mmax_scale_mtof)
	AM_maxOutWindowScale();
    else
	AM_activateNewScale();
}


//
//
//
void AM_doFollowPlayer(void)
{

    if (f_oldloc.getXX() != plr->mo->position.getX() ||
	f_oldloc.getYY() != plr->mo->position.getY())
    {

	mm_x = FFTOM(MMTOF(plr->mo->position.getX())) - mm_w/2;
	mm_y = FFTOM(MMTOF(plr->mo->position.getY())) - mm_h/2;
	mm_x2 = mm_x + mm_w;
	mm_y2 = mm_y + mm_h;
	f_oldloc = MPoint(plr->mo->position.getX(),
			  plr->mo->position.getY());

	//  m_x = FTOM(MTOF(plr->mo->x - m_w/2));
	//  m_y = FTOM(MTOF(plr->mo->y - m_h/2));
	//  m_x = plr->mo->x - m_w/2;
	//  m_y = plr->mo->y - m_h/2;

    }

}

//
//
//
void AM_updateLightLev(void)
{
    static int nexttic = 0;
    //static int litelevels[] = { 0, 3, 5, 6, 6, 7, 7, 7 };
    static int litelevels[] = { 0, 4, 7, 10, 12, 14, 15, 15 };
    static int litelevelscnt = 0;
   
    // Change light level
    if (amclock>nexttic)
    {
	lightlev = litelevels[litelevelscnt++];
	if (litelevelscnt == sizeof(litelevels)/sizeof(int)) litelevelscnt = 0;
	nexttic = amclock + 6 - (amclock % 6);
    }

}


//
// Updates on Game Tick
//
void AM_Ticker (void)
{

    if (!automapactive)
	return;

    amclock++;

    if (followplayer)
	AM_doFollowPlayer();

    // Change the zoom if necessary
    if (fftom_zoommul != 1)
	AM_changeWindowScale();

    // Change x,y location
    if (m_paninc.getXX() != 0.0 || m_paninc.getYY() != 0.0)
	AM_changeWindowLoc();

    // Update light level
    // AM_updateLightLev();

}

// Automap clipping of lines.
//
// Based on Cohen-Sutherland clipping algorithm but with a slightly
// faster reject and precalculated slopes.  If the speed is needed,
// use a hash algorithm to handle  the common cases.
//
bool
AM_clipMline
( MLine*	ml,
  fline_t*	fl )
{
    enum
    {
	LEFT	=1,
	RIGHT	=2,
	BOTTOM	=4,
	TOP	=8
    };
    
    register	int outcode1 = 0;
    register	int outcode2 = 0;
    register	int outside;
    
    fpoint_t	tmp;
    int		dx;
    int		dy;

    
#define DOOUTCODE(oc, mx, my) \
    (oc) = 0; \
    if ((my) < 0) (oc) |= TOP; \
    else if ((my) >= BASE_HEIGHT) (oc) |= BOTTOM; \
    if ((mx) < 0) (oc) |= LEFT; \
    else if ((mx) >= BASE_WIDTH) (oc) |= RIGHT;

    
    // do trivial rejects and outcodes
    if (ml->_a.getYY() > mm_y2)
	outcode1 = TOP;
    else if (ml->_a.getYY() < mm_y)
	outcode1 = BOTTOM;

    if (ml->_b.getYY() > mm_y2)
	outcode2 = TOP;
    else if (ml->_b.getYY() < mm_y)
	outcode2 = BOTTOM;
    
    if (outcode1 & outcode2)
	return false; // trivially outside

    if (ml->_a.getXX() < mm_x)
	outcode1 |= LEFT;
    else if (ml->_a.getXX() > mm_x2)
	outcode1 |= RIGHT;
    
    if (ml->_b.getXX() < mm_x)
	outcode2 |= LEFT;
    else if (ml->_b.getXX() > mm_x2)
	outcode2 |= RIGHT;
    
    if (outcode1 & outcode2)
	return false; // trivially outside

    // transform to frame-buffer coordinates.
    fl->a.x = CCXMTOF(ml->_a.getXX());
    fl->a.y = CCYMTOF(ml->_a.getYY());
    fl->b.x = CCXMTOF(ml->_b.getXX());
    fl->b.y = CCYMTOF(ml->_b.getYY());

    DOOUTCODE(outcode1, fl->a.x, fl->a.y);
    DOOUTCODE(outcode2, fl->b.x, fl->b.y);

    if (outcode1 & outcode2)
	return false;

    while (outcode1 | outcode2)
    {
	// may be partially inside box
	// find an outside point
	if (outcode1)
	    outside = outcode1;
	else
	    outside = outcode2;
	
	// clip to each side
	if (outside & TOP)
	{
	    dy = fl->a.y - fl->b.y;
	    dx = fl->b.x - fl->a.x;
	    tmp.x = fl->a.x + (dx*(fl->a.y))/dy;
	    tmp.y = 0;
	}
	else if (outside & BOTTOM)
	{
	    dy = fl->a.y - fl->b.y;
	    dx = fl->b.x - fl->a.x;
	    tmp.x = fl->a.x + (dx*(fl->a.y - BASE_HEIGHT))/dy;
	    tmp.y = BASE_HEIGHT - 1;
	}
	else if (outside & RIGHT)
	{
	    dy = fl->b.y - fl->a.y;
	    dx = fl->b.x - fl->a.x;
	    tmp.y = fl->a.y + (dy*(BASE_WIDTH - 1 - fl->a.x))/dx;
	    tmp.x = BASE_WIDTH - 1;
	}
	else if (outside & LEFT)
	{
	    dy = fl->b.y - fl->a.y;
	    dx = fl->b.x - fl->a.x;
	    tmp.y = fl->a.y + (dy*(-fl->a.x))/dx;
	    tmp.x = 0;
	}

	if (outside == outcode1)
	{
	    fl->a = tmp;
	    DOOUTCODE(outcode1, fl->a.x, fl->a.y);
	}
	else
	{
	    fl->b = tmp;
	    DOOUTCODE(outcode2, fl->b.x, fl->b.y);
	}
	
	if (outcode1 & outcode2)
	    return false; // trivially outside
    }

    return true;
}
#undef DOOUTCODE


//
// Classic Bresenham w/ whatever optimizations needed for speed
//
void
AM_drawFline
( fline_t*	fl,
  int		color )
{
    register int x;
    register int y;
    register int dx;
    register int dy;
    register int sx;
    register int sy;
    register int ax;
    register int ay;
    register int d;
    
    static int fuck = 0;

    // For debugging only
    if (      fl->a.x < 0 || fl->a.x >= BASE_WIDTH
	   || fl->a.y < 0 || fl->a.y >= BASE_HEIGHT
	   || fl->b.x < 0 || fl->b.x >= BASE_WIDTH
	   || fl->b.y < 0 || fl->b.y >= BASE_HEIGHT)
    {
	fprintf(stderr, "fuck %d \r", fuck++);
	return;
    }

#define PUTDOT(xx,yy,cc) mapView->draw(xx, yy,cc)

    dx = fl->b.x - fl->a.x;
    ax = 2 * (dx<0 ? -dx : dx);
    sx = dx<0 ? -1 : 1;

    dy = fl->b.y - fl->a.y;
    ay = 2 * (dy<0 ? -dy : dy);
    sy = dy<0 ? -1 : 1;

    x = fl->a.x;
    y = fl->a.y;

    if (ax > ay)
    {
	d = ay - ax/2;
	while (1)
	{
	    PUTDOT(x,y,color);
	    if (x == fl->b.x) return;
	    if (d>=0)
	    {
		y += sy;
		d -= ax;
	    }
	    x += sx;
	    d += ay;
	}
    }
    else
    {
	d = ax - ay/2;
	while (1)
	{
	    PUTDOT(x, y, color);
	    if (y == fl->b.y) return;
	    if (d >= 0)
	    {
		x += sx;
		d -= ay;
	    }
	    y += sy;
	    d += ax;
	}
    }
}


//
// Clip lines, draw visible part sof lines.
//
void
AM_drawMline
( MLine*	ml,
  int		color )
{
    static fline_t fl;

    if (AM_clipMline(ml, &fl))
	AM_drawFline(&fl, color); // draws it on frame buffer using fb coords
}



//
// Draws flat (floor/ceiling tile) aligned grid lines.
//
void AM_drawGrid(int color)
{
    MLine ml;

    // Figure out start of vertical gridlines
    double start = mm_x;
    if (fmod(start - blockMap.oorgx, MAPBLOCKUNITS))
	start += MAPBLOCKUNITS - fmod(start- blockMap.oorgx, MAPBLOCKUNITS);
    double end = mm_x + mm_w;

    // draw vertical gridlines
    ml._a.setYY(mm_y);
    ml._b.setYY(mm_y + mm_h);
    for (double x = start; x < end; x+= MAPBLOCKUNITS)
    {
	ml._a.setXX(x);
	ml._b.setXX(x);
	AM_drawMline(&ml, color);
    }

    // Figure out start of horizontal gridlines
    start = mm_y;
    if (fmod(start - blockMap.oorgy, MAPBLOCKUNITS))
	start += MAPBLOCKUNITS - fmod(start - blockMap.oorgy, MAPBLOCKUNITS);
    end = mm_y + mm_h;

    // draw horizontal gridlines
    ml._a.setXX(mm_x);
    ml._b.setXX(mm_x + mm_w);
    for (double y = start; y < end; y += MAPBLOCKUNITS)
    {
	ml._a.setYY(y);
	ml._b.setYY(y);
	AM_drawMline(&ml, color);
    }
}

//
// Determines visible lines, draws them.
// This is LineDef based, not LineSeg based.
//
void AM_drawWalls(void)
{
    int i;
    static MLine l;

    for (i=0;i<numlines;i++)
    {
	l._a.setXX(lines[i].v1->getX());
	l._a.setYY(lines[i].v1->getY());
	l._b.setXX(lines[i].v2->getX());
	l._b.setYY(lines[i].v2->getY());
	if (cheating || (lines[i].flags & ML_MAPPED))
	{
	    if ((lines[i].flags & LINE_NEVERSEE) && !cheating)
		continue;
	    if (!lines[i].backsector)
	    {
		AM_drawMline(&l, WALLCOLORS+lightlev);
	    }
	    else
	    {
		if (lines[i].special == 39)
		{ // teleporters
		    AM_drawMline(&l, WALLCOLORS+WALLRANGE/2);
		}
		else if (lines[i].flags & ML_SECRET) // secret door
		{
		    if (cheating) AM_drawMline(&l, SECRETWALLCOLORS + lightlev);
		    else AM_drawMline(&l, WALLCOLORS+lightlev);
		}
		else if (lines[i].backsector->ffloorheight
			   != lines[i].frontsector->ffloorheight) {
		    AM_drawMline(&l, FDWALLCOLORS + lightlev); // floor level change
		}
		else if (lines[i].backsector->cceilingheight
			   != lines[i].frontsector->cceilingheight) {
		    AM_drawMline(&l, CDWALLCOLORS+lightlev); // ceiling level change
		}
		else if (cheating) {
		    AM_drawMline(&l, TSWALLCOLORS+lightlev);
		}
	    }
	}
	else if (plr->powers[pw_allmap])
	{
	    if (!(lines[i].flags & LINE_NEVERSEE)) AM_drawMline(&l, GRAYS+3);
	}
    }
}

void
AM_drawLineCharacter
( MLine*	lineguy,
  int		lineguylines,
  double	scale,
  Angle	        angle,
  int		color,
  double	x,
  double	y )
{
    MLine	l;

    for (int i = 0; i < lineguylines; i++)
    {
	l._a = lineguy[i]._a;

	if (scale != 0)
	    l._a.scale(scale);

	if (angle != Angle())
            l._a.rotate(angle);

	l._a.incX(x);
	l._a.incY(y);

	l._b = lineguy[i]._b;

	if (scale != 0)
	    l._b.scale(scale);;
	
	if (angle != Angle())
	    l._b.rotate(angle);
	
	l._b.incX(x);
	l._b.incY(y);

	AM_drawMline(&l, color);
    }
}

void AM_drawPlayers(void)
{
    int		i;
    player_t*	p;
    static int 	their_colors[] = { GREENS, GRAYS, BROWNS, REDS };
    int		their_color = -1;
    int		color;

    if (!netgame)
    {
	if (cheating)
	    AM_drawLineCharacter
		(cheat_player_arrow, NUMCHEATPLYRLINES, 0,
		 plr->mo->_angle, WHITE,
		 plr->mo->position.getX(),
		 plr->mo->position.getY());
	else
	    AM_drawLineCharacter
		(player_arrow, NUMPLYRLINES, 0, plr->mo->_angle,
		 WHITE,
		 plr->mo->position.getX(),
		 plr->mo->position.getY());
	return;
    }

    for (i=0;i<MAXPLAYERS;i++)
    {
	their_color++;
	p = &players[i];

	if ( (deathmatch && !singledemo) && p != plr)
	    continue;

	if (!playeringame[i])
	    continue;

	if (p->powers[pw_invisibility])
	    color = 246; // *close* to black
	else
	    color = their_colors[their_color];
	
	AM_drawLineCharacter
	    (player_arrow, NUMPLYRLINES, 0, p->mo->_angle,
	     color,
	     plr->mo->position.getX(),
	     plr->mo->position.getY());
    }

}

void
AM_drawThings
( int	colors,
  int 	colorrange)
{
    int		i;
    Mob*	t;

    for (i=0;i<numsectors;i++)
    {
	t = sectors[i].thinglist;
	while (t)
	{
	    AM_drawLineCharacter
		(thintriangle_guy, NUMTHINTRIANGLEGUYLINES,
		 16, t->_angle, colors+lightlev,
		 t->position.getX(),
		 t->position.getY());
	    t = t->snext;
	}
    }
}

void AM_drawMarks(void)
{
    int i, fx, fy, w, h;

    for (i=0;i<AM_NUMMARKPOINTS;i++)
    {
	if (markpoints[i].getXX() != -1)
	{
	    w = SHORT(marknums[i]->width);
	    h = SHORT(marknums[i]->height);
	    fx = CCXMTOF(markpoints[i].getXX());
	    fy = CCYMTOF(markpoints[i].getYY());
	    if (fx >= 0 && fx < BASE_WIDTH - w && fy >= 0 && fy < BASE_HEIGHT - h)
		mapView->drawPatch(fx, fy, marknums[i]);
	}
    }

}

void AM_drawCrosshair(int color)
{
    //@todo drawing a cross hair at the start point cant be hard 
    //fb[(f_w*(f_h+1))/2] = color; // single point for now

}

void AM_Drawer (void)
{
    if (!automapactive) return;

    if (grid)
	AM_drawGrid(GRIDCOLORS);
    AM_drawWalls();
    AM_drawPlayers();
    if (cheating == 2)
	AM_drawThings(THINGCOLORS, THINGRANGE);
    AM_drawCrosshair(XHAIRCOLORS);

    AM_drawMarks();
    // Put the map upper left corner
    mapView->display(SCREENWIDTH - BASE_WIDTH * 2.0, 0, 2.0);
    mapView->reset();
}
