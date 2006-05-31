/***************************************************************************
 *   Copyright (C) 2004 by Veselin Georgiev                                *
 *   vesko@ViruS                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef __COMMON_H__
#define __COMMON_H__

#include <string.h>
#include "MyGlobal.h"

#define imin(a,b) ((a)<(b)?(a):(b))
#define imax(a,b) ((a)>(b)?(a):(b))

int power_of_2(int x);
float lightformulae_tiny(float x);
Uint32 bilinea4(Uint32 x0y0, Uint32 x1y0, Uint32 x0y1, Uint32 x1y1, int x, int y);

void *sse_malloc(size_t size);
void sse_free(void *ptr);

extern int sqrt_lut[];
void common_init(void);

#ifdef _MSC_VER
// returns a with the sign bit changed to match b's
double copysign(double a, double b);
// returns the minimum of two numbers
#define fmin(a,b) ((a)<(b)?(a):(b))
#endif

#ifdef __GNUC__
#	define strcmp_without_case strcasecmp
#	else
#	ifdef _MSC_VER
#		define strcmp_without_case stricmp
#		else
#		error unsupported compiler (what does case-insensitive string 
#		error compare in you compiler)??
#	endif
#endif

#ifdef USE_LUT_SQRT

static inline float lut_sqrt(float f)
{
	__asm __volatile (
			"mov	%0,	%%eax\n"
			"mov	$1,	%%ecx\n"
			"mov	%%eax,	%%edx\n"
			"and	$0x7fffff,	%%eax\n"
			"shr	$23,	%%edx\n"
			"sub	$127,	%%edx\n"
			"mov	%1,	%%esi\n"
			"and	%%edx,	%%ecx\n"
			"sar	$1,	%%edx\n"
			"shl	$23,	%%ecx\n"
			"or	%%ecx,	%%eax\n"
			"add	$127,	%%edx\n"
			"shr	$15,	%%eax\n"
			"shl	$23,	%%edx\n"
			"mov	(%%esi,	%%eax,	4),	%%eax\n"
			"or	%%edx,	%%eax\n"
			"mov	%%eax,	%0\n"
	:
			"=m"(f)
	:
			"m"(sqrt_lut)
	:"memory", "eax", "ecx", "edx", "esi"
	);
	return f;
}
static inline float sse_sqrt(float f)
{
	__asm __volatile (
			"	rsqrtss	%0,	%%xmm0\n"
			"	rcpss	%%xmm0,	%%xmm0\n"
			"	movss	%%xmm0,	%0\n"
	:"=m"(f)
	::"memory","xmm0"
	);
	return f;
}

	#define fsqrt sse_sqrt
#else
	#ifdef fast_sqrt
		#define fsqrt fast_sqrt
	#else
		#define fsqrt sqrt
	#endif // ifdef fast_sqrt
#endif // ifdef USE_LUT_SQRT
	
#endif // __COMMON_H__
