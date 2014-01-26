/***************************************************************************
 *   Copyright (C) 2004 by Veselin Georgiev                                *
 *   vesko@ViruS                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "MyTypes.h"
#include "vector3.h"
#include "hierarchy.h"
#include "threads.h"
#include "object.h"

#define NUM_VOXELS 2
//#define NUM_VOXELS 1
#define MAX_VOXEL_DIST 4000
#define __4 3

#define HEIGHTFIELD_RAYTRACE_MIPLEVEL 3

// should be power of two; increasing it makes the shadowcasting faster,
// but may expose some annoying artifacts
#define SHADOW_CAST_ACCURACY 4

#define LIGHT_PRECALC_ACCURACY 1.4142

//#define LIGHT_PRECALC_ACCURACY 1.0

#define PREC_MAX_DIST	1416
#define RESOLUTION	10

#define FLOOR_LOW -9999.0
#define CEIL_HIGH +9999.0

#define IS_NEGATIVE(x) ((((unsigned)(x))&0x80000000))

// pi / 4
#define FOURTY_FIVE_DEGREES 0.78539816339745
#define decompose(a,b) b[0]=(a&0xff)<<16,b[1]=((a&0xff00)<<8),b[2]=(a&0xff0000)
#define init_color_interpolation(dst,src,inc,stepsrcp)	inc[0]=(((dst[0]-src[0])/256)*stepsrcp)/256,\
							inc[1]=(((dst[1]-src[1])/256)*stepsrcp)/256,\
							inc[2]=(((dst[2]-src[2])/256)*stepsrcp)/256
#define step_interpolate(a,i) a[0]+=i[0],a[1]+=i[1],a[2]+=i[2]

typedef struct {
	const char *heightmap;
	const char *texture;
	const double y_offset;
	const double scale;
	const int is_floor;
}info_t;

typedef struct {
	float *heightmap;
	Uint32 *texture,  *input_texture;
	Uint32 *raytrace_mip;
	float *normal_map;
	int size, yoffs, floor;
	float med_val; /// @relates heightmap
	Hierarchy hierarchy;
}vox_t;

void voxel_single_frame_do(Uint32 *fb, int, int, Vector & tt, Vector & ti, Vector & tti, int thread_index, InterlockedInt&);
Uint32 voxel_raytrace(const Vector & cur, const Vector & v);
double voxel_raycast_distance(const Vector &cur, const Vector & v);

extern int voxel_rendering_method, shadow_casting_method;
extern bool light_moving;

int voxel_init(void);
void voxel_close(void);
void voxel_shoot(void);

void voxel_frame_init(const Vector& tt, const Vector &ti, const Vector &tti, Uint32* fb);

void voxel_water_init(void);
Object* voxel_water_object(void);
double voxel_get_waterlevel(void);

