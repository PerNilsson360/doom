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
//	Do all the WAD I/O, get map description,
//	set up initial state and misc. LUTs.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: p_setup.c,v 1.5 1997/02/03 22:45:12 b1 Exp $";


#include <math.h>
#include <iostream>
#include <vector>

#include "BlockMap.hh"
#include "DataInput.hh"
#include "Sector.hh"

#include "z_zone.h"

#include "m_swap.h"
#include "m_bbox.h"

#include "g_game.h"

#include "i_system.h"
#include "w_wad.h"

#include "doomdef.h"
#include "p_local.h"

#include "s_sound.h"

#include "doomstat.h"


void	P_SpawnMapThing (MapThing*	mthing);


//
// MAP related Lookup tables.
// Store VERTEXES, LINEDEFS, SIDEDEFS, etc.
//
std::vector<Vertex> vertexes;

int		numsegs;
seg_t*		segs;

std::vector<Sector> sectors;

int		numsubsectors;
subsector_t*	subsectors;

std::vector<BspNode> nodes;

int		numlines;
line_t*		lines;

int		numsides;
side_t*		sides;


// BLOCKMAP
// Created from axis aligned bounding box
// of the map, a rectangular array of
// blocks of size ...
// Used to speed up collision detection
// by spatial subdivision in 2D.
BlockMap blockMap;


// REJECT
// For fast sight rejection.
// Speeds up enemy AI by skipping detailed
//  LineOf Sight calculation.
// Without special effect, this could be
//  used as a PVS lookup as well.
//
byte*		rejectmatrix;


// Maintain single and multi player starting spots.
#define MAX_DEATHMATCH_STARTS	10

MapThing	deathmatchstarts[MAX_DEATHMATCH_STARTS];
MapThing*	deathmatch_p;
MapThing	playerstarts[MAXPLAYERS];





//
// P_LoadVertexes
//
void P_LoadVertexes (int lump)
{
    vertexes.clear();
    int dataLength = W_LumpLength(lump);
    int numverticies = dataLength / Vertex::getBinarySize();
    byte* data = (byte*)W_CacheLumpNum(lump, PU_STATIC);
    DataInput dataInput(data, dataLength);
    for (int i = 0 ;i < numverticies; i++) {
	vertexes.push_back(Vertex(dataInput));
    }
    Z_Free (data);
}



//
// P_LoadSegs
//
void P_LoadSegs (int lump)
{
    byte*		data;
    int			i;
    mapseg_t*		ml;
    seg_t*		li;
    line_t*		ldef;
    int			linedef;
    int			side;
	
    numsegs = W_LumpLength (lump) / sizeof(mapseg_t);
    segs = (seg_t*)Z_Malloc (numsegs*sizeof(seg_t),PU_LEVEL,0);	
    memset (segs, 0, numsegs*sizeof(seg_t));
    data = (byte*)W_CacheLumpNum (lump,PU_STATIC);
	
    ml = (mapseg_t *)data;
    li = segs;
    for (i=0 ; i<numsegs ; i++, li++, ml++)
    {
	li->v1 = &vertexes[SHORT(ml->v1)];
	li->v2 = &vertexes[SHORT(ml->v2)];
					
	li->aangle = Angle(SHORT(ml->angle) * Angle::A360 / 65536);
	li->ooffset = (SHORT(ml->offset));
	linedef = SHORT(ml->linedef);
	ldef = &lines[linedef];
	li->linedef = ldef;
	side = SHORT(ml->side);
	li->sidedef = &sides[ldef->sidenum[side]];
	li->frontsector = sides[ldef->sidenum[side]].sector;
	if (ldef-> flags & ML_TWOSIDED)
	    li->backsector = sides[ldef->sidenum[side^1]].sector;
	else
	    li->backsector = 0;
    }
	
    Z_Free (data);
}


//
// P_LoadSubsectors
//
void P_LoadSubsectors (int lump)
{
    byte*		data;
    int			i;
    mapsubsector_t*	ms;
    subsector_t*	ss;
	
    numsubsectors = W_LumpLength (lump) / sizeof(mapsubsector_t);
    subsectors = (subsector_t*)Z_Malloc (numsubsectors*sizeof(subsector_t),PU_LEVEL,0);	
    data = (byte*)W_CacheLumpNum (lump,PU_STATIC);
	
    ms = (mapsubsector_t *)data;
    memset (subsectors,0, numsubsectors*sizeof(subsector_t));
    ss = subsectors;
    
    for (i=0 ; i<numsubsectors ; i++, ss++, ms++)
    {
	ss->numlines = SHORT(ms->numsegs);
	ss->firstline = SHORT(ms->firstseg);
    }
	
    Z_Free (data);
}



//
// P_LoadSectors
//
void P_LoadSectors (int lump)
{
    sectors.clear();
    int dataLength = W_LumpLength(lump);
    int nSectors = dataLength / Sector::getBinarySize();
    byte* data = (byte*)W_CacheLumpNum(lump, PU_STATIC);
    DataInput dataInput(data, dataLength);
    for (int i = 0; i < nSectors; i++) {
	sectors.push_back(Sector(dataInput));
    }
    Z_Free (data);
}


//
// P_LoadNodes
//
void P_LoadNodes (int lump)
{
    nodes.clear();
    int dataLength = W_LumpLength(lump);
    int numnodes = dataLength / BspNode::getBinarySize();
    byte* data = (byte*)W_CacheLumpNum(lump, PU_STATIC);
    DataInput dataInput(data, dataLength);
    for (int i = 0; i < numnodes; i++) {
	nodes.push_back(BspNode(dataInput));
    }
    Z_Free (data);
}


//
// P_LoadThings
//
void P_LoadThings (int lump)
{
    byte*		data;
    int			i;
    MapThing*		mt;
    int			numthings;
	
    data = (byte*)W_CacheLumpNum (lump,PU_STATIC);
    numthings = W_LumpLength (lump) / sizeof(MapThing);
	
    mt = (MapThing *)data;
    for (i=0 ; i<numthings ; i++, mt++) {
	mt->x = SHORT(mt->x);
	mt->y = SHORT(mt->y);
	mt->angle = SHORT(mt->angle);
	mt->type = SHORT(mt->type);
	mt->options = SHORT(mt->options);
	
	P_SpawnMapThing (mt);
    }
	
    Z_Free (data);
}


//
// P_LoadLineDefs
// Also counts secret lines for intermissions.
//
void P_LoadLineDefs (int lump)
{
    byte*		data;
    int			i;
    maplinedef_t*	mld;
    line_t*		ld;
    Vertex*		v1;
    Vertex*		v2;
	
    numlines = W_LumpLength (lump) / sizeof(maplinedef_t);
    lines = (line_t*)Z_Malloc (numlines*sizeof(line_t),PU_LEVEL,0);	
    memset (lines, 0, numlines*sizeof(line_t));
    data = (byte*)W_CacheLumpNum (lump,PU_STATIC);
	
    mld = (maplinedef_t *)data;
    ld = lines;
    for (i=0 ; i<numlines ; i++, mld++, ld++)
    {
	ld->flags = SHORT(mld->flags);
	ld->special = SHORT(mld->special);
	ld->tag = SHORT(mld->tag);
	v1 = ld->v1 = &vertexes[SHORT(mld->v1)];
	v2 = ld->v2 = &vertexes[SHORT(mld->v2)];
	ld->ddx
	    = v2->getX() - v1->getX();
	ld->ddy = v2->getY() - v1->getY();
	
	if (ld->ddx == 0)
	    ld->slopetype = ST_VERTICAL;
	else if (ld->ddy == 0)
	    ld->slopetype = ST_HORIZONTAL;
	else
	{
	    if (ld->ddy / ld->ddx > 0)
            ld->slopetype = ST_POSITIVE;
	    else
            ld->slopetype = ST_NEGATIVE;
	}
		
	if (v1->getX() < v2->getX())
	{
	    ld->bbbox[BOXLEFT] = v1->getX();
	    ld->bbbox[BOXRIGHT] = v2->getX();
	}
	else
	{
	    ld->bbbox[BOXLEFT] = v2->getX();
	    ld->bbbox[BOXRIGHT] = v1->getX();
	}

	if (v1->getY() < v2->getY())
	{
	    ld->bbbox[BOXBOTTOM] = v1->getY();
	    ld->bbbox[BOXTOP] = v2->getY();
	}
	else
	{
	    ld->bbbox[BOXBOTTOM] = v2->getY();
	    ld->bbbox[BOXTOP] = v1->getY();
	}

	ld->sidenum[0] = SHORT(mld->sidenum[0]);
	ld->sidenum[1] = SHORT(mld->sidenum[1]);

	if (ld->sidenum[0] != -1)
	    ld->frontsector = sides[ld->sidenum[0]].sector;
	else
	    ld->frontsector = 0;

	if (ld->sidenum[1] != -1)
	    ld->backsector = sides[ld->sidenum[1]].sector;
	else
	    ld->backsector = 0;
    }
	
    Z_Free (data);
}


//
// P_LoadSideDefs
//
void P_LoadSideDefs (int lump)
{
    byte*		data;
    int			i;
    mapsidedef_t*	msd;
    side_t*		sd;
	
    numsides = W_LumpLength (lump) / sizeof(mapsidedef_t);
    sides = (side_t*)Z_Malloc (numsides*sizeof(side_t),PU_LEVEL,0);	
    memset (sides, 0, numsides*sizeof(side_t));
    data = (byte*)W_CacheLumpNum (lump,PU_STATIC);
	
    msd = (mapsidedef_t *)data;
    sd = sides;
    for (i=0 ; i<numsides ; i++, msd++, sd++)
    {
	sd->ttextureoffset = SHORT(msd->textureoffset);
	sd->rrowoffset = SHORT(msd->rowoffset);
	sd->toptexture = R_TextureNumForName(msd->toptexture);
	sd->bottomtexture = R_TextureNumForName(msd->bottomtexture);
	sd->midtexture = R_TextureNumForName(msd->midtexture);
	sd->sector = &sectors[SHORT(msd->sector)];
    }
	
    Z_Free (data);
}

//
// P_GroupLines
// Builds sector line lists and subsector sector numbers.
// Finds block bounding boxes for sectors.
//
void P_GroupLines (void)
{
    line_t**		linebuffer;
    int			i;
    int			j;
    int			total;
    line_t*		li;
    subsector_t*	ss;
    seg_t*		seg;
    double		bbbox[4];
    int			block;
	
    // look up sector number for each subsector
    ss = subsectors;
    for (i=0 ; i<numsubsectors ; i++, ss++)
    {
	seg = &segs[ss->firstline];
	ss->sector = seg->sidedef->sector;
    }

    // count number of lines in each sector
    li = lines;
    total = 0;
    for (i=0 ; i<numlines ; i++, li++)
    {
	total++;
	li->frontsector->linecount++;

	if (li->backsector && li->backsector != li->frontsector)
	{
	    li->backsector->linecount++;
	    total++;
	}
    }
	
    // build line tables for each sector	
    linebuffer = (line_t**)Z_Malloc (total*8, PU_LEVEL, 0);
    for (int i = 0, len = sectors.size(); i < len; i++)
    {
	Sector* sector = &sectors[i];
	MM_ClearBox (bbbox);
	sector->lines = linebuffer;
	li = lines;
	for (j=0 ; j<numlines ; j++, li++)
	{
	    if (li->frontsector == sector || li->backsector == sector)
	    {
		*linebuffer++ = li;
		MM_AddToBox (bbbox, li->v1->getX(), li->v1->getY());
		MM_AddToBox (bbbox, li->v2->getX(), li->v2->getY());
	    }
	}
	/*
	if (linebuffer - sector->lines != sector->linecount)
	    I_Error ("P_GroupLines: miscounted");
	*/		
	// set the degenmobj_t to the middle of the bounding box
	sector->soundorg.xx = (bbbox[BOXRIGHT]+bbbox[BOXLEFT])/2;
	sector->soundorg.yy = (bbbox[BOXTOP]+bbbox[BOXBOTTOM])/2;
		
	// adjust bounding box to map blocks
	block = (bbbox[BOXTOP]-blockMap.oorgy+MMAXRADIUS) / DOUBLE_MAPBLOCKS_DIV;
	block = block >= blockMap.height ? blockMap.height-1 : block;
	sector->blockbox[BOXTOP]=block;

	block = (bbbox[BOXBOTTOM]-blockMap.oorgy-MMAXRADIUS) / DOUBLE_MAPBLOCKS_DIV;
	block = block < 0 ? 0 : block;
	sector->blockbox[BOXBOTTOM]=block;

	block = (bbbox[BOXRIGHT]-blockMap.oorgx+MMAXRADIUS) / DOUBLE_MAPBLOCKS_DIV;
	block = block >= blockMap.width ? blockMap.width-1 : block;
	sector->blockbox[BOXRIGHT]=block;

	block = (bbbox[BOXLEFT]-blockMap.oorgx-MMAXRADIUS) / DOUBLE_MAPBLOCKS_DIV;
	block = block < 0 ? 0 : block;
	sector->blockbox[BOXLEFT]=block;
    }
	
}


//
// P_SetupLevel
//
void
P_SetupLevel
( int		episode,
  int		map,
  int		playermask,
  skill_t	skill)
{
    int		i;
    char	lumpname[9];
    int		lumpnum;
	
    totalkills = totalitems = totalsecret = wminfo.maxfrags = 0;
    wminfo.partime = 180;
    for (i=0 ; i<MAXPLAYERS ; i++)
    {
	players[i].killcount = players[i].secretcount 
	    = players[i].itemcount = 0;
    }

    // Initial height of PointOfView
    // will be set by player think.
    players[consoleplayer].vviewz = 1; 

    // Make sure all sounds are stopped before Z_FreeTags.
    S_Start ();			

    
#if 0 // UNUSED
    if (debugfile)
    {
	Z_FreeTags (PU_LEVEL, MAXINT);
	Z_FileDumpHeap (debugfile);
    }
    else
#endif
	Z_FreeTags (PU_LEVEL, PU_PURGELEVEL-1);


    // unused W_Profile ();
    P_InitThinkers ();

    // if working with a devlopment map, reload it
    W_Reload ();			
	   
    // find map name
    if ( gamemode == commercial)
    {
	if (map<10)
	    sprintf (lumpname,"map0%i", map);
	else
	    sprintf (lumpname,"map%i", map);
    }
    else
    {
	lumpname[0] = 'E';
	lumpname[1] = '0' + episode;
	lumpname[2] = 'M';
	lumpname[3] = '0' + map;
	lumpname[4] = 0;
    }

    lumpnum = W_GetNumForName (lumpname);
	
    leveltime = 0;
	
    // note: most of this ordering is important	
    blockMap.load(lumpnum+ML_BLOCKMAP);
    P_LoadVertexes (lumpnum+ML_VERTEXES);
    P_LoadSectors (lumpnum+ML_SECTORS);
    P_LoadSideDefs (lumpnum+ML_SIDEDEFS);

    P_LoadLineDefs (lumpnum+ML_LINEDEFS);
    P_LoadSubsectors (lumpnum+ML_SSECTORS);
    P_LoadNodes (lumpnum+ML_NODES);
    P_LoadSegs (lumpnum+ML_SEGS);
	
    rejectmatrix = (byte*)W_CacheLumpNum (lumpnum+ML_REJECT,PU_LEVEL);
    P_GroupLines ();

    bodyqueslot = 0;
    deathmatch_p = deathmatchstarts;
    P_LoadThings (lumpnum+ML_THINGS);
    
    // if deathmatch, randomly spawn the active players
    if (deathmatch)
    {
	for (i=0 ; i<MAXPLAYERS ; i++)
	    if (playeringame[i])
	    {
		players[i].mo = NULL;
		G_DeathMatchSpawnPlayer (i);
	    }
			
    }

    // clear special respawning que
    iquehead = iquetail = 0;		
	
    // set up world state
    P_SpawnSpecials ();
	
    // build subsector connect matrix
    //	UNUSED P_ConnectSubsectors ();

    // preload graphics
    if (precache)
	R_PrecacheLevel ();

    //printf ("free memory: 0x%x\n", Z_FreeMemory());

}



//
// P_Init
//
void P_Init (void)
{
    P_InitSwitchList ();
    P_InitPicAnims ();
    R_InitSprites (sprnames);
}



