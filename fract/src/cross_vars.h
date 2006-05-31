/***************************************************************************
 *   Copyright (C) 2004 by Veselin Georgiev                                *
 *   vesko@ViruS                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *-------------------------------------------------------------------------*
 * This file is used for cross-definition of variables between fract.cpp   *
 * and render.cpp                                                          *
 ***************************************************************************/

#include "bitmap.h"
#include "sphere.h"
#include "triangle.h"
extern RawImg tex;

// the default font

extern RawImg font0;

// precalculated storage...

extern int *sqrtsqrt;

// the framebuffer:

extern Uint32 framebuffer[RES_MAXX*RES_MAXY];
extern int apply_blur;

// texture collection:

extern RawImg T[12]; // this allows texture sizes up to 4096 ;)
extern int end_tex;

// sphere collection

extern sphere sp[MAX_SPHERES];
extern Vector pp[MAX_SPHERES][MAX_SPHERE_SIDES];
extern int num_sides[MAX_SPHERES];
extern int spherecount;


extern int vframe, bilfilter;

extern Uint32 clk;
extern double daFloor, daCeiling;
extern int sse_enabled, mmx2_enabled, mmx_enabled;
extern double gX, gY;
extern double alpha, beta;
extern Vector w[3], cur;
extern double pdlo[MAX_SPHERES];
extern double lft, delta;
extern double speed;
extern double anglespeed;
extern double mouse_sensitivity, fov;

extern int developer;
extern int RenderMode;

extern int not_fps_written_yet;
extern int runmode;
extern int camera_moved;

extern int loop_mode;
extern int loops_remaining;

// the light source
extern int lx, ly, lz;

extern int ysqrd; // light y distance to the current plane (floor or ceiling), squared
extern int ysqrd_floor, ysqrd_ceil;

extern int CollDetect;
extern int Physics;
extern double play_time;
extern int cd_frames;
extern int kbd_control;
extern int file_control;
extern int use_shader;

extern double beta_limit_low, beta_limit_high;

extern int user_pp_state;

#ifdef MAKE_CHECKSUM
extern Uint32 cksum;
#endif

#ifdef SHOWFPS
extern Uint32 fpsclk;
extern int fpsfrm;
#endif

extern bool savevideo;
