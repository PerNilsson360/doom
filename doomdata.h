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
//  all external data is defined here
//  most of the data is loaded into different structures at run time
//  some internal structures shared by many modules are here
//
//-----------------------------------------------------------------------------

#ifndef __DOOMDATA__
#define __DOOMDATA__

// The most basic types we use, portability.
#include "doomtype.h"

// Some global defines, that configure the game.
#include "doomdef.h"



//
// Map level types.
// The following data structures define the persistent format
// used in the lumps of the WAD files.
//

// Lump order in a map WAD: each map needs a couple of lumps
// to provide a complete scene geometry description.
enum
{
  ML_LABEL,		// A separator, name, ExMx or MAPxx
  ML_THINGS,		// Monsters, items..
  ML_LINEDEFS,		// LineDefs, from editing
  ML_SIDEDEFS,		// SideDefs, from editing
  ML_VERTEXES,		// Vertices, edited and BSP splits generated
  ML_SEGS,		// LineSegs, from LineDefs split by BSP
  ML_SSECTORS,		// SubSectors, list of LineSegs
  ML_NODES,		// BSP nodes
  ML_SECTORS,		// Sectors, from editing
  ML_REJECT,		// LUT, sector-sector visibility	
  ML_BLOCKMAP		// LUT, motion clipping, walls/grid element
};

#endif			// __DOOMDATA__
//-----------------------------------------------------------------------------
//
// $Log:$
//
//-----------------------------------------------------------------------------

