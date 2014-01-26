/***************************************************************************
 *   Copyright (C) 2004 by Veselin Georgiev                                *
 *   vesko@ViruS                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

//#define MAKE_PROFILING
#include "asmconfig.h"

#define MAX_PROF_ENTRIES 256


// how much milliseconds run the idle loop to calibrate the cpu speed.
#define CPU_CALIBRATE_TIME 500

/*
	Profile IDs are substructured this way:
		if Id is divisable by 16 it is for a grand task, which can consist of up to 15 subtasks, whose
		IDs are between the father's ID and the next grandtask ID.
*/
#ifdef MAKE_PROFILING
void prof_enter(unsigned ID);
void prof_leave(unsigned ID);
void prof_init(void);
#else
#define prof_enter(expr)
#define prof_leave(expr)
#define prof_init()
#endif

long long prof_rdtsc(void);


void prof_statistics(void);

int  prof_get_cpu_speed(void);
void mark_cpu_rdtsc(void);


// a list of IDs follow:

/* GRAND   ID's ::::: */
#define PROF_BASH_PREFRAME	0x20
#define PROF_PREFRAME_DO	0x30
#define PROF_RENDER_FLOOR	0x40
#define PROF_RENDER_SPHERES	0x50
#define PROF_RENDER_SHADOWS	0x60
#define PROF_RENDER_VOXEL	0x70
#define PROF_MERGE_BUFFERS	0x80
#define PROF_POSTFRAME_DO	0xA0

/* SUBTASK ID's ::::::::::*/


// PROF_POSTFRAME_DO
#define PROF_ANTIALIAS		0xA1
#define PROF_SHADER1		0xA2
#define PROF_SHADER2		0xA3
#define PROF_CHECKSUM		0xA4
#define PROF_BLUR_DO		0xA5
#define PROF_SHOWFPS		0xA6
#define PROF_MEMCPY		0xA7
#define PROF_SDL_FLIP		0xA8
#define PROF_CONVERTRGB2YUV	0xA9
#define PROF_DISPLAYOVERLAY	0xAa

// PROF_PREFRAME_DO
#define PROF_PREFRAME_MEMSET	0x31
#define PROF_SPHERE_THINGS	0x32
#define PROF_PHYSICS		0x33
#define PROF_SORT		0x34
#define PROF_SHADOWED_CHECK	0x35
#define PROF_PROJECT		0x36
#define PROF_YSORT		0x37
#define PROF_PASS		0x38
#define PROF_MESHINIT		0x39
#define PROF_ANTIBUF_INIT	0x3a

// PROF_BASH_PREFRAME
// nothing...

// PROF_RENDER_FLOOR
#define PROF_INIT_PER_ROW	0x41
#define PROF_WORK_PER_ROW	0x42

// PROF_RENDER_SPHERES
#define PROF_MEMSETS		0x51
#define PROF_SOLVE3D		0x52
#define PROF_RAYTRACE		0x53

// PROF_RENDER_SHADOWS
#define PROF_ZERO_SBUFFER	0x61
#define PROF_SHADOW_PRECALC	0x62
#define PROF_POLY_GEN		0x63
#define PROF_POLY_REARRANGE	0x64
#define PROF_CONNECT_GRAPH	0x65
#define PROF_MAKE_WEDGES	0x66
#define PROF_FRUSTRUM_CLIP	0x67
#define PROF_POLY_DISPLAY	0x6a
#define PROF_MERGE		0x6f

// PROF_RENDER_VOXEL
#define PROF_BUFFER_CLEAR	0x71
#define PROF_ADDRESS_GENERATE	0x73
#define PROF_INTERPOL_INIT	0x74
#define PROF_INTERPOLATE	0x75
#define PROF_STAGE1		0x76
#define PROF_STAGE2		0x77

// PROF_MERGE_BUFFERS
// none

//ATTIC
#define PROF_SHADOWIZE		0
#define PROF_CANDIDATESEARCH	0
#define PROF_TRIANGLEFILL	0
#define PROF_TRIANGLEMERGE	0
