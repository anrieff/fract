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
#include "MyTypes.h"
#include "memory.h"

#define imin(a,b) ((a)<(b)?(a):(b))
#define imax(a,b) ((a)>(b)?(a):(b))
#define inf 999666111
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

#ifdef fast_sqrt
#define fsqrt fast_sqrt
#else
#define fsqrt sqrt
#endif // ifdef fast_sqrt


// checks if x is an integral power of 2. If it is not, -1 is returned, else the log2 of x is returned
int power_of_2(int x);

// calculates the formula darkening which should be due to the distance from the light source
float lightformulae_tiny(float x);

// bilinear filtering with four colors at the corners and coordinates of the sample point
// in x, y, 16-bit fixedpoint format
Uint32 bilinea4(Uint32 x0y0, Uint32 x1y0, Uint32 x0y1, Uint32 x1y1, int x, int y);

/// perlin:
/// generates fractal perlin noise
/// @param x - should be in [0..1]
/// @param y - should be in [0..1]
/// @returns a float [-1..+1]
float perlin(float x, float y);


void common_init(void);



// smooth hermite interpolation between the values of a and b (t must be in [0..1])
template <typename T>
T hermite(const T& a, const T& b, double t)
{
	return a + (a - b) * (-3.0 * t * t + 2.0 * t * t* t);
}

// min and max templates
template <typename T>
static inline T min(T a, T b) { return a < b ? a : b; }
template <typename T>
static inline T max(T a, T b) { return a > b ? a : b; }


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
	
/** Generic sorting function
 *  THREAD SAFETY: No (uses static storage)
 *  STABLE: Yes
 *  @param a - the array of values to sort
 *  @param n - the number of elements in the array
 * NOTE: sort() requires a operator &lt; to be defined
*/
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
 * Generic sorting function for use in multithreaded environment
 * THREAD SAFETY: Yes
 * STABLE: No
 * @param a - the array of values to sort
 * @param n - the number of elements in the array
 * NOTE: sort_inplace() requires a operator &lt; to be defined
*/
template <typename T>
void sort_inplace(T a[], int n)
{
	a--; // make the array 1-based
	// Stage 1: create the heap
	for (int i = 2; i <= n; i++) {
		int x = i;
		while (x != 1 && a[x/2] < a[x]) { // sift up
			T t = a[x/2];
			a[x/2] = a[x];
			a[x] = t;
			x /= 2;
		}
	}
	
	// Stage 2: extract elements from the heap
	for (; n; n--) {
		// exchange top & tail:
		T t = a[1];
		a[1] = a[n];
		a[n] = t;
		
		// sift the top element down
		int x = 1, y;
		while (1) {
			y = -1;
			if (x * 2 < n && a[x] < a[x*2]) y = x * 2;
			if (x * 2 + 1 < n && a[x] < a[x*2+1] && a[x*2] < a[x*2+1]) y = x * 2 + 1;
			if (y == -1) break;
			t = a[x];
			a[x] = a[y];
			a[y] = t;
			x = y;
		}
	}
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
};


enum AllocatorType {
	ALLOCATOR_MALLOC_FREE,
	ALLOCATOR_NEW_DELETE
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
 * 
 * The constructor accepts an optional parameter which determines how allocation and
 * deallocation will be performed; malloc()/free() in the case of ALLOCATOR_MALLOC_FREE,
 * new/delete in the case of ALLOCATOR_NEW_DELETE.
*/ 
template <class T>
class Allocator {
	T *xdata[64];
	AllocatorType type;
public:
	Allocator(AllocatorType mytype = ALLOCATOR_NEW_DELETE)
	{
		memset(xdata, 0, sizeof(xdata));
		type = mytype;
	}
	~Allocator()
	{
		for (int i = 0; i < 64; i++) {
			if (xdata[i]) { 
				if (type == ALLOCATOR_MALLOC_FREE)
					sse_free(xdata[i]);
				else
					delete xdata[i];
				xdata[i] = 0;
			}
		}
	}
	T* operator [] (int index) 
	{
		if (!xdata[index]) {
			if (type == ALLOCATOR_MALLOC_FREE)
				xdata[index] = (T*) sse_malloc(sizeof(T));
			else
				xdata[index] = new T;
		}
		return xdata[index];
	}
};

#endif // __COMMON_H__
