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
	
/// Generic sorting function
/// @param a - the array of values to sort
/// @param n - the number of elements in the array
/// NOTE: sort() requires a operator &lt; to be defined
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

/**
 * @class  HashMap
 * @brief  Template like STL's map, but using a hash table for storage
 * @author Veselin Georgiev
 * @date   2006-06-18
 * 
 * NOTE: requires operator == to be defined, also the function T::get_hash_code
 *       must be present (takes no params and returns a Uint32)
*/ 
template <typename T, typename U>
class HashMap {
public:
	/// this struct is returned on operations like find(), etc.
	struct iterator {
		T first;
		U second;
		iterator * next;
		
		iterator(const T& one, const U& two) : first(one), second(two), next(NULL) {}
	};
private:
	int _count;
	iterator** hash;
	Uint32 hs;
	Uint32 _xiter;
	iterator *_yiter;
	
	void delete_chain(iterator *it)
	{
		iterator *p;
		while (it) {
			p = it->next;
			delete it;
			it = p;
		}
	}
public:
	/** Constructor
	 * @param hash_size - 
	 *	the size of the hash table to be created. The 
	 *	value should obey the following rules:
	 *
	 * 	a) The size should be at least twice the maximum number of elements you
	 *	   intend to store in the hash;
	 * 	b) The size should be a prime number
	*/ 
	HashMap(Uint32 hash_size = 10459)
	{
		hs = hash_size;
		hash = (iterator**) malloc(hs * sizeof(iterator*));
		for (Uint32 i = 0; i < hs; i++)
			hash[i] = NULL;
		_count = 0;
	}
	~HashMap()
	{
		clear();
		free(hash);
	}
	
	/// number of insert() operations (number of object inside the hash)
	int size() const
	{
		return _count;
	}
	
	/// Clears the hash map
	void clear()
	{
		for (Uint32 i = 0; i < hs; i++) if (hash[i]) {
			delete_chain(hash[i]);
			hash[i] = NULL;
		}
		_count = 0;
	}
	
	/// Map the element `t' with the value `u'
	void insert(const T& t, const U& u)
	{
		Uint32 code = t.get_hash_code() % hs;
		iterator * p = new iterator(t, u);
		p->next = hash[code];
		hash[code] = p;
		++_count;
	}
	
	/// Try to find an element in the map
	/// @returns NULL if the element is not in the map
	iterator *find(const T& t)
	{
		Uint32 code = t.get_hash_code() % hs;
		for (iterator *i = hash[code]; i; i = i->next) {
			if (t == i->first) return i;
		}
		return NULL;
	}
	
	/// Call this in order to iterate through all map's elements
	void iter_start()
	{
		_xiter = -1;
		_yiter = NULL;
	}
	
	/// Returns the next element of the iteration, or NULL if there are no more
	iterator * iter()
	{
		if (_yiter) {
			iterator *retval = _yiter;
			_yiter = _yiter->next;
			return retval;
		} else {
			++_xiter;
			while (_xiter < hs && NULL == hash[_xiter]) _xiter++;
			if (_xiter >= hs) return NULL;
			iterator * retval = hash[_xiter];
			_yiter = retval->next;
			return retval;
		}
	}
	
	/// Displays some "useless" stats
	void stats()
	{
		int c1 = 0;
		for (Uint32 i = 0; i < hs; i++) {
			if (hash[i]) c1++;
		}
		printf("Total slots = %u\nSlots used  = %u\n# of elements = %u\n",
		       hs, c1, _count);
	}
};

/**
 * @class Allocator
 * @brief Class that keeps track of local thread storage
 * @date 2006-06-22
 * @author Veselin Georgiev
 * 
 * Allocator is intended to be used with local thread storage classes. When a thread
 * needs large storage (too large to fit on stack), you can use this class's operator[]
 * to access your data, given your thread index.
 * 
 * As a side-effect, the local thread storage info is preserved across thread runs, but
 * do not rely on that.
 * 
 * The data is automatically allocated and deallocated
*/ 
template <class T>
class Allocator {
	T *xdata[64];
public:
	Allocator()
	{
		memset(xdata, 0, sizeof(xdata));
	}
	~Allocator()
	{
		for (int i = 0; i < 64; i++)
			if (xdata[i]) { delete xdata[i]; xdata[i] = 0; }
	}
	T* operator [] (int index) 
	{
		if (!xdata[index]) xdata[index] = new T;
		return xdata[index];
	}
};

#endif // __COMMON_H__
