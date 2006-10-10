/***************************************************************************
 *   Copyright (C) 2004 by Veselin Georgiev                                *
 *   vesko@ViruS                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *-------------------------------------------------------------------------*
 *   A few common and useful functions                                     *
 ***************************************************************************/

#include "MyGlobal.h"
#include "MyTypes.h"
#include "common.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// static storage for the sort() function
char static_sort_storage[SSSTORAGE_SIZE];

// checks if x is an integral power of 2. If it is not, -1 is returned, else the log2 of x is returned
int power_of_2(int x)
{
	int c=0;
	if (!x) return -1; //zero is not a power of two
	while (x!=1) if (x%2) return -1; else {x/=2; c++;}
	return c;
}

/*
	The following idea of a fast reciprocal of a 4th root approximation is influenced directly from
	http://playstation2-linux.com/files/p2lsd/fastrsqrt.pdf

	In fact, the algorithm suggested ther worked fine for 4th root (instead of square root) with a simple
	constants substitution ...

	10x to Matthew Jones for this amazing piece of work.

	(	the function approximates 1/sqrt(sqrt(x)).	The error is within +/-3 % 	)

	I have *NO* idea how this thing works
*/

/*
 * Note (anrieff): The code in its original version didn't work with gcc4 
 * (presumably due to aliasing issues). Using an union instead of those
 * magical `* ((uint*)&val)' casts now works out correctly for gcc ver 3 and 4.
 */
#define uint unsigned int
static inline float fastpower_minus_quarter(float val)
{
	const float magicValue = 1331185664.0;
	union {
		float as_float;
		uint as_int;
	} value;
	value.as_float = val;
	float tmp = (float) value.as_int;
	tmp = (tmp * -0.25) + magicValue;
	value.as_int = (uint) tmp;
	return value.as_float;
}

// calculates the formula darkening which should be due to the distance from the light source
float lightformulae_tiny(float x)
{
	return 25.0 * fastpower_minus_quarter(x);
}




static float noise(unsigned x)
{
	x = (x<<13) ^ x;
	return ( 1.0 - ( (x * (x * x * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0);
}
/// perlin:
/// generates fractal perlin noise
/// @param x - should be in [0..1]
/// @param y - should be in [0..1]
/// @returns a float [-1..+1]
float perlin(float x, float y)
{
	float res = 0.0f, amp = 1.0f;
	unsigned size = 1;
	for (int i = 0; i <6; i++, size*=2) {
		unsigned x0 = (unsigned) (x*size);
		unsigned y0 = (unsigned) (y*size);
		unsigned q = x0 + y0 *size;
		float fx = x*size - x0;
		float fy = y*size - y0;
		float nf = noise(q       )*(1.0f-fx)*(1.0f-fy) +
				noise(q+1     )*(     fx)*(1.0f-fy) +
				noise(q  +size)*(1.0f-fx)*(     fy) +
				noise(q+1+size)*(     fx)*(     fy);
		res += amp * nf;
		amp *= 0.72;
	}
	return res;
}


#ifdef _MSC_VER
#define DOUBLE_SIGN_MASK 0x8000000000000000
double copysign(double a, double b)
{
	long long *aa=(long long*)&a, *bb = (long long*) &b;
	aa[0] = (aa[0] & ~DOUBLE_SIGN_MASK) | (bb[0] & DOUBLE_SIGN_MASK);
	return a;
}
#endif

#define FSQRT_PRECISION 8
#define FSQRT_SHIFT (23-FSQRT_PRECISION)
#define FSQRT_ENTRIES (2*(1<<FSQRT_PRECISION))

int sqrt_lut[FSQRT_ENTRIES];

static void build_sqrt_lut(void)
{
	unsigned i;
	volatile float f;
	volatile unsigned int *fi = (unsigned int *) &f;
	for (i = 0; i < FSQRT_ENTRIES / 2; i++) {
		*fi = 0;
		*fi = (i << FSQRT_SHIFT) | (127 << 23);
		f = sqrt(f);
		sqrt_lut[i] = (*fi & 0x7fffff) ;
		*fi = (i << FSQRT_SHIFT) | (128 << 23);
		f = sqrt(f);
		sqrt_lut[i + FSQRT_ENTRIES/2] = (*fi & 0x7fffff) ;
		
	}
}

void common_init(void)
{
	build_sqrt_lut();
}

/*inline float lut_sqrt(float f)
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
}*/

#define common_asm
#include "x86_asm.h"
#undef common_asm


