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
//	Status bar code.
//	Does the face/direction indicator animatin.
//	Does palette indicators as well (red pain/berserk, bright pickup)
//
//-----------------------------------------------------------------------------

#ifndef __STSTUFF_H__
#define __STSTUFF_H__

#include "doomtype.h"
#include "d_event.h"

// Size of statusbar.
// Now sensitive for scaling.
#define ST_HEIGHT	32
#define ST_WIDTH        320

// Location of status bar
#define ST_Y		(SCREENHEIGHT - 2 * ST_HEIGHT)
#define ST_X		(SCREENWIDTH / 2) - ST_WIDTH 


//
// STATUS BAR
//

// Called by main loop.
bool ST_Responder (event_t* ev);

// Called by main loop.
void ST_Ticker (void);

// Called by main loop.
void ST_Drawer();

// Called when the console player is spawned on each level.
void ST_Start (void);

// Called by startup code.
void ST_Init (void);



// States for status bar code.
typedef enum
{
    AutomapState,
    FirstPersonState
    
} st_stateenum_t;


// States for the chat code.
typedef enum
{
    StartChatState,
    WaitDestState,
    GetChatState
    
} st_chatstateenum_t;


#endif
//-----------------------------------------------------------------------------
//
// $Log:$
//
//-----------------------------------------------------------------------------