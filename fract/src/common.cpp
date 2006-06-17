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
#if (__GNUC__ < 4)
#define uint unsigned int
static inline float fastpower_minus_quarter(float val)
{
    const float magicValue = 1331185664.0;
    float tmp = (float) *((uint*)&val);
    tmp = (tmp * -0.25) + magicValue;
    uint tmp2 = (uint) tmp;
    return *(float*)&tmp2;
}
#else
#define fastpower_minus_quarter(x) (1.0f/sqrt(sqrt(x)))
// turns out that GCC 4.0 breaks at the above function; TODO
#endif

// calculates the formula darkening which should be due to the distance from the light source
float lightformulae_tiny(float x)
{
	return 25.0 * fastpower_minus_quarter(x);
}

// given an (possibly) unaligned pointer, return an 16-byte aligned one (possibly larger)
static inline void * perfectify(void *in)
{
	long t = reinterpret_cast<long>( in );
	t = (t&~15l)+16l;
	return reinterpret_cast<void*>( t );
}

// malloc's memory with forced 16-byte alignment
// Since malloc guarantees 8-byte alignment, we allocate 16 bytes more than
// we need. Then we perfectify() the pointer. We store the address of the
// original malloc() return just before the 16-byte boundary (that is,
// 4 or 8 bytes before the address, that is returned after)
void *sse_malloc(size_t size)
{
	void *res = malloc(size + 16);// malloc guarantees 8-byte alignment
	void *perfect = perfectify(res);
	void **store = (void**)perfect;
	*(--store) = res;
	
	return perfect;
}

void sse_free(void *ptr)
{
	void * to_free = *(((void**)ptr)-1);
	if (perfectify(to_free) == ptr) {
		free(to_free);
	} else {
		printf("Memory corruption!!!\n");
		printf("Something is wrong with sse_malloc/sse_free!\n");
	}
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


