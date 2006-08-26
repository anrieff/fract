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
#ifndef __VECTOR3_FRACT_H__
#define __VECTOR3_FRACT_H__

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
 * @method inverse   - component-wise 1/v[i]
*/
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

#if !defined SIMD_VECTOR
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
	inline void inverse(void);
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

inline void Vector::inverse(void)
{
	if (fabs(v[0]) >= 1e-12) v[0] = 1.0/v[0];
	if (fabs(v[1]) >= 1e-12) v[1] = 1.0/v[1];
	if (fabs(v[2]) >= 1e-12) v[2] = 1.0/v[2];
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

#endif // Non-sse2 version

#define VECTOR3_ASM
#include "x86_asm.h"
#undef VECTOR3_ASM

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


/**************************************************************************************************************************************/

#if defined SIMD_VECTOR && defined __SSE2__ && !defined __SSE3__
#include <emmintrin.h>
#define vdd __m128d

#ifdef _MSC_VER
__declspec(align(16))
#endif
class Vector {
public:
	union {
		double v[4];
		vdd a[2];
	};
	void align_test()
	{
		if ((reinterpret_cast<long long>(this)) & 0xf) {
			printf("A Vector does not have the correct alignment!!\n");
			*(int*)0 = 1;
		}
	}
#ifdef DEBUG
#define ALIGN_TEST() align_test()
#else
#define ALIGN_TEST()
#endif
	Vector(){ ALIGN_TEST(); }
	inline Vector(double x, double y, double z) {
		ALIGN_TEST();
		v[0] = x;
		v[1] = y;
		v[2] = z;
	}
	inline Vector(double const *data) {
		ALIGN_TEST();
		v[0] = data[0];
		v[1] = data[1];
		v[2] = data[2];
	}

	inline Vector(const Vector &r) {
		ALIGN_TEST();
		a[0] = r.a[0];
		v[2] = r.v[2];
	}
	inline Vector & operator = (const Vector & r) {
		ALIGN_TEST();
		a[0] = r.a[0];
		v[2] = r.v[2];
		return *this;
	}

	inline void make_vector(const Vector & dst, const Vector & src) {
		a[0] = _mm_sub_pd(dst.a[0], src.a[0]);
		v[2] = dst.v[2] - src.v[2];
	}

	inline void add2(const Vector & x, const Vector & y) {
		a[0] = _mm_add_pd(x.a[0], y.a[0]);
		v[2] = x.v[2] + y.v[2];
	}

	inline void add3(const Vector & x, const Vector & y, const Vector & z) {
		vdd    t = _mm_add_pd(x.a[0], y.a[0]);
		double u = x.v[2] + y.v[2];
		a[0]     = _mm_add_pd(t, z.a[0]);
		v[2]     = u + z.v[2];
	}

	inline void macc(const Vector & start, const Vector & ray, const double & scale) {
		// *this = start + ray * scale:
		vdd 	mm = _mm_load1_pd(&scale),
			rr = _mm_load_sd(ray.v+2);
		vdd	st = _mm_load_sd(start.v+2),
			t0 = _mm_mul_pd(ray.a[0], mm);
			rr = _mm_mul_sd(rr, mm);
		a[0] = _mm_add_pd(start.a[0], t0);
		_mm_store_sd(v + 2, _mm_add_sd(rr, st));
	}

	inline void make_length(const double & new_length) {
		vdd 	t0 = a[0],
			t1 = _mm_load_sd(v+2);

		vdd	oo = _mm_load_sd(&new_length);

		vdd	p0 = _mm_mul_pd(t0, t0),
			p1 = _mm_mul_sd(t1, t1);
		vdd	p2 = _mm_shuffle_pd(p0, p0, 1);
			p0 = _mm_add_sd(p0, p1);
			p0 = _mm_add_sd(p0, p2);
			p0 = _mm_sqrt_sd(p0, p0);
			oo = _mm_div_sd(oo, p0);
			oo = _mm_shuffle_pd(oo,oo,0);
			a[0] = _mm_mul_pd(t0, oo);
			_mm_store_sd(v+2, _mm_mul_sd(t1, oo));
	}

	/// normalizes a non-null vector to unitary
	inline void norm() {
		vdd 	t0 = a[0],
			t1 = _mm_load_sd(v+2);

		const double one = 1.0;
		vdd	oo = _mm_load_sd(&one);

		vdd	p0 = _mm_mul_pd(t0, t0),
			p1 = _mm_mul_sd(t1, t1);
		vdd	p2 = _mm_shuffle_pd(p0, p0, 1);
			p0 = _mm_add_sd(p0, p1);
			p0 = _mm_add_sd(p0, p2);
			p0 = _mm_sqrt_sd(p0, p0);
			oo = _mm_div_sd(oo, p0);
			oo = _mm_shuffle_pd(oo,oo,0);
			a[0] = _mm_mul_pd(t0, oo);
			_mm_store_sd(v+2, _mm_mul_sd(t1, oo));
	}
	/// nullifies a vector
	inline void zero() {
		a[0] = (vdd) _mm_setzero_pd();
		a[1] = (vdd) _mm_setzero_pd();
	}
	/// returns the length of the vector
	inline double lengthSqr() const {
		return v[0]*v[0]+v[1]*v[1]+v[2]*v[2];
	}
	/// returns the squared length
	inline double length() const {
			vdd	tmp0 = _mm_load_sd(v    ),
			tmp1 = _mm_load_sd(v + 1),
			tmp2 = _mm_load_sd(v + 2);
			tmp0 = _mm_mul_sd(tmp0, tmp0);
			tmp1 = _mm_mul_sd(tmp1, tmp1);
			tmp2 = _mm_mul_sd(tmp2, tmp2);
			tmp0 = _mm_add_sd(tmp0, tmp1);
			tmp0 = _mm_add_sd(tmp0, tmp2);
			tmp0 = _mm_sqrt_sd(tmp0, tmp0);
			double result;
			_mm_store_sd(&result, tmp0);
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
		res.a[0] = _mm_add_pd(a[0], r.a[0]);
		res.v[2] = v[2] + r.v[2];
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
		a[0] = _mm_add_pd(a[0], r.a[0]);
		v[2] += r.v[2];
	}
	/// substraction
	inline void operator -=(const Vector & r) {
		a[0] = _mm_sub_pd(a[0], r.a[0]);
		v[2] -= r.v[2];

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
		vdd 	t0 = a[0],
			t1 = _mm_load_sd(v + 2),
			t2 = r.a[0],
			t3 = _mm_load_sd(r.v + 2);

		vdd	s0 = _mm_shuffle_pd(t0, t1, 1),
			s1 = _mm_shuffle_pd(t3, t2, 0);
			t1 = _mm_shuffle_pd(t1, t0, 0);
		vdd	s3 = _mm_shuffle_pd(t2, t3, 1);

		vdd	p0 = _mm_mul_pd(s0, s1);
		vdd	p1 = _mm_mul_pd(t1, s3);

		res.a[0]   = _mm_sub_pd(p0, p1);

			t0 = _mm_mul_sd(t0, s3);
			t2 = _mm_mul_sd(s0, t2);
			_mm_store_sd(res.v+2,
				_mm_sub_sd(t0, t2));
		return res;
	}
	inline double& operator [] (const unsigned index) {
		return v[index];
	}
	inline const double & operator [] (const unsigned index) const {
		return v[index];
	}
	inline double distto(const Vector & r) const {
		vdd	s0 = a[0];
			s0 = _mm_sub_pd(s0, r.a[0]);
		vdd	s1 = _mm_load_sd(v+2);
			s1 = _mm_sub_sd(s1, r.a[1]);

			s0 = _mm_mul_pd(s0,s0);
			s1 = _mm_mul_sd(s1,s1);

		vdd	s2 = _mm_shuffle_pd(s0, s0, 1);
			s0 = _mm_add_sd(s0, s1);
			s0 = _mm_add_sd(s0, s2);

		double res;
			_mm_store_sd(&res, _mm_sqrt_sd(s0, s0));
		return res;
	}
	
	inline void inverse(void) {
		const double double_one = 1.0;
		vdd one = _mm_load1_pd(&double_one);
		a[0] = _mm_div_pd(a[0], one);
		v[2] = double_one / v[2];
	}
	
#define equal(a,b,c) (fabs((a)-(b))<(c))

	inline bool like(const Vector &r, double compare_factor = 1e-9) const {
		return 	equal(v[0], r.v[0], compare_factor) && 
			equal(v[1], r.v[1], compare_factor) && 
			equal(v[2], r.v[2], compare_factor);
	}
	
	inline bool operator == (const Vector & r) const {
		return v[0] == r.v[0] && v[1] == r.v[1] && v[2] == r.v[2];
	}
	
	inline bool operator != (const Vector & r) const {
		return v[0] != r.v[0] || v[1] != r.v[1] || v[2] != r.v[2];
	}

	void print() const {
		printf("(%.3lf, %.3lf, %.3lf)", v[0], v[1], v[2]);
	}

	void * operator new (size_t size);
	void operator delete(void * what);
	void * operator new[] (size_t size);
	void operator delete[](void * what);
}
#ifdef __GNUC__
__attribute__ ((aligned(16)))
#endif
;

#endif // __SSE2__ && !__SSE3__


/**************************************************************************************************************************************/

#if defined SIMD_VECTOR && defined __SSE2__ && defined __SSE3__
#include <emmintrin.h>
#include <pmmintrin.h>
#define vdd __m128d

#ifdef _MSC_VER
__declspec(align(16))
#endif
class Vector {
public:
	union {
		double v[4];
		vdd a[2];
	};
	Vector(){}
	inline Vector(double x, double y, double z) {
		v[0] = x;
		v[1] = y;
		v[2] = z;
	}
	inline Vector(double const *data) {
		v[0] = data[0];
		v[1] = data[1];
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
		a[0] = _mm_sub_pd(dst.a[0], src.a[0]);
		vdd	t0 = _mm_load_sd(dst.v+2),
			t1 = _mm_load_sd(src.v+2);
		_mm_store_sd(v+2, _mm_sub_sd(t0, t1));
	}

	inline void add2(const Vector & x, const Vector & y) {
		a[0] = _mm_add_pd(x.a[0], y.a[0]);
		vdd	t0 = _mm_load_sd(x.v+2),
			t1 = _mm_load_sd(y.v+2);
		_mm_store_sd(v+2, _mm_add_sd(t0, t1));
	}

	inline void add3(const Vector & x, const Vector & y, const Vector & z) {
		vdd t = _mm_add_pd(x.a[0], y.a[0]);
		a[0]   = _mm_add_pd(t, z.a[0]);
		vdd	t0 = _mm_load_sd(x.v+2),
			t1 = _mm_load_sd(y.v+2),
			t2 = _mm_load_sd(z.v+2);
		_mm_store_sd(v+2, _mm_add_sd(t2,
					    _mm_add_sd(t0, t1)));
	}

	inline void macc(const Vector & start, const Vector & ray, const double & scale) {
		// *this = start + ray * scale:
		vdd 	mm = _mm_load1_pd(&scale),
			rr = _mm_load_sd(ray.v+2);
		vdd	st = _mm_load_sd(start.v+2),
			t0 = _mm_mul_pd(ray.a[0], mm);
			rr = _mm_mul_sd(rr, mm);
		a[0] = _mm_add_pd(start.a[0], t0);
		_mm_store_sd(v + 2, _mm_add_sd(rr, st));
	}

	inline void make_length(const double & new_length) {
		vdd 	t0 = a[0],
			t1 = _mm_load_sd(v+2);

		vdd	oo = _mm_load_sd(&new_length);

		vdd	p0 = _mm_mul_pd(t0, t0),
			p1 = _mm_mul_sd(t1, t1);
			p0 = _mm_hadd_pd(p0, p0);
			p0 = _mm_add_sd(p0, p1);
			p0 = _mm_sqrt_sd(p0, p0);
			oo = _mm_div_sd(oo, p0);
			oo = _mm_shuffle_pd(oo, oo, 0);
			a[0] = _mm_mul_pd(t0, oo);
			_mm_store_sd(v+2, _mm_mul_sd(t1, oo));
	}

	/// normalizes a non-null vector to unitary
	inline void norm() {
		vdd 	t0 = a[0],
			t1 = _mm_load_sd(v+2);

		const double one = 1.0;
		vdd	oo = _mm_load_sd(&one);

		vdd	p0 = _mm_mul_pd(t0, t0),
			p1 = _mm_mul_sd(t1, t1);
			p0 = _mm_hadd_pd(p0, p0);
			p0 = _mm_add_sd(p0, p1);
			p0 = _mm_sqrt_sd(p0, p0);
			oo = _mm_div_sd(oo, p0);
			oo = _mm_shuffle_pd(oo, oo, 0);
			a[0] = _mm_mul_pd(t0, oo);
			_mm_store_sd(v+2, _mm_mul_sd(t1, oo));
	}
	/// nullifies a vector
	inline void zero() {
		a[0] = (vdd) _mm_setzero_pd();
		a[1] = (vdd) _mm_setzero_pd();
	}
	/// returns the length of the vector
	inline double lengthSqr() const {
		double result;
		vdd	t0 = _mm_mul_pd(a[0], a[0]),
			t1 = _mm_load_sd(v+2);
			t0 = _mm_hadd_pd(t0, t0);
			t1 = _mm_mul_sd(t1,t1);
		_mm_store_sd(& result, _mm_add_sd(t0,t1));
		return result;
	}
	/// returns the squared length
	inline double length() const {
			vdd	tmp0 = _mm_mul_pd(a[0],a[0]),
			tmp2 = _mm_load_sd(v + 2);
			tmp2 = _mm_mul_sd(tmp2, tmp2);
			tmp0 = _mm_hadd_pd(tmp0, tmp0);
			tmp0 = _mm_add_sd(tmp0, tmp2);
			tmp0 = _mm_sqrt_sd(tmp0, tmp0);
			double result;
			_mm_store_sd(&result, tmp0);
		return result;
	}
	/// multiplies by a scalar
	inline Vector & scale(double const & scale) {
		vdd 	mm = _mm_load1_pd(&scale);
		a[0] = _mm_mul_pd(a[0], mm);
		a[1] = _mm_mul_sd(a[1], mm);
		return *this;
	}

	/// addition
	inline const Vector operator +(const Vector & r) const
	{
		Vector res;
		res.a[0] = _mm_add_pd(a[0], r.a[0]);
		vdd	t1 = _mm_load_sd(v+2),
			t2 = _mm_load_sd(r.v+2);
			_mm_store_sd(res.v+2,_mm_add_sd(t1, t2));
		return res;

	}
	/// substraction
	inline const Vector operator -(const Vector & r) const {
		return Vector(v[0]-r.v[0], v[1]-r.v[1], v[2]-r.v[2]);
	}
	/// addition
	inline void operator +=(const Vector & r) {
		a[0] = _mm_add_pd(a[0], r.a[0]);
		vdd	i1 = _mm_load_sd(v+2),
			i2 = _mm_load_sd(r.v+2);
		_mm_store_sd(v+2, _mm_add_sd(i1, i2));
	}
	/// substraction
	inline void operator -=(const Vector & r) {
		a[0] = _mm_sub_pd(a[0], r.a[0]);
		vdd	i1 = _mm_load_sd(v+2),
			i2 = _mm_load_sd(r.v+2);
		_mm_store_sd(v+2, _mm_sub_sd(i1, i2));
	}

	/// dot product
	inline double operator * (const Vector &r) const {
		return v[0]*r.v[0]+v[1]*r.v[1]+v[2]*r.v[2];
	}
	/// multiplication by a scalar
	inline Vector operator *(double const & scale) const {
		Vector result;
		result.a[0] = _mm_mul_pd(a[0], _mm_load1_pd(&scale));
		result.v[2] = v[2] * scale;
		return result;
	}
	/// cross product
	inline Vector operator ^ (const Vector & r) const {
		// some weirrrrd shuffles ;)
		Vector res;
		vdd 	t0 = a[0],
			t1 = _mm_load_sd(v + 2),
			t2 = r.a[0],
			t3 = _mm_load_sd(r.v + 2);

		vdd	s0 = _mm_shuffle_pd(t0, t1, 1),
			s1 = _mm_shuffle_pd(t3, t2, 0);
			t1 = _mm_shuffle_pd(t1, t0, 0);
		vdd	s3 = _mm_shuffle_pd(t2, t3, 1);

		vdd	p0 = _mm_mul_pd(s0, s1);
		vdd	p1 = _mm_mul_pd(t1, s3);

		res.a[0]   = _mm_sub_pd(p0, p1);

			t0 = _mm_mul_sd(t0, s3);
			t2 = _mm_mul_sd(s0, t2);
			_mm_store_sd(res.v+2,
				_mm_sub_sd(t0, t2));
		return res;
	}
	inline double operator [] (const unsigned index) const {
		return v[index];
	}
	inline double distto(const Vector & r) const {
		vdd	s0 = a[0];
			s0 = _mm_sub_pd(s0, r.a[0]);
		vdd	s1 = _mm_load_sd(v+2);
			s1 = _mm_sub_sd(s1, r.a[1]);

			s0 = _mm_mul_pd(s0,s0);
			s1 = _mm_mul_sd(s1,s1);

			s0 = _mm_hadd_pd(s0, s0);
			s0 = _mm_add_sd(s0, s1);

		double res;
			_mm_store_sd(&res, _mm_sqrt_sd(s0, s0));
		return res;
	}

	void print() const {
		printf("(%.3lf, %.3lf, %.3lf)", v[0], v[1], v[2]);
	}
}
#ifdef __GNUC__
__attribute__ ((aligned(16)))
#endif
;
#endif // __SSE2__ && __SSE3__

#endif // __VECTOR3_FRACT_H__
