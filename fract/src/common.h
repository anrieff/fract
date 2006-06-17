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

#include <stdlib.h>
#include <string.h>
#include "MyGlobal.h"

#define imin(a,b) ((a)<(b)?(a):(b))
#define imax(a,b) ((a)>(b)?(a):(b))
#define inf 999666111


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


#define SSSTORAGE_SIZE (1024*1024)
extern char static_sort_storage[SSSTORAGE_SIZE];

	
template <typename T>
void _mergesort_imp(T a[], int l, int r, T t[])
{
	/* Stage 1 : check for bottom */
	if (r <= l) return;
	if (r == l + 1) {
		if (a[r] < a[l]) {
			t[0] = a[l];
			a[l] = a[r];
			a[r] = t[0];
		}
		return;
	}
	
	/* Stage 2 : recurse down */
	int m = (l + r) / 2;
	_mergesort_imp(a, l, m, t);
	_mergesort_imp(a, m + 1, r, t);
	
	/* Stage 3 : merge */
	int size = (r - l + 1);
	int i = l, j = m + 1, k = 0;
	while (i <= m && j <= r) {
		if (a[j] < a[i])
			t[k++] = a[j++];
		else
			t[k++] = a[i++];
	}
	if (i <= m) for (; i <= m; i++) t[k++] = a[i];
	if (j <= r) for (; j <= r; j++) t[k++] = a[j];
	for (k = 0; k < size; k++)
		a[l + k] = t[k];
}
	
// Generic sorting function
template <typename T>
void sort(T a[], int n)
{
	// if no sorting is needed
	if (n < 2) return;
	
	// try using the static storage...
	T* storage = (T*) static_sort_storage;
	bool static_storage = true;
	
	if (sizeof(T) * n > SSSTORAGE_SIZE) {
		// .. but if it is not enough, use a dynamic one
		static_storage = false;
		storage = (T*) malloc(sizeof(T)*n);
	}
	
	// perform the actual sortng
	_mergesort_imp(a, 0, n - 1, storage);
	
	// non static storage must be freed
	if (!static_storage)
		free(storage);
}

#endif // __COMMON_H__
