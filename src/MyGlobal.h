/***************************************************************************
 *   Copyright (C) 2004 by Veselin Georgiev                                *
 *   vesko@ViruS                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/
// assure non-reentrance
#ifndef MYGLOBAL_H
#define MYGLOBAL_H

#define Fract_Version "unofficial"
#ifdef __SSE3__
#	ifdef SIMD_VECTOR
#		define Mod_Instruction_Set "SSE3v"
#	else

#		define Mod_Instruction_Set "SSE3"
#	endif
#else
#	ifdef __SSE2__
#		ifdef SIMD_VECTOR
#			define Mod_Instruction_Set "SSE2v"
#		else
#			define Mod_Instruction_Set "SSE2"
#		endif
#	else
#		define Mod_Instruction_Set "SSE"
#	endif
#endif

// this will toggle between flipping and just drawing on the framebuffer
#define ACTUALLYDISPLAY

#include "asmconfig.h"

#ifdef USE_ASSEMBLY
// this will select whether CPU speed should be shown in the result screen
#define SHOW_CPU_SPEED
#endif


// maximum framebuffer sizes
#define RES_MAXX 1920
#define RES_MAXY 1200

#define BLUR 1

// enable this to create a file with the coordinates for each frame generated
//#define RECORD 1
#define RECORD_FILE "rekord.txt"

// if the following define exists then no time calculations are performed: frames are rendered
// consecutively from the data given in the context file.
#define IGNORE_TIME 1


// this would create a checksum for all frames, if uncommented
//#define MAKE_CHECKSUM 1

// ... will create checksum, but only on the first frame
//#define ONLY_FIRST_FRAME 1

// if this is defined, data files are checked for consistency using MD5
// otherwise they are still checked, but only warning is emitted and .result files are
// created as usual
#define MD5_CHECK

// these are the phong shading coefficients. Change them as you like, but try to keep their sum below 1.
#define ambient 0.2
#define diffuse 0.55
#define specular 0.15
// ambient * 65536
#define iambient 13107

// get MAX_SPHERES, MAX_TRIANGLES, etc.
#include "MyLimits.h"


// These are all used in the sphere flags. They can be ORed to be in effect simultaneously

// the object will be raytraced to a single reflection
#define RAYTRACED	0x00000001

// allows multiple reflections
#define RECURSIVE	0x00000002

// the object will be traced through (will be transparent)
#define SEETHROUGH	0x00000004

// applies to meshes only. If set, reflections will reflect other parts of the mesh;
// useful if the mesh object is not convex
// (MESHES ONLY)
#define RECURSE_SELF    0x00000008

// the object will never bounce, have gravity, be animated .. etc.
#define STATIC		0x00000010
// object can collide
#define COLLIDEABLE	0x00000020
// (SPHERE ONLY) new coords are calculated each frame using a corresponding AniSphere structure
#define ANIMATED	0x00000040
// present on objects, which don't bounce off floors/ceilings/etc.
#define NOFLOORBOUNCE	0x00000080
// object will be affected by the gravity
#define GRAVITY		0x00000100
// object will be affected by the air resistance
#define AIR		0x00000200

// object will be mapped with the given texture
#define MAPPED		0x00001000
// object will have normal map, making the object look smooth
// (MESHES ONLY)
#define NORMAL_MAP	0x00002000
// object has a bump map (MESH ONLY)
#define BUMPMAPPED	0x00004000
// (MESHES ONLY) perlin noise will be added to the light intensity channel, making the material look rough
#define STOCHASTIC	0x00008000

// Defined if the object casts shadows on other objects
#define CASTS_SHADOW	0x00010000

// Defined if the object is transparent, but the coefficients of reflection and
// transmittance are not fixed, but calculated via fresnel equations. When set,
// the .refl property of the object is ignored, and the opacity field determines
// contribution of reflection+refraction to the color.
#define FRESNEL		0x00100000

// This is a temporal, per-frame flag - it is set when the whole object is under shadow
#define SHADOWED	0x01000000

// This is a temporal, per-frame flag - it is set if some part of the object is under shadow
#define PART_SHADOWED	0x02000000

// tune the following constant up if the spheres look too choppy to you.
#define MIN_SPHERE_SIDES 8

//Pithagor rullz
#define pith0(x1, y1, z1, x2, y2, z2) fast_sqrt(sqr(x1-x2)+sqr(y1-y2)+sqr(z1-z2))
#define pith(a, b) fast_sqrt(sqr(a[0]-b[0])+sqr(a[1]-b[1])+sqr(a[2]-b[2]))
#define EPS 0.0000000001

// The maximum recursive call depth of "raytrace".
// If exceeded, the value returned is "RAYTRACE_BLEND_COLOR"
#define MAX_RAYTRACE_ITERATIONS 9
#define RAYTRACE_BLEND_COLOR 0x000000

// The overall brightness
#define BRIGHTNESS_C 25.0
#ifdef _MSC_VER
// some constants which aren't in MSVC++'s math.h
#define M_PI 3.1415926535897932384626433832795
#define M_PI_2 1.5707963267948966192313216916398
#define round(x) ((int) ((x)+0.5))
#endif

#define FOV_CORRECTION 0.882
// this defines the Field-Of-View constant. 1 means 90 degrees.
//90 degrees is usually best (determined experimentally). More than 110 degrees are *FREAKY*
#define STANDARD_FIELD_OF_VIEW 1.0



/****************************************************
 * These defines did belong to fract.cpp            *
 ****************************************************/
// the maximum distance that the illumination lookup table keeps
#define MAX_DIST 512*512

// current texture size
#define MAX_TEXTURE_SIZE	256
#define LOG2_MAX_TEXTURE_SIZE	8
#define LOG2_TEXTURE_AREA	(LOG2_MAX_TEXTURE_SIZE*2)
// the `8' here should be LOG2_MAX_TEXTURE_SIZE
#define SHIFT_LEFT_LOG2_OF_MAX_TEXTURE_SIZE_mm2 "	pslld	$8,	%%mm2\n"
#define SHIFT_LEFT_LOG2_OF_MAX_TEXTURE_SIZE_mm3 "	pslld	$8,	%%mm3\n"


// the default mipmap level which will be used when reflecting/refracting
#define TEX_S 3
// if this is defined, additional rays will be shot out to determine best miplevel
// for first reflection texture filtering
#define TEX_OPTIMIZE
#define BALL_T_AND_MASK ((MAX_TEXTURE_SIZE>>TEXTURE_FOR_BALLS)-1)

// uncomment this if you want to see mipmap levels instead of textures
//#define DEBUG_MIPMAPS

// round a 16.16 fixedpoint integer
#define r(x) ((x+32768)/65536)
#define i2im(a, b) ((((unsigned) a) * ((unsigned) b))>>16)
#define PDIVIZOR 8
// the log2(PDIVIZOR):
#define DIVIZOR_SHIFT 3

#define __512 256*256

#define iabs(x) (((x)<0)?-(x):(x))
#define sqr(x) ((x)*(x))
#define clamp(a) (a<0?0:(a>255?255:(a)))

// if a ray is dropped to the floor engine it checks if the |y| component of the vector is lower than this threshold.
// The idea is that such rays go in the infinity and should be painted black. The texturer should paint them anyway, but due
// to some nasty inaccuracy in the fixedpoint math some full intensity white points are introduced, garbling the picture.
#define DST_THRESHOLD 0.0055

// define how we ask the compiler to align variables on a 16 byte boundary
#ifdef _MSC_VER
#define SSE_ALIGN(x) __declspec(align(16)) x
#else // _MSC_VER
#ifdef __GNUC__
#define SSE_ALIGN(x) x __attribute__((aligned(16)))
#else
#error unsupported compiler
#endif // __GNUC__
#endif // _MSC_VER

// Show the FPS Counter in the upper-left corner of the screen
#define SHOWFPS

// Show the active antialiasing mode in the upper right corner of the screen
#define SHOWFSAA

// Gather statistics for the prefiller accuracy
//#define FSTAT

// Use bilinear filter by default
#define BILINEAR_FILTER

// uncomment the following to create a dump of the prepass framebuffer in a file: prepass_dump.bmp
// along with a screenshot in final_dump.bmp. Activated with "d" from the keyboard
#define DUMPPREPASS

#ifdef SHOWFPS
// update the FPS thing two times per second
#define FPS_UPDATE_INTERVAL 500
#endif

// when the screen capture is on, this string will be used as a file prefix, before
// imageXXXX.bmp (could be a path, too):
#define ANIMATION_PREFIX "video/"

#endif //MYGLOBAL_H
