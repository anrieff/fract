/***************************************************************************
 *   Copyright (C) 2004 by Veselin Georgiev                                *
 *   vesko@ViruS                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *-------------------------------------------------------------------------*
 * vector3.h...                                                            *
 *             contains a base 3D vector class                             *
 *                                                                         *
 * Vector - 3D vector (doubles as coordinates)                             *
 *                                                                         *
 * Contains SSE2 and SSE3 optimized versions, which one is compiled is     *
 * determined by the use of compilation defines __SSE2__ and __SSE3__      *
 * which are selected by `-msse2' and `-msse3' compiler switches in gcc    *
 ***************************************************************************/
#ifndef __VECTOR3_H__
#define __VECTOR3_H__

#include <stdio.h>
#include <math.h>

#ifndef sqr
#define sqr(x) ((x)*(x))
#endif

/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
/**
 * @class Vector
 * @short 3D Vector with the usual operations
 *
 * Constructors:
 * [double, double, double]
 * [double*]
 * [Vector]
 *
 * `+' - adds two vectors
 * `-' - substracts two vectors
 * `*' of the form <vector>*<scalar> - scales a vector
 * `*' of the form <vector>*<vector> - Dot (scalar) product
 * `^' - Vector product
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

//#if !defined __SSE2__ && !defined __SSE3__
#define SCALE v[0]*=scale,v[1]*=scale,v[2]*=scale
#define real double
class Vector {
public:
	union {
		real v[4];
		struct {real x, y, z, a;};
		};

	Vector() {}
	Vector(const real& x1, const real& y1, const real& z1):x(x1), y(y1), z(z1) {}
	Vector(real data[3]) : x(data[0]), y(data[1]), z(data[2]) {}
	Vector(const Vector &r) : x(r.v[0]), y(r.v[1]), z(r.v[2]) {}
	Vector& operator =(const Vector& r) {
		v[0] = r.v[0];
		v[1] = r.v[1];
		v[2] = r.v[2];
		return *this;
	}
	
	/// returns true if vectors are IDENTICAL (bitwize)
	inline bool operator == (const Vector &r) const;
	inline bool operator != (const Vector &r) const;
	
	/// returns trie if vectors are roughly the same
	inline bool like(const Vector & r, double compare_factor = 1e-9) const;

	/// calculates the vector from point `src' to point `dst'
	inline void make_vector(const Vector & dst, const Vector & src);
	/// calculates the vector: start + scale * ray
	inline void macc(const Vector & start, const Vector & ray, const real & scale);

	/// normalizes a non-null vector to unitary
	inline void norm();
	/// nullifies a vector
	inline void zero();
	/// returns the length of the vector
	inline real length() const;
	/// returns the squared length
	inline real lengthSqr() const;
	/// multiplies by a scalar
	inline Vector & scale(real const &);

	inline void add2(const Vector & x, const Vector & y);
	inline void add3(const Vector & x, const Vector & y, const Vector & z);
	inline void make_length(const real & new_length);

	/// addition
	inline const Vector operator +(const Vector &) const;
	/// substraction
	inline const Vector operator -(const Vector &) const;
	/// addition
	inline Vector & operator +=(const Vector &);
	/// substraction
	inline Vector & operator -=(const Vector &);

	/// dot product
	inline real operator * (const Vector &) const;
	/// multiplication by a scalar
	inline Vector operator *(real const & ) const;
	/// cross product
	inline Vector operator ^ (const Vector &) const;
	inline const real& operator [] (const unsigned index) const;
	inline real & operator [] (const unsigned index);
	inline real distto(const Vector &) const;
	inline void print() const;
};

inline void Vector::make_vector(const Vector & dst, const Vector & src)
{
	v[0] = dst.v[0] - src.v[0];
	v[1] = dst.v[1] - src.v[1];
	v[2] = dst.v[2] - src.v[2];
}

inline void Vector::macc(const Vector & start, const Vector & ray, const real & scale)
{
	v[0] = start.v[0] + scale * ray.v[0];
	v[1] = start.v[1] + scale * ray.v[1];
	v[2] = start.v[2] + scale * ray.v[2];
}

inline void Vector::norm()
{
	real scale = 1.0/sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);
	SCALE;
}

inline void Vector::zero()
{
	v[0] = 0.0;
	v[1] = 0.0;
	v[2] = 0.0;
}

inline Vector & Vector::scale(real const &  scale)
{
	SCALE;
	return *this;
}

inline real Vector::length() const
{
	return sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);
}

inline real Vector::lengthSqr() const
{
	return v[0]*v[0]+v[1]*v[1]+v[2]*v[2];
}

inline void Vector::add2(const Vector & x, const Vector & y)
{
	v[0] = x.v[0] + y.v[0];
	v[1] = x.v[1] + y.v[1];
	v[2] = x.v[2] + y.v[2];
}

inline void Vector::add3(const Vector & x, const Vector & y, const Vector & z)
{
	v[0] = x.v[0] + y.v[0] + z.v[0];
	v[1] = x.v[1] + y.v[1] + z.v[1];
	v[2] = x.v[2] + y.v[2] + z.v[2];
}

inline void Vector::make_length(const real & new_length)
{
	real scale = new_length/sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);
	SCALE;
}

inline const Vector  Vector::operator + (const Vector & r) const
{
	return Vector (
		v[0] + r.v[0],
		v[1] + r.v[1],
		v[2] + r.v[2]
	);
}

inline const Vector  Vector::operator - (const Vector & r) const
{
	return Vector(v[0] - r.v[0],
		       v[1] - r.v[1],
		       v[2] - r.v[2]);
}

inline real Vector::operator * (const Vector & r) const
{
	return v[0]*r.v[0] + v[1]*r.v[1] + v[2]*r.v[2];
}

inline Vector Vector::operator *(real const &  r) const
{
	return Vector(v[0]*r, v[1]*r, v[2]*r);
}

inline Vector Vector::operator ^ (const Vector & r) const
{
	return Vector(v[1]*r.v[2] - v[2]*r.v[1],
		       v[2]*r.v[0] - v[0]*r.v[2],
		       v[0]*r.v[1] - v[1]*r.v[0]
		       );
}

inline Vector & Vector::operator += (const Vector & r)
{
	v[0] += r.v[0];
	v[1] += r.v[1];
	v[2] += r.v[2];
	return *this;
}

inline Vector & Vector::operator -= (const Vector & r)
{
	v[0] -= r.v[0];
	v[1] -= r.v[1];
	v[2] -= r.v[2];
	return *this;
}
inline const real& Vector::operator [] (const unsigned index) const
{
	return v[index];
}

inline real & Vector::operator [] (const unsigned index)
{
	return v[index];
}

inline real Vector::distto(const Vector & r) const
{
	return sqrt(sqr(r.v[0]-v[0]) + sqr(r.v[1]-v[1]) + sqr(r.v[2]-v[2]));
}

inline bool Vector::operator == (const Vector & r) const
{
	return v[0] == r.v[0] && v[1] == r.v[1] && v[2] == r.v[2];
}

inline bool Vector::operator != (const Vector & r) const
{
	return v[0] != r.v[0] || v[1] != r.v[1] || v[2] != r.v[2];
}

#define equal(a,b,c) (fabs((a)-(b))<(c))

inline bool Vector::like(const Vector &r, double compare_factor) const
{
	return 	equal(v[0], r.v[0], compare_factor) && 
		equal(v[1], r.v[1], compare_factor) && 
		equal(v[2], r.v[2], compare_factor);
}

inline void Vector::print() const
{
	printf("(%.4lf, %.4lf, %.4lf)\n", x, y, z);
}

#define fast_float_sqrt(x) sqrtf(x)
#if defined __GNUC__ && defined __SSE__ && defined __SSE2__
#define fast_sqrt(x) \
({\
	register double _resultz;\
	__asm__ (\
	"	sqrtsd	%1,	%0\n"\
	:"=x"(_resultz)\
	:"x"(x)\
	);\
	_resultz;\
})
#else
#define fast_sqrt(x) sqrt(x)
#endif

//#endif // Non-sse2 version
#define VECTOR3_ASM
#include "x86_asm.h"
#undef VECTOR3_ASM

/**************************************************************************************************************************************/

//#if defined __SSE2__
#if 0

// enumerate the vector types we're using
typedef int v2df __attribute__ ((mode(V2DF),aligned(16)));
typedef int v4sf __attribute__ ((mode(V4SF),aligned(16)));


#endif

//#if defined __SSE2__ && !defined __SSE3__
#if 0
class Vector {
	friend class vector3;
public:
	union {
		SSE_ALIGN(double v[4]);
		SSE_ALIGN(v2df a[2]);
	};
	Vector(){}
	inline Vector(double x, double y, double z) {
		v[0] = x;
		v[1] = y;
		v[2] = z;
		//v[3] = 0.0;
	}
	inline Vector(double const *data) {
		a[0] = *(v2df*)data;
		v[2] = data[2];
	}

	inline Vector(const Vector &r) {
		a[0] = r.a[0];
		v[2] = r.v[2];
	}
	inline Vector & operator = (const Vector & r) {
		a[0] = r.a[0];
		v[2] = r.v[2];
		return *this;
	}

	inline void make_vector(const Vector & dst, const Vector & src) {
		a[0] = __builtin_ia32_subpd(dst.a[0], src.a[0]);
		v2df	t0 = __builtin_ia32_loadsd(dst.v+2),
			t1 = __builtin_ia32_loadsd(src.v+2);
		__builtin_ia32_storesd(v+2, __builtin_ia32_subsd(t0, t1));
	}

	inline void add2(const Vector & x, const Vector & y) {
		a[0] = __builtin_ia32_addpd(x.a[0], y.a[0]);
		v2df	t0 = __builtin_ia32_loadsd(x.v+2),
			t1 = __builtin_ia32_loadsd(y.v+2);
		__builtin_ia32_storesd(v+2, __builtin_ia32_addsd(t0, t1));
	}

	inline void add3(const Vector & x, const Vector & y, const Vector & z) {
		v2df t = __builtin_ia32_addpd(x.a[0], y.a[0]);
		a[0]   = __builtin_ia32_addpd(t, z.a[0]);
		v2df	t0 = __builtin_ia32_loadsd(x.v+2),
			t1 = __builtin_ia32_loadsd(y.v+2),
			t2 = __builtin_ia32_loadsd(z.v+2);
		__builtin_ia32_storesd(v+2, __builtin_ia32_addsd(t2,
					    __builtin_ia32_addsd(t0, t1)));
	}

	inline void macc(const Vector & start, const Vector & ray, const double & scale) {
		// *this = start + ray * scale:
		v2df 	mm = __builtin_ia32_loadsd(&scale),
			rr = __builtin_ia32_loadsd(ray.v+2);
			mm = __builtin_ia32_shufpd(mm,mm,0);
		v2df	st = __builtin_ia32_loadsd(start.v+2),
			t0 = __builtin_ia32_mulpd(ray.a[0], mm);
			rr = __builtin_ia32_mulsd(rr, mm);
		a[0] = __builtin_ia32_addpd(start.a[0], t0);
		__builtin_ia32_storesd(v + 2, __builtin_ia32_addsd(rr, st));
	}

	inline void make_length(const double & new_length) {
		v2df 	t0 = a[0],
			t1 = __builtin_ia32_loadsd(v+2);

		v2df	oo = __builtin_ia32_loadsd(&new_length);

		v2df	p0 = __builtin_ia32_mulpd(t0, t0),
			p1 = __builtin_ia32_mulsd(t1, t1);
		v2df	p2 = __builtin_ia32_shufpd(p0, p0, 1);
			p0 = __builtin_ia32_addsd(p0, p1);
			p0 = __builtin_ia32_sqrtsd(__builtin_ia32_addsd(p0, p2));
			oo = __builtin_ia32_divsd(oo, p0);
			oo = __builtin_ia32_shufpd(oo,oo,0);
			a[0] = __builtin_ia32_mulpd(t0, oo);
			__builtin_ia32_storesd(v+2, __builtin_ia32_mulsd(t1, oo));
	}

	/// normalizes a non-null vector to unitary
	inline void norm() {
		v2df 	t0 = a[0],
			t1 = __builtin_ia32_loadsd(v+2);

		const double one = 1.0;
		v2df	oo = __builtin_ia32_loadsd(&one);

		v2df	p0 = __builtin_ia32_mulpd(t0, t0),
			p1 = __builtin_ia32_mulsd(t1, t1);
		v2df	p2 = __builtin_ia32_shufpd(p0, p0, 1);
			p0 = __builtin_ia32_addsd(p0, p1);
			p0 = __builtin_ia32_sqrtsd(__builtin_ia32_addsd(p0, p2));
			oo = __builtin_ia32_divsd(oo, p0);
			oo = __builtin_ia32_shufpd(oo,oo,0);
			a[0] = __builtin_ia32_mulpd(t0, oo);
			__builtin_ia32_storesd(v+2, __builtin_ia32_mulsd(t1, oo));
	}
	/// nullifies a vector
	inline void zero() {
		a[0] = (v2df) __builtin_ia32_setzeropd();
		a[1] = (v2df) __builtin_ia32_setzeropd();
	}
	/// returns the length of the vector
	inline double lengthSqr() const {
		return v[0]*v[0]+v[1]*v[1]+v[2]*v[2];
	}
	/// returns the squared length
	inline double length() const {
			v2df	tmp0 = __builtin_ia32_loadsd(v    ),
			tmp1 = __builtin_ia32_loadsd(v + 1),
			tmp2 = __builtin_ia32_loadsd(v + 2);
			tmp0 = __builtin_ia32_mulsd(tmp0, tmp0);
			tmp1 = __builtin_ia32_mulsd(tmp1, tmp1);
			tmp2 = __builtin_ia32_mulsd(tmp2, tmp2);
			tmp0 = __builtin_ia32_addsd(tmp0, tmp1);
			tmp0 = __builtin_ia32_addsd(tmp0, tmp2);
			tmp0 = __builtin_ia32_sqrtsd(tmp0);
			double result;
			__builtin_ia32_storesd(&result, tmp0);
		return result;
	}
	/// multiplies by a scalar
	inline Vector & scale(double const & scale) {
		v[0] *= scale; v[1] *= scale; v[2] *= scale;
		return *this;
	}

	/// addition
	inline const Vector operator +(const Vector & r) const
	{
		Vector res;
		res.a[0] = __builtin_ia32_addpd(a[0], r.a[0]);
		v2df	t1 = __builtin_ia32_loadsd(v+2),
			t2 = __builtin_ia32_loadsd(r.v+2);
			__builtin_ia32_storesd(res.v+2,__builtin_ia32_addsd(t1, t2));
		return res;

		//:(
		//return Vector(v[0]+r.v[0], v[1]+r.v[1], v[2]+r.v[2]);
	}
	/// substraction
	inline const Vector operator -(const Vector & r) const {
		return Vector(v[0]-r.v[0], v[1]-r.v[1], v[2]-r.v[2]);
	}
	/// addition
	inline void operator +=(const Vector & r) {
		a[0] = __builtin_ia32_addpd(a[0], r.a[0]);
		v2df	i1 = __builtin_ia32_loadsd(v+2),
			i2 = __builtin_ia32_loadsd(r.v+2);
		__builtin_ia32_storesd(v+2, __builtin_ia32_addsd(i1, i2));

	}
	/// substraction
	inline void operator -=(const Vector & r) {
		a[0] = __builtin_ia32_subpd(a[0], r.a[0]);
		v2df	i1 = __builtin_ia32_loadsd(v+2),
			i2 = __builtin_ia32_loadsd(r.v+2);
		__builtin_ia32_storesd(v+2, __builtin_ia32_subsd(i1, i2));

	}

	/// dot product
	inline double operator * (const Vector &r) const {
		return v[0]*r.v[0]+v[1]*r.v[1]+v[2]*r.v[2];
	}
	/// multiplication by a scalar
	inline Vector operator *(double const & scale) const {
		Vector result;
		result.v[0] = v[0] * scale;
		result.v[1] = v[1] * scale;
		result.v[2] = v[2] * scale;
		return result;
	}
	/// cross product
	inline Vector operator ^ (const Vector & r) const {
		// some weirrrrd shuffles ;)
		Vector res;
		v2df 	t0 = a[0],
			t1 = __builtin_ia32_loadsd(v + 2),
			t2 = r.a[0],
			t3 = __builtin_ia32_loadsd(r.v + 2);

		v2df	s0 = __builtin_ia32_shufpd(t0, t1, 1),
			s1 = __builtin_ia32_shufpd(t3, t2, 0);
			t1 = __builtin_ia32_shufpd(t1, t0, 0);
		v2df	s3 = __builtin_ia32_shufpd(t2, t3, 1);

		v2df	p0 = __builtin_ia32_mulpd(s0, s1);
		v2df	p1 = __builtin_ia32_mulpd(t1, s3);

		res.a[0]   = __builtin_ia32_subpd(p0, p1);

			t0 = __builtin_ia32_mulsd(t0, s3);
			t2 = __builtin_ia32_mulsd(s0, t2);
			__builtin_ia32_storesd(res.v+2,
				__builtin_ia32_subsd(t0, t2));
		return res;
	}
	inline double operator [] (const unsigned index) const {
		return v[index];
	}
	inline double distto(const Vector & r) const {
		v2df	s0 = a[0];
			s0 = __builtin_ia32_subpd(s0, r.a[0]);
		v2df	s1 = __builtin_ia32_loadsd(v+2);
			s1 = __builtin_ia32_subsd(s1, r.a[1]);

			s0 = __builtin_ia32_mulpd(s0,s0);
			s1 = __builtin_ia32_mulsd(s1,s1);

		v2df	s2 = __builtin_ia32_shufpd(s0, s0, 1);
			s0 = __builtin_ia32_addsd(s0, s1);
			s0 = __builtin_ia32_addsd(s0, s2);

		double res;
			__builtin_ia32_storesd(&res, __builtin_ia32_sqrtsd(s0));
		return res;
	}

	void print() const {
		printf("(%.3lf, %.3lf, %.3lf)", v[0], v[1], v[2]);
	}
};

#endif // __SSE2__ && !__SSE3__


/**************************************************************************************************************************************/

//#if defined __SSE2__ && defined __SSE3__
#if 0
class Vector {
public:
	union {
		SSE_ALIGN(double v[4]);
		SSE_ALIGN(v2df a[2]);
	};
	Vector(){}
	inline Vector(double x, double y, double z) {
		v[0] = x;
		v[1] = y;
		v[2] = z;
		//v[3] = 0.0;
	}
	inline Vector(double const *data) {
		a[0] = *(v2df*)data;
		v[2] = data[2];
	}

	inline Vector(const Vector &r) {
		a[0] = r.a[0];
		v[2] = r.v[2];
	}
	inline Vector & operator = (const Vector & r) {
		a[0] = r.a[0];
		v[2] = r.v[2];
		return *this;
	}

	inline void make_vector(const Vector & dst, const Vector & src) {
		a[0] = __builtin_ia32_subpd(dst.a[0], src.a[0]);
		v2df	t0 = __builtin_ia32_loadsd(dst.v+2),
			t1 = __builtin_ia32_loadsd(src.v+2);
		__builtin_ia32_storesd(v+2, __builtin_ia32_subsd(t0, t1));
	}

	inline void add2(const Vector & x, const Vector & y) {
		a[0] = __builtin_ia32_addpd(x.a[0], y.a[0]);
		v2df	t0 = __builtin_ia32_loadsd(x.v+2),
			t1 = __builtin_ia32_loadsd(y.v+2);
		__builtin_ia32_storesd(v+2, __builtin_ia32_addsd(t0, t1));
	}

	inline void add3(const Vector & x, const Vector & y, const Vector & z) {
		v2df t = __builtin_ia32_addpd(x.a[0], y.a[0]);
		a[0]   = __builtin_ia32_addpd(t, z.a[0]);
		v2df	t0 = __builtin_ia32_loadsd(x.v+2),
			t1 = __builtin_ia32_loadsd(y.v+2),
			t2 = __builtin_ia32_loadsd(z.v+2);
		__builtin_ia32_storesd(v+2, __builtin_ia32_addsd(t2,
					    __builtin_ia32_addsd(t0, t1)));
	}

	inline void macc(const Vector & start, const Vector & ray, const double & scale) {
		// *this = start + ray * scale:
		v2df 	mm = __builtin_ia32_loadddup(&scale),
			rr = __builtin_ia32_loadsd(ray.v+2);
		v2df	st = __builtin_ia32_loadsd(start.v+2),
			t0 = __builtin_ia32_mulpd(ray.a[0], mm);
			rr = __builtin_ia32_mulsd(rr, mm);
		a[0] = __builtin_ia32_addpd(start.a[0], t0);
		__builtin_ia32_storesd(v + 2, __builtin_ia32_addsd(rr, st));
	}

	inline void make_length(const double & new_length) {
		v2df 	t0 = a[0],
			t1 = __builtin_ia32_loadsd(v+2);

		v2df	oo = __builtin_ia32_loadsd(&new_length);

		v2df	p0 = __builtin_ia32_mulpd(t0, t0),
			p1 = __builtin_ia32_mulsd(t1, t1);
			p0 = __builtin_ia32_haddpd(p0, p0);
			p0 = __builtin_ia32_addsd(p0, p1);
			p0 = __builtin_ia32_sqrtsd(p0);
			oo = __builtin_ia32_divsd(oo, p0);
			oo = __builtin_ia32_shufpd(oo, oo, 0);
			a[0] = __builtin_ia32_mulpd(t0, oo);
			__builtin_ia32_storesd(v+2, __builtin_ia32_mulsd(t1, oo));
	}

	/// normalizes a non-null vector to unitary
	inline void norm() {
		v2df 	t0 = a[0],
			t1 = __builtin_ia32_loadsd(v+2);

		const double one = 1.0;
		v2df	oo = __builtin_ia32_loadsd(&one);

		v2df	p0 = __builtin_ia32_mulpd(t0, t0),
			p1 = __builtin_ia32_mulsd(t1, t1);
			p0 = __builtin_ia32_haddpd(p0, p0);
			p0 = __builtin_ia32_addsd(p0, p1);
			p0 = __builtin_ia32_sqrtsd(p0);
			oo = __builtin_ia32_divsd(oo, p0);
			oo = __builtin_ia32_shufpd(oo, oo, 0);
			a[0] = __builtin_ia32_mulpd(t0, oo);
			__builtin_ia32_storesd(v+2, __builtin_ia32_mulsd(t1, oo));
	}
	/// nullifies a vector
	inline void zero() {
		a[0] = (v2df) __builtin_ia32_setzeropd();
		a[1] = (v2df) __builtin_ia32_setzeropd();
	}
	/// returns the length of the vector
	inline double lengthSqr() const {
		double result;
		v2df	t0 = __builtin_ia32_mulpd(a[0], a[0]),
			t1 = __builtin_ia32_loadsd(v+2);
			t0 = __builtin_ia32_haddpd(t0, t0);
			t1 = __builtin_ia32_mulsd(t1,t1);
		__builtin_ia32_storesd(& result, __builtin_ia32_addsd(t0,t1));
		return result;
	}
	/// returns the squared length
	inline double length() const {
			v2df	tmp0 = __builtin_ia32_mulpd(a[0],a[0]),
			tmp2 = __builtin_ia32_loadsd(v + 2);
			tmp2 = __builtin_ia32_mulsd(tmp2, tmp2);
			tmp0 = __builtin_ia32_haddpd(tmp0, tmp0);
			tmp0 = __builtin_ia32_addsd(tmp0, tmp2);
			tmp0 = __builtin_ia32_sqrtsd(tmp0);
			double result;
			__builtin_ia32_storesd(&result, tmp0);
		return result;
	}
	/// multiplies by a scalar
	inline Vector & scale(double const & scale) {
		v2df 	mm = __builtin_ia32_loadddup(&scale);
		a[0] = __builtin_ia32_mulpd(a[0], mm);
		a[1] = __builtin_ia32_mulsd(a[1], mm);
		return *this;
	}

	/// addition
	inline const Vector operator +(const Vector & r) const
	{
		Vector res;
		res.a[0] = __builtin_ia32_addpd(a[0], r.a[0]);
		v2df	t1 = __builtin_ia32_loadsd(v+2),
			t2 = __builtin_ia32_loadsd(r.v+2);
			__builtin_ia32_storesd(res.v+2,__builtin_ia32_addsd(t1, t2));
		return res;

	}
	/// substraction
	inline const Vector operator -(const Vector & r) const {
		return Vector(v[0]-r.v[0], v[1]-r.v[1], v[2]-r.v[2]);
	}
	/// addition
	inline void operator +=(const Vector & r) {
		a[0] = __builtin_ia32_addpd(a[0], r.a[0]);
		v2df	i1 = __builtin_ia32_loadsd(v+2),
			i2 = __builtin_ia32_loadsd(r.v+2);
		__builtin_ia32_storesd(v+2, __builtin_ia32_addsd(i1, i2));
	}
	/// substraction
	inline void operator -=(const Vector & r) {
		a[0] = __builtin_ia32_subpd(a[0], r.a[0]);
		v2df	i1 = __builtin_ia32_loadsd(v+2),
			i2 = __builtin_ia32_loadsd(r.v+2);
		__builtin_ia32_storesd(v+2, __builtin_ia32_subsd(i1, i2));
	}

	/// dot product
	inline double operator * (const Vector &r) const {
		return v[0]*r.v[0]+v[1]*r.v[1]+v[2]*r.v[2];
	}
	/// multiplication by a scalar
	inline Vector operator *(double const & scale) const {
		Vector result;
		result.a[0] = __builtin_ia32_mulpd(a[0], __builtin_ia32_loadddup(&scale));
		result.v[2] = v[2] * scale;
		return result;
	}
	/// cross product
	inline Vector operator ^ (const Vector & r) const {
		// some weirrrrd shuffles ;)
		Vector res;
		v2df 	t0 = a[0],
			t1 = __builtin_ia32_loadsd(v + 2),
			t2 = r.a[0],
			t3 = __builtin_ia32_loadsd(r.v + 2);

		v2df	s0 = __builtin_ia32_shufpd(t0, t1, 1),
			s1 = __builtin_ia32_shufpd(t3, t2, 0);
			t1 = __builtin_ia32_shufpd(t1, t0, 0);
		v2df	s3 = __builtin_ia32_shufpd(t2, t3, 1);

		v2df	p0 = __builtin_ia32_mulpd(s0, s1);
		v2df	p1 = __builtin_ia32_mulpd(t1, s3);

		res.a[0]   = __builtin_ia32_subpd(p0, p1);

			t0 = __builtin_ia32_mulsd(t0, s3);
			t2 = __builtin_ia32_mulsd(s0, t2);
			__builtin_ia32_storesd(res.v+2,
				__builtin_ia32_subsd(t0, t2));
		return res;
	}
	inline double operator [] (const unsigned index) const {
		return v[index];
	}
	inline double distto(const Vector & r) const {
		v2df	s0 = a[0];
			s0 = __builtin_ia32_subpd(s0, r.a[0]);
		v2df	s1 = __builtin_ia32_loadsd(v+2);
			s1 = __builtin_ia32_subsd(s1, r.a[1]);

			s0 = __builtin_ia32_mulpd(s0,s0);
			s1 = __builtin_ia32_mulsd(s1,s1);

			s0 = __builtin_ia32_haddpd(s0, s0);
			s0 = __builtin_ia32_addsd(s0, s1);

		double res;
			__builtin_ia32_storesd(&res, __builtin_ia32_sqrtsd(s0));
		return res;
	}

	void print() const {
		printf("(%.3lf, %.3lf, %.3lf)", v[0], v[1], v[2]);
	}
};
#endif // __SSE2__ && __SSE3__


#endif // __VECTOR3_H__
