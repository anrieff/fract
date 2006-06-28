/***************************************************************************
 *   Copyright (C) 2004 by Veselin Georgiev                                *
 *   vesko@ViruS                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *-------------------------------------------------------------------------*
 * vector2.h...                                                            *
 *             contains a base 2D vector class                             *
 *                                                                         *
 * vec2f - 2D vector (floats as coordinates)                               *
 *                                                                         *
 ***************************************************************************/
#ifndef __VECTOR2F_H__
#define __VECTOR2F_H__

#include <stdio.h>
#include <math.h>

#ifndef sqr
#define sqr(x) ((x)*(x))
#endif

/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
/**
 * @class vec2f
 * @short 2D vector with the usual operations
 *
 * Constructors:
 * [float, float]
 * [float*]
 * [vec2f]
 *
 * `+' - adds two vectors
 * `-' - substracts two vectors
 * `*' of the form <vector>*<scalar> - scales a vector
 * `*' of the form <vector>*<vector> - Dot (scalar) product
 * `^' - vector product
 * +=, -= - accumulation
 * ==  - true if vector are EXACTLY identical (no threshold comparison is done)
 *
 * Methods:
 * @method scale     - scales a vector by a scalar (result is not lost)
 * @method norm      - normalizes a vector to unitary
 * @method zero      - makes a vector zero
 * @method length    - returns the length
 * @method lengthSqr - returns the squared length
 * @method distto    - returns the distance to the destination
 *	@param vec - the destination vector
 * @method make_vector - makes a vector between the points A and B
 *	@param A  - first (destination) vector
 *	@param B  - second (source) vector
 * @method macc      - Multiply and accumulate;
 * 	@param start
 *	@param ray
 *	@param scale
 *	@note function: *this = start + ray * scale
 * @method add2      - assigns the sum of two vectors to *this
 * @method add3      - assigns the sum of three vectors to *this
*/
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

#define SCALE v[0]*=scale,v[1]*=scale,v[2]*=scale
class vec2f {
public:
	float v[2];

	vec2f() {}
	vec2f(float x1, float y1)
	{
		v[0] = x1; v[1] = y1;
	}
	vec2f(const float data[2])
	{
		v[0] = data[0]; v[1] = data[1];
	}
	vec2f(const vec2f &r)
	{
		v[0] = r.v[0]; v[1] = r.v[1];
	}
	vec2f& operator =(const vec2f& r) {
		v[0] = r.v[0];
		v[1] = r.v[1];
		return *this;
	}
	
	/// returns true if vectors are IDENTICAL (bitwize)
	inline bool operator == (const vec2f &r) const;
	inline bool operator != (const vec2f &r) const;

	/// compare by Y, then by X
	inline bool operator < (const vec2f &r) const;
	
	/// returns trie if vectors are roughly the same
	inline bool like(const vec2f & r, double compare_factor = 1e-5) const;

	/// calculates the vector from point `src' to point `dst'
	inline void make_vector(const vec2f & dst, const vec2f & src);
	/// calculates the vector: start + scale * ray
	inline void macc(const vec2f & start, const vec2f & ray, float scale);

	/// normalizes a non-null vector to unitary
	inline void norm();
	/// nullifies a vector
	inline void zero();
	/// returns the length of the vector
	inline float length() const;
	/// returns the squared length
	inline float lengthSqr() const;
	/// multiplies by a scalar
	inline vec2f & scale(float);

	inline void add2(const vec2f & x, const vec2f & y);
	inline void add3(const vec2f & x, const vec2f & y, const vec2f & z);
	inline void make_length(float new_length);

	/// addition
	inline const vec2f operator +(const vec2f &) const;
	/// substraction
	inline const vec2f operator -(const vec2f &) const;
	/// addition
	inline vec2f & operator +=(const vec2f &);
	/// substraction
	inline vec2f & operator -=(const vec2f &);

	/// dot product
	inline float operator * (const vec2f &) const;
	/// multiplication by a scalar
	inline vec2f operator *(float) const;
	/// cross product
	inline float operator ^ (const vec2f &) const;
	inline const float& operator [] (const unsigned index) const;
	inline float & operator [] (const unsigned index);
	inline float distto(const vec2f &) const;
	inline void print() const;
};

inline void vec2f::make_vector(const vec2f & dst, const vec2f & src)
{
	v[0] = dst.v[0] - src.v[0];
	v[1] = dst.v[1] - src.v[1];
}

inline void vec2f::macc(const vec2f & start, const vec2f & ray, float scale)
{
	v[0] = start.v[0] + scale * ray.v[0];
	v[1] = start.v[1] + scale * ray.v[1];
}

inline void vec2f::norm()
{
	float scale = 1.0f/sqrtf(v[0]*v[0]+v[1]*v[1]);
	SCALE;
}

inline void vec2f::zero()
{
	v[0] = 0.0;
	v[1] = 0.0;
}

inline vec2f & vec2f::scale(float scale)
{
	SCALE;
	return *this;
}

inline float vec2f::length() const
{
	return sqrtf(v[0]*v[0]+v[1]*v[1]);
}

inline float vec2f::lengthSqr() const
{
	return v[0]*v[0]+v[1]*v[1];
}

inline void vec2f::add2(const vec2f & x, const vec2f & y)
{
	v[0] = x.v[0] + y.v[0];
	v[1] = x.v[1] + y.v[1];
}

inline void vec2f::add3(const vec2f & x, const vec2f & y, const vec2f & z)
{
	v[0] = x.v[0] + y.v[0] + z.v[0];
	v[1] = x.v[1] + y.v[1] + z.v[1];
}

inline void vec2f::make_length(float new_length)
{
	float scale = new_length/sqrtf(v[0]*v[0]+v[1]*v[1]);
	SCALE;
}

inline const vec2f  vec2f::operator + (const vec2f & r) const
{
	return vec2f (
		v[0] + r.v[0],
		v[1] + r.v[1]
	);
}

inline const vec2f  vec2f::operator - (const vec2f & r) const
{
	return vec2f(v[0] - r.v[0],
		       v[1] - r.v[1]);
}

inline float vec2f::operator * (const vec2f & r) const
{
	return v[0]*r.v[0] + v[1]*r.v[1];
}

inline vec2f vec2f::operator *(float r) const
{
	return vec2f(v[0]*r, v[1]*r);
}

inline float vec2f::operator ^ (const vec2f & r) const
{
	return v[0] * r.v[1] - v[1] * r.v[0];
}

inline vec2f & vec2f::operator += (const vec2f & r)
{
	v[0] += r.v[0];
	v[1] += r.v[1];
	return *this;
}

inline vec2f & vec2f::operator -= (const vec2f & r)
{
	v[0] -= r.v[0];
	v[1] -= r.v[1];
	return *this;
}
inline const float& vec2f::operator [] (const unsigned index) const
{
	return v[index];
}

inline float & vec2f::operator [] (const unsigned index)
{
	return v[index];
}

inline float vec2f::distto(const vec2f & r) const
{
	return sqrt(sqr(r.v[0]-v[0]) + sqr(r.v[1]-v[1]));
}

inline bool vec2f::operator == (const vec2f & r) const
{
	return v[0] == r.v[0] && v[1] == r.v[1];
}

inline bool vec2f::operator != (const vec2f & r) const
{
	return v[0] != r.v[0] || v[1] != r.v[1];
}

inline bool vec2f::operator < (const vec2f & r) const
{
	if (v[1] != r.v[1]) return v[1] < r.v[1];
	return v[0] < r.v[0];
}

#define equal(a,b,c) (fabs((a)-(b))<(c))

inline bool vec2f::like(const vec2f &r, double compare_factor) const
{
	return 	equal(v[0], r.v[0], compare_factor) && 
		equal(v[1], r.v[1], compare_factor);
}

inline void vec2f::print() const
{
	printf("(%.4f, %.4f)", v[0], v[1]);
}

#endif // __VECTOR2F_H__
