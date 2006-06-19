/***************************************************************************
 *   Copyright (C) 2004 by Veselin Georgiev                                *
 *   vesko@ViruS                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef __RENDER_H__
#define __RENDER_H__

//#define FPS_COLOR 0xfe4bef
#define FPS_COLOR 0x23ea90

#define SHADOW_QUEUE_SIZE 8192
#define qmod (SHADOW_QUEUE_SIZE-1)
#define QSENTINEL -200

#define S_SPNUM_MASK		0x7F00
#define S_UPPER_MASK		0xFF00
#define S_MAX_BLACK		0x007F
#define S_COLOR_MASK		0x00FF
#define S_COLOR_BITS		7

// if the most significant bit in a word in the sbuffer is set, then the pixel contains blended data from two shadows
#define S_MIXED_MASK		0x8000

#define S_SPNUM_MODULUS		0x7f
#define S_SPNUM_OFFSET		8

// this controls the "smoothness" of the shadow edges
#define S_EDGE 0.5f
#define S_DEFAULT_OPACITY 0.2

// lower value = less bugs
#define CANDIDATE_INTERVAL	4

#include "MyGlobal.h"
#include "vector3.h"
#include "object.h"
#include "threads.h"

/*
ML_BUFFER_GRAN defines the subsampling accuracy of the MipLevel History Buffer.
E.g. for ML_BUFFER_GRAN = 16, a single value will be stored for each
16x16 screen square
*/
#define ML_BUFFER_GRAN	16

enum {
	STEREO_MODE_NONE,
	STEREO_MODE_LEFT,
	STEREO_MODE_RIGHT,
	STEREO_MODE_FINAL
};

enum ORType {
	OPENING,
	CLOSING
};

struct ORNode {
	int object_id;
	int y;
	ORType type;
	bool operator < (const ORNode & r) const
	{
		if (y != r.y) return y < r.y;
		return (type == OPENING && r.type == CLOSING);
	}
};

extern ObjectArray allobjects;
extern bool parallel;
extern double stereo_separation;
extern double stereo_depth;
extern ThreadPool thread_pool;

#ifdef ACTUALLYDISPLAY
void render_single_frame(SDL_Surface *p, SDL_Overlay *ov);
void postframe_do(SDL_Surface *p, SDL_Overlay *ov);
#else
void render_single_frame(void);
void postframe_do(void);
#endif

bool testsphere(Sphere *a, const Vector & v, const Vector & cur, const double & A, double & determinant, double & gB);

int render_init(void);
void bash_preframe(Vector& lw, Vector& tt, Vector& ti, Vector& tti);
// nearest doesn't need the fractional part...
void move_camera(double angle);
void rotate_camera(double alphai, double betai);
void check_params(void);
void blur_reinit(void);
Uint32 *get_frame_buffer(void);

void render_close(void);

void fract_thread(void);

#ifdef _MSC_VER
double log2(double x);
#endif


#endif // __RENDER_H__
