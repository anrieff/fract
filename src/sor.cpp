/***************************************************************************
 *   Copyright (C) 2007 by Veselin Georgiev                                *
 *   anrieff@mailg.com (convert to gmail)                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 * ----------------------------------------------------------------------- *
 * Support for SOR (Surface of revolution) object type.                    *
 *                                                                         *
 ***************************************************************************/

#include "MyGlobal.h"
#include "sor.h"
#include "array.h"
#include "cvars.h"
#include "cvarlist.h"
#include "vector3.h"
#include "vector2f.h"
#include "vectormath.h"
#include "gfx.h"
#include "antialias.h"

Sor sor[MAX_SORS];
int sorcount = 0;

struct SorIntersection {
	double dist;
	Vector normal;
	Vector ip;
};

/** @class CubicSpline */
CubicSpline::CubicSpline()
{
	coeffs = x = NULL;
	n = 0;
}
	
CubicSpline::~CubicSpline()
{
	if (x) { free(x); x = NULL; }
	if (coeffs) { free(coeffs); coeffs = NULL; }
}

double CubicSpline::eval(double X)
{
	if (X < x[1]) X = x[1];
	if (X > x[n-1]) X = x[n-1];
	int i = 2;
	while (x[i] < X) i++;
	i--;
	return (((coeffs[4*i]*X) + coeffs[4*i+1])*X + coeffs[4*i+2]) * X + coeffs[4*i+3];
}

bool CubicSpline::in_range(double X)
{
	return X >= x[1] && X <= x[n-1];
}

double CubicSpline::eval(int i, double X)
{
	return (((coeffs[4*i]*X) + coeffs[4*i+1])*X + coeffs[4*i+2]) * X + coeffs[4*i+3];
}

double CubicSpline::eval_deriv(double X)
{
	if (X < x[1]) X = x[1];
	if (X > x[n-1]) X = x[n-1];
	int i = 2;
	while (x[i] < X) i++;
	i--;
	return (((3 * coeffs[4*i] * X) + 2 * coeffs[4*i+1]) * X) + coeffs[4*i+2];
}

double CubicSpline::max_radius(double begin, double end)
{
	// This calculates the max value of the spline. We can do this using
	// local extrema finding methods, but for the sake of simplicity, we
	// just sample the spline at 10000 points and return the largest.
	double r = 0;
	for (int i = 0; i <= 10000; i++) {
		double f = i / 10000.0;
		double X = x[1] + (x[n-1] - x[1]) *f;
		if (X > begin && X < end) {
			double t = eval(X);
			//t = sqrt(t);
			if (t > r) r = t;
		}
	}
	return r;
}

/** Misc intersection helpers */
static inline double cubeeval(double a, double b, double c, double d, double x)
{
	return ((((a * x) + b) * x + c) * x + d);
}

const double r3 = 0.333333333333333333, r9 = 0.11111111111111111111111,
	r27 = 0.037037037037037037, r04 = 2.5, r54 = 0.0185185185185185185;
	


#define CBRT2 1.2599210498948731648             /* 2^(1/3) */
#define SQR_CBRT2 1.5874010519681994748         /* 2^(2/3) */

static const float factor[5] =
{
  1.0f / SQR_CBRT2,
  1.0f / CBRT2,
  1.0f,
  CBRT2,
  SQR_CBRT2
};


// Copied from GNU Libc
static float
cube_root (float x)
{
  float xm, ym, u, t2;
  int xe;

  // handle special cases
  if (!finite(x) || x == 0) return x;

  /* Reduce X.  XM now is an range 1.0 to 0.5.  */
  xm = frexpf (fabsf (x), &xe);

  u = (0.492659620528969547f + (0.697570460207922770f
                               - 0.191502161678719066f * xm) * xm);

  t2 = u * u * u;

  ym = u * (t2 + 2.0f * xm) / (2.0f * t2 + xm) * factor[2 + xe % 3];

  return ldexpf (x > 0.0f ? ym : -ym, xe / 3);
}

// Copied from the GNU scientific library
static int cubic_solve_gsl(double a, double b, double c, double *X)
{
  double q = (a * a - 3 * b);
  double r = (2 * a * a * a - 9 * a * b + 27 * c);

  double Q = q * r9;
  double R = r * r54;

  double Q3 = Q * Q * Q;
  double R2 = R * R;

  double CR2 = 729 * r * r;
  double CQ3 = 2916 * q * q * q;
  double ar3 = a*r3;

  if (R == 0 && Q == 0)
    {
      X[0] = - ar3 ;
      return 1 ;
    }
  else if (CR2 == CQ3) 
    {
      /* this test is actually R2 == Q3, written in a form suitable
         for exact computation with integers */

      /* Due to finite precision some double roots may be missed, and
         considered to be a pair of complex roots z = x +/- epsilon i
         close to the real axis. */

      double sqrtQ = sqrt (Q);

      if (R > 0)
        {
          X[0] = -2 * sqrtQ  - ar3;
          X[1] = sqrtQ - ar3;
        }
      else
        {
          X[0] = - sqrtQ  - ar3;
          X[1] = 2 * sqrtQ - ar3;
        }
      return 2 ;
    }
  else if (CR2 < CQ3) /* equivalent to R2 < Q3 */
    {
      double sqrtQ = sqrt (Q);
      double sqrtQ3 = sqrtQ * sqrtQ * sqrtQ;
      double theta = acos (R / sqrtQ3);
      double norm = -2 * sqrtQ;
      X[0] = norm * cos (theta * r3) - ar3;
      X[1] = norm * cos ((theta + 2.0 * M_PI) * r3) - ar3;
      X[2] = norm * cos ((theta - 2.0 * M_PI) * r3) - ar3;
      return 3;
    }
  else
    {
      double sgnR = (R >= 0 ? 1 : -1);
      double A = -sgnR * cube_root (fabs (R) + sqrt (R2 - Q3));
      double B = Q / A ;
      X[0] = A + B - ar3;
      return 1;
    }
}


static int cubic_solve(double A, double B, double C, double D, double x[])
{
	if (A == 0) {
		double a = B;
		double b = C;
		double c = D;
		double D = b*b - 4 * a * c;
		if (D < 0) return 0;
		D = sqrt(D);
		double ra = 0.5 / a;
		x[0] = (-b - D) * ra;
		x[1] = (-b + D) * ra;
		return 2;
	}
	if (A != 1.0) {
		double rA = 1.0 /A;
		B *= rA;
		C *= rA;
		D *= rA;
	}
	return cubic_solve_gsl(B, C, D, x);
}


/** @class Sor */

Sor::Sor()
{
	spline_count = 0;
	spline = NULL;
	movement.zero();
	scale = 1;
	miny = maxy = 0;
	max_radius = 0;
	color = 0;
	refl = opacity = 0;
	texture = NULL;
}

Sor::~Sor()
{
	if (spline_count && spline) {
		delete [] spline;
		spline = NULL; spline_count = 0;
	}
	if (texture) {
		delete texture;
		texture = NULL;
	}
	//
}

bool Sor::read_from_file(const char *fn)
{
	FILE *f = fopen(fn, "rt");
	if (!f) return false;
	fscanf(f, "%lf%lf", &start, &end);
	Array <Array<double> > a;
	double x;
	while (1 == fscanf(f, "%lf", &x)) {
		int n = (int) x;
		Array<double> b;
		b += x;
		int all = n-1 + 4*(n-2);
		for (int i = 0; i < all; i++) {
			if (1 != fscanf(f, "%lf", &x)) {
				fclose(f);
				return false;
			}
			b += x;
		}
		a += b;
	}
	fclose(f);
	//
	this->spline_count = a.size();
	this->spline = new CubicSpline[a.size()];
	max_radius = 0;
	for (int l = 0; l < spline_count; l++) {
		CubicSpline &s = spline[l];
		Array<double> &b = a[l];
		int n = (int) b[0];
		s.n = n;
		s.x = new double[n];
		s.coeffs = new double[4*n];
		int k = 1;
		for (int i = 1; i < n; i++) {
			if (k >= b.size()) return false;
			s.x[i] = b[k++];
		}
		for (int i = 1; i < n - 1; i++) {
			for (int j = 0; j < 4; j++) {
				if (k >= b.size()) return false;
				s.coeffs[i*4+j] = b[k++];
			}
		}
		double md = s.max_radius(start, end);
		if (md > max_radius) max_radius = md;
	}
	max_radius = sqrt(max_radius);
	return true;
}

void Sor::move(Vector xmovement)
{
	movement += xmovement;
}

void Sor::set_scale(double scaler)
{
	scale *= scaler;
}

void Sor::set_reflectivity(float f)
{
	refl = f;
}

void Sor::set_opacity(float f)
{
	opacity = f;
}

void Sor::set_color(int value, char which)
{
	switch (which) {
		case 'r':
			color &= 0x00ffff;
			color |= (value << 16);
			break;
		case 'g':
			color &= 0xff00ff;
			color |= (value << 8);
			break;
		case 'b':
			color &= 0xffff00;
			color |= value;
			break;
		default:
			break;
	}
}

bool Sor::set_texture(const char * texture_file_name)
{
	texture = new RawImg;
	return texture->load_bmp(texture_file_name);
}

void Sor::perframe_init(void)
{
	bbox.recalc();
	miny = start * scale;
	maxy = end * scale;
	miny += movement[1];
	maxy += movement[1];
	double r = max_radius * scale;
	bbox.add(movement + Vector(-r, miny, -r));
	bbox.add(movement + Vector(+r, maxy, +r));
}

double Sor::get_depth(const Vector &camera)
{
	Vector temp = movement;
	temp.v[1] += (miny + maxy) * 0.5;
	return temp.distto(camera);
}

bool Sor::is_visible(const Vector & camera, Vector w[3])
{
	return true;
}

int Sor::calculate_convex(Vector output[], const Vector& camera)
{
	struct pt {
		Vector v;
		vec2f f;
		bool operator < (const pt &rhs) const { return f[1] < rhs.f[1]; }
	};
	Vector w[8];
	calc_grid_basics(camera, CVars::alpha, CVars::beta, w);
	int xr = xsize_render(xres());
	int yr = ysize_render(yres());
	pt a[8];
	int count = 0;
	for (int k = 0; k < 8; k++) {
		Vector v;
		for (int i = 0; i < 3; i++) {
			v[i] = ((k&(1<<i))==0) ? bbox.vmin[i] : bbox.vmax[i];
		}
		float x, y;
		if (project_point(&x, &y, v, camera, w, xr, yr) < 4) {
			a[count].v = v;
			a[count++].f = vec2f(x, y); 
		}
	}
	if (count < 3) return 0;
	Array<vec2f> b;
	for (int i = 0; i < count; i++)
		b += a[i].f;
	Array<vec2f> c = convex_hull(b);
	for (int i = 0; i < c.size(); i++) {
		for (int j = 0; j < count; j++) {
			if (c[i] == a[j].f) {
				output[c.size()-1-i] = a[j].v;
				break;
			}
		}
	}
	return c.size();
}

void Sor::map2screen(Uint32 *framebuffer, int color, int sides, Vector pt[], const Vector& camera, Vector w[3], int & min_y, int & max_y)
{
	if (sides < 3) return;
	int L[RES_MAXY], R[RES_MAXY];
	int ys, ye, bs, be;
	min_y = ys = accumulate(pt, sides, camera, w, fun_less, 999666111, bs);
	max_y = ye = accumulate(pt, sides, camera, w, fun_more,-999666111, be);
	int size = (be + sides - bs) % sides;
	project_hull_part(L, pt, bs, +1, size, sides, color, camera, w, fun_min);
	project_hull_part(R, pt, bs, -1, sides - size, sides, color, camera, w, fun_max);
	map_hull(framebuffer, L, R, ys, ye, color);
}

bool Sor::fastintersect(const Vector& xray, const Vector& camera, const double& rls, void *IntersectContext) const
{
	if (!bbox.testintersect(camera, xray)) return false;
	SorIntersection& is = *(static_cast<SorIntersection*>(IntersectContext));
	//
	Vector ray;
	if (rls != 1) {
		ray = xray * (1.0/sqrt(rls));
	} else {
		ray = xray;
	}
	//
	double R2 = max_radius*max_radius*scale*scale;
	double hx = movement[0] - camera[0];
	double hy = movement[2] - camera[2];
	double A = sqr(ray[0]) + sqr(ray[2]);
	double B = -2 * (hx * ray[0] + hy * ray[2]);
	double C = sqr(hx) + sqr(hy);
	double r2A = 0.5 / A;
	
	// distance from sor's center is A*p^2 + B * p + C. The closest point is -b/2a
	double X = -B * r2A;
	double cd = A*X*X+B*X+C;
	if (cd > R2 || (X < 0 && C > R2)) return false; // it doesn't cross the bounding cyl - bail out
	
	double scaling = scale;
	double scale2 = sqr(scaling);
	
	C -= R2;
	double D = B * B - 4 * A * C;
	if (D < 0) return true;
	D = sqrt(D);
	double x1 = ( -B - D) * r2A;
	double x2 = ( -B + D) * r2A;
	Vector p1 = camera + ray * x1;
	Vector p2 = camera + ray * x2;
	if ((p1[1] < miny  && p2[1] < miny) || (p1[1] > maxy && p2[1] > maxy)) {
		return false;
	}
	C += R2;
	//
	double Y1 = movement[1];
	double Y0 = movement[1]+scale;
	double mint, maxt, dt, minAbsolute, maxAbsolute;
	bool extremal_case = false;
	double rd1;
	if (fabs(ray[1]) < 5*1e-3) {
		minAbsolute = mint = -1e99;
		maxAbsolute = maxt = 1e99;
		dt = 1e99;
		extremal_case = true;
		if (camera[1] < miny - 1e-2 || camera[1] > maxy + 1e-2)
			return false;
	} else {
		rd1 = 1.0 / ray[1];
		mint = (Y1 - camera[1]) * rd1;
		maxt = (Y0 - camera[1]) * rd1;
		minAbsolute = (miny - camera[1]) * rd1;
		maxAbsolute = (maxy - camera[1]) * rd1;
		dt = maxt - mint;
	}
	double rmincmp, rmaxcmp;
	if (minAbsolute < maxAbsolute) {
		rmincmp = minAbsolute;
		rmaxcmp = maxAbsolute;
	} else {
		rmincmp = maxAbsolute;
		rmaxcmp = minAbsolute;
	}
	
	
	
	double min_p = 1e99;
	double deriv = 0;
	CubicSpline &spline = this->spline[0];
	if (extremal_case) {
		double x = (camera[1] - Y1) / (Y0-Y1);
		double off = spline.eval(x);
		C -= off * scale2;
		D = B * B - 4 * A * C;
		if (D < 0) return false;
		D = sqrt(D);
		min_p = (- B - D) / (2 * A);
		if (off < 1e-9)
			deriv = 1e99;
		else
			deriv = 0.5 * spline.eval_deriv(x) / sqrt(off);
	} else {
		for (int i = 1; i < spline.n - 1; i++) {
			double U, V, W, T;
			U = -spline.coeffs[i*4+0]*scale2;
			V = -spline.coeffs[i*4+1]*scale2;
			W = -spline.coeffs[i*4+2]*scale2;
			T = -spline.coeffs[i*4+3]*scale2;
			U += 0;
			V += A * dt * dt;
			W += 2 * A * dt * mint + B * dt;
			T += A * mint * mint + B * mint + C;
			
			double sols[3];
			int sc = cubic_solve(U, V, W, T, sols);
			for (int j = 0; j < sc; j++) {
				if (sols[j] < spline.x[i] || sols[j] > spline.x[i+1]) continue;
				double p = (maxt-mint)*sols[j] + mint;
				if (p < min_p && p > 0 && p >= rmincmp && p <= rmaxcmp) {
					min_p = p;
					double off = spline.eval(i, sols[j]);
					if (off < 1e-9)
						deriv = 1e99;
					else
						deriv = 0.5 * spline.eval_deriv(sols[j]) / sqrt(off);
				}
			}
		}
	}
	
	if (min_p < 1e99) {
		is.ip = camera + ray * min_p;
		Vector & normal = is.normal;
		normal = is.ip - movement;
		
		normal[1] = 0;
		normal.norm();
		
		double co = sqrt(1.0 / (deriv * deriv + 1.0));
		double si = sqrt(1.0 - co * co);
		normal = normal * co + Vector(0, copysign(si, -deriv), 0);
		
		is.dist = is.ip.distto(camera);
		return true;
	}
	return false;
}

bool Sor::intersect(const Vector& ray, const Vector &camera, void *IntersectContext)
{
	return fastintersect(ray, camera, 1, IntersectContext);
}

double Sor::intersection_dist(void *IntersectContext) const
{
	SorIntersection* is = static_cast<SorIntersection*>(IntersectContext);
	return is->dist;
}

Uint32 Sor::shade(Vector& xray, const Vector& C, const Vector& L, double rr, float &o, void *ic, int iteration, FilteringInfo& finfo)
{
	o = 1.0f;
	SorIntersection &is = *(static_cast<SorIntersection*>(ic));
	/*
	Vector n;
	n = is.normal * 255.0;
	double x = n[1];
	if (x < 0) {
		return ((int) -x) << 16;
	} else {
		return ((int) x) << 8;
	}
	//
	*/
	//
	Uint32 color = this->color;
	if ((flags & MAPPED) && texture) {
		float tx, ty;
		ty = (is.ip[1] - miny) / (maxy-miny);
		tx = (atan2(is.ip[2] - movement[2], is.ip[0] - movement[0]) + M_PI) / (2*M_PI);
		int itx = (int) (tx * texture->get_x());
		int ity = (int) (ty * texture->get_y());
		if (itx < 0) itx = 0;
		if (ity < 0) ity = 0;
		if (itx >= texture->get_x()) itx = texture->get_x()-1;
		if (ity >= texture->get_y()) ity = texture->get_y()-1;
		color = ((Uint32*)(texture->get_data()))[itx + ity* texture->get_x()];
	}
	o = 1.0f;
	Vector ray;
	if (rr != 1) {
		ray = xray * (1.0 / rr);
	} else ray = xray;
	float m = ambient;
	//
	Vector IL;
	IL.make_vector(L, is.ip);
	IL.norm();
	float d = IL * is.normal;
	if (d > 0) {
		m += d * diffuse;
	}
	Vector VI;
	VI.make_vector(is.ip, C);
	VI.norm();
	VI += is.normal * 2.0;
	VI.norm();
	float sp = VI * IL;
	if (sp > 0) {
		sp *= sp;
		sp *= sp;
		sp *= sp;
		sp *= sp;
		sp *= sp;
		sp *= sp;
		m += sp * 3.0f;
	}
	if ((flags & RAYTRACED) && refl > 0.0f) {
		finfo.lectionType = REFLECTION;
		Uint32 reflected = Raytrace(is.ip, VI, (flags & RECURSIVE) | RAYTRACE_BILINEAR_MASK, iteration + 1, this, finfo, 0.0f);
		color = combine(color, m*(1-refl), reflected, refl);
	} else {
		color = multiplycolorf(color, m);
	}
	return color;
}

int Sor::get_best_miplevel(double x0, double z0, FilteringInfo & fi)
{
	return 1;
}

OBTYPE Sor::get_type(bool generic) const
{
	return OB_SOR;
}


bool Sor::sintersect(const Vector & start, const Vector & dir, int opt)
{
	return false;
}
