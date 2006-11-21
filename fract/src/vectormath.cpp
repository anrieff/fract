/***************************************************************************
 *   Copyright (C) 2004 by Veselin Georgiev                                *
 *   vesko@ViruS                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *-------------------------------------------------------------------------*
 *                                                                         *
 *      VECTORMATH.CPP - Does anything related to vector calculation:      *
 *        - sphere pre-projection                                          *
 *        - phong shading                                                  *
 *        - physics engine                                                 *
 *                                                                         *
 ***************************************************************************/

#include "MyGlobal.h"
#include "MyTypes.h"
#include "common.h"
#include "fract.h"
#include "sphere.h"
#include "triangle.h"
#include "profile.h"
#include "gfx.h"
#include "mesh.h"
#include "vector3.h"
#include "vectormath.h"
#include "render.h"
#include "infinite_plane.h"
#include "voxel.h"
#include <math.h>

#ifdef _MSC_VER
// 1/Pi
#define M_1_PI 0.31830988618379067153776752674503
#endif
// this should be 45 degrees, or M_PI/4
#define A_ALPHA_OFFSET 0.78539816339745
// this should be 3/4 of A_ALPHA_OFFSET
#define A_BETA_OFFSET  0.58904862254809

//#define PRINT_COLLISIONS

#define AIRTHICKNESS_NORMAL 0.5
#define AIRTHICKNESS_HIGH   2.5
#define YI_THRESH 1

#define TIME_EPS 0.00025
// this should be the reciprocal of TIME_EPS
#define T_1_TIME_EPS 4000.0


// how much of the energy is preserved in a bounce (1 means that bounce takes no energy)
#define BOUNCE_COEFF 0.666

/*double ambient = 0.2;
double diffuse = 0.55;
double specular = 0.15;*/
extern Sphere sp[];
extern int spherecount;
extern double fov;
extern int vframe;
extern int ysqrd_floor, ysqrd_ceil;

int CollDetect = 1;
int Physics = 1;
double sv_gravity = 166.66;
double sv_air     = 3.99;

Vector lix;

unsigned char m0ving[MAX_SPHERES];



#define InitMatrix2(m) m[0][0]=xv[0], m[0][1]=xv[1], m[0][2]=xv[2], m[1][0]=yv[0], m[1][1]=yv[1], m[1][2]=yv[2], m[2][0]=zv[0], m[2][1]=zv[1], m[2][2]=zv[2]
#define InitMatrix3(m) m[0][0]=a[0], m[0][1]=b[0], m[0][2]=-c[0], m[1][0]=a[1], m[1][1]=b[1], m[1][2]=-c[1], m[2][0]=a[2], m[2][1]=b[2], m[2][2]=-c[2]

#define ReplaceCol(m, col, a) m[col][0]=a[0], m[col][1]=a[1], m[col][2]=a[2]
#define ReplaceCol1(m,col, a) m[0][col]=a[0], m[1][col]=a[1], m[2][col]=a[2]
#define v1(a, b, c) c[0]=a[0]-b[0], c[1]=a[1]-b[1], c[2]=a[2]-b[2]
#define v2(a, b, c) c[0]=a[0]+b[0], c[1]=a[1]+b[1], c[2]=a[2]+b[2]
#define limit1(a) (a>1?1:(a))
#define arccos(x) (atan2(fast_sqrt(1-(x)*(x)), x))
#define lift(x) ((x)*(x))
#define eq(a,b) (iabs((a)-(b))<EPS)


// calculates some grid point :
// INPUT:
// c -> camera position
// a - alpha of the point
// b - beta of the point
// alphaorig - alpha of the direction of the camera
// w - destination 3dpoint
void calc_grid_point(const Vector & c, double a, double b, double alphaorig, Vector & w)
{
 w = Vector(
 	c[0] + sin(a) + sin(alphaorig)*(cos(b)-1),
 	c[1] + sin(b),
 	c[2] + cos(a) + cos(alphaorig)*(cos(b)-1)
	);
}

// calculates the three ending points of the vector grid given a user viewpoint and a direction
void calc_grid_basics(const Vector &c, double alpha, double beta, Vector w[3])
{
 calc_grid_point(c, alpha-A_ALPHA_OFFSET*fov*FOV_CORRECTION, beta+A_BETA_OFFSET*fov*FOV_CORRECTION, alpha, w[0]);
 calc_grid_point(c, alpha+A_ALPHA_OFFSET*fov*FOV_CORRECTION, beta+A_BETA_OFFSET*fov*FOV_CORRECTION, alpha, w[1]);
 calc_grid_point(c, alpha-A_ALPHA_OFFSET*fov*FOV_CORRECTION, beta-A_BETA_OFFSET*fov*FOV_CORRECTION, alpha, w[2]);
}

void InitMatrix(double m[3][3], const Vector&a, const Vector &b, const Vector &c) 
{
	m[0][0]=a[0]; 
	m[1][0]=b[0]; 
	m[2][0]=-c[0];
	m[0][1]=0;
	m[1][1]=b[1];
	m[2][1]=-c[1];
	m[0][2]=a[2];
	m[1][2]=b[2];
	m[2][2]=-c[2];
}


// calculates the determinant of the 3*3 matrix M.
double MatrixRoot(double M[3][3])
{
 return (M[0][0]*M[1][1]*M[2][2] + M[1][0]*M[2][1]*M[0][2] + M[0][1]*M[1][2]*M[2][0]
        -M[0][2]*M[1][1]*M[2][0] - M[2][2]*M[0][1]*M[1][0] - M[1][2]*M[2][1]*M[0][0]);
}

// determines where the center of the given sphere will map on the 2d screen
// works correctly with any FOV, X/Y ratio, etc.
// returns 0 if the center will be wildly of the screen (most probably when the sphere will not render at all)
int  project_sphere(Sphere *d, const Vector & cur, const Vector w[3], int *dx, int *dy, int xres, int yres)
{
 Vector a, b, c, h;
 double m[3][3];
 double Dcr;
 double beta, gamma, delta;

 // init the a, b, c & h vectors
 h = cur - w[0];
 a = w[1] - w[0];
 b = w[2] - w[0];
 c.make_vector(d->pos, cur);

 InitMatrix(m, a, b, c);
 Dcr = MatrixRoot(m);
 if (iabs(Dcr)<0.01) {
 	//printf("Negative Determinant - ");
 	return 0; // sphere is inprojectable - zero determinant
	}

 // solve the ugly equation
 //InitMatrix(m);
 ReplaceCol(m, 0, h);
 beta = MatrixRoot(m) / Dcr;
 InitMatrix(m, a, b, c);
 ReplaceCol(m, 1, h);
 gamma = MatrixRoot(m) / Dcr;
 InitMatrix(m, a, b, c);
 ReplaceCol(m, 2, h);
 delta = MatrixRoot(m) / Dcr;

 if (delta < 0) {
 	//printf("Negative Determinant - ");
 	return 0; // sphere is inprojectable - behind user
	}
 if (beta < -0.2 || beta > 1.2) {
 	//printf("Negative Determinant - ");
 	return 0; // sphere is inprojectable - out of screen
	}
 if (gamma <-0.2 || gamma> 1.2) {
 	//printf("Negative Determinant - ");
 	return 0; // sphere is inprojectable - out of screen
	}
 *dx = (int) round(beta * xres);
 *dy = (int) round(gamma* yres);
 return 1;
 }

/**
 * project_point - determines where the given point will map on the 2d screen
 * NOTE: Works correctly with any FOV, X/Y ratio, etc.
 * @param dx - output - x coordinate (scaled by X resolution)
 * @param dy - output - y coordinate
 * @param d - The point in 3D which shoud be mapped
 * @param cur - The camera
 * @param w - Vector projection grid
 * @param xres - X Resolution
 * @param yres - Y Resolution
 * @returns zero for no error. If there is some error, these values are ORed:
 * 8 - Math error (point is inprojectable)
 * 4 - The point is behind the user
 * 2 - The point is outside the Y range
 * 1 - The point is outside the X range
 */
#ifdef __GNUC__
int  project_point(float *dx, float *dy, const  Vector & d, const Vector & cur, Vector w[3], int xres, int yres) __attribute__((noinline));
#endif
int  project_point(float *dx, float *dy, const  Vector & d, const Vector & cur, Vector w[3], int xres, int yres)
{
	Vector a, b, c, h;
	double m[3][3];
	double Dcr, rDcr;
	double beta, gamma, delta;
		
	*dx = 0.0f; *dy = 0.0f;
	// init the a, b, c & h vectors
	h = cur - w[0];
	a = w[1] - w[0];
	b = w[2] - w[0];
	c.make_vector(d, cur);
	
	InitMatrix(m, a, b, c);
	Dcr = MatrixRoot(m);
	if (iabs(Dcr)<0.01) {
		//printf("Negative Determinant - ");
		return 8; // sphere is inprojectable - zero determinant
	} else rDcr = 1 / Dcr;
	
	// solve the ugly equation
	//InitMatrix(m);
	ReplaceCol(m, 0, h);
	beta = MatrixRoot(m) * rDcr;
	InitMatrix(m, a, b, c);
	ReplaceCol(m, 1, h);
	gamma = MatrixRoot(m) * rDcr;
	InitMatrix(m, a, b, c);
	ReplaceCol(m, 2, h);
	delta = MatrixRoot(m) * rDcr;
	
	if (delta < 0) {
		//printf("bad delta");
		return 4; // sphere is inprojectable - behind user
	}
	*dx = (beta * xres);
	*dy = (gamma* yres);
	return (1*(beta < 0.0f || beta > 1.0f)) | (2 * (gamma < 0.0f || gamma > 1.0f));
}

/**
 * project_point - the same as the above version, but returns ints, and the return value is
 * 1 for success, 0 for failure of the kind 4 | 8 in the above version
 */ 
int  project_point(int *dx, int *dy, const  Vector & d, const Vector & cur, Vector w[3], int xres, int yres)
{
	float x, y;
	int res = project_point(&x, &y, d, cur, w, xres, yres);
	if (res >= 4) return 0;
	*dx = (int) round(x);
	*dy = (int) round(y);
	return 1;
}

/** project_point_shadow
 * @brief Projects where a point will map on the screen.
 * @param vs - point to be mapped
 * @param l - light source
 * @param[out] result - target 2d coordinates to be written (coordinates are in screen space)
 * @param xres - X resolution
 * @param yres - Y resolution
 * @param cur - camera position
 * @param tt - upper-left point of the projection grid
 * @param a - column increase
 * @param b - row increase
 * @param[out] isfloor set to true if the projection is on the floor, or to false when it is on the ceiling
 * @param[out] casted - The point on the plane after the cast
 * @returns 0, ORed with the following bits:
 *          0x10 - Some math error (usually means incastable point)
 *          0x08 - The light and the point are colinear with the floor, cast is impossible
 *          0x04 - The point is behind the camera
 *          0x02 - The point is outside the y range
 *          0x01 - The point is outside the x range
 *          
 *  0x10 and 0x08 happen if the light projection goes to infinity (with an
 *  epsilon DST_THRESHOLD, used for floor rendering)
*/
int  project_point_shadow(const Vector & vs, const Vector & l, float result[], int xres, int yres, const Vector& cur, const Vector& tt,
			const Vector& a, const Vector& b, bool& isfloor, Vector &casted)
{
	double x, y, y_to_be, z, planeDist, m, lambda, theta, zurla, Dcr;
	double M[3][3];
	Vector h;
	y = vs[1] - l[1];
	if (fabs(y) == 0.0) return 8; // sphere shadow goes to infinity
	if (y < 0.0) {
		planeDist = l[1] - daFloor;
		y_to_be = daFloor;
		isfloor = true;
	} else {
		planeDist = daCeiling - l[1];
		y_to_be = daCeiling;
		isfloor = false;
	}

	m = planeDist / fabs(y);

	x = l[0] + (vs[0] - l[0])*m;
	z = l[2] + (vs[2] - l[2])*m;
	y = y_to_be;
	casted = Vector(x, y, z);

	// we're done with 3D projection. The sphere hits floor/ceiling in (x, y, z);

	// the following is based on the equation:
	/*
		  P = tt + lambda * ti + theta * tti = c + zurla * ((x,y,z)-c)

		the equaton is only one, but since we solve it for x, y and z, we get the three
		unknowns (lambda, theta & zurla)
	*/
	Vector c(x - cur[0], y - cur[1], z - cur[2]);
	h.make_vector(cur, tt);// -> vector from tt to cur
	InitMatrix3(M);

	Dcr = MatrixRoot(M);
	if (fabs(Dcr)<1E-8) {
		printf("Cannot solve shadow equation, zero determinant!\n");
		return 16;
	}

	ReplaceCol1(M, 0, h);
	lambda = MatrixRoot(M) / Dcr;
	int ret = 0;
	if (lambda < 0.0 || lambda >= xres) ret = 1;
	result[0] = lambda;
	InitMatrix3(M);
	ReplaceCol1(M, 1, h);
	theta = MatrixRoot(M) / Dcr;
	if (theta < 0.0 || theta >= yres) ret |= 2;
	result[1] = theta;
	InitMatrix3(M);
	ReplaceCol1(M, 2, h);
	zurla = MatrixRoot(M) / Dcr;
	if (zurla <= 0) ret |= 4;
	return ret;
}
// old verse
int  project_point_shadow(const Vector & vs, const Vector & l, int *x_2d, int *y_2d, int xres, int yres, const Vector& cur, const Vector& tt,
			 const Vector& a, const Vector& b, bool& isfloor)
{
	float ret[2];
	Vector temp;
	int res = project_point_shadow(vs, l, ret, xres, yres, cur, tt, a, b, isfloor, temp);
	if (res > 4) return 0;
	if (!(res & 0x01)) *x_2d = (int) ret[0];
	if (!(res & 0x02)) *y_2d = (int) ret[1];
	return res == 0;
}


double predlo[MAX_SPHERES], dlc;

void pass_pre(double *p, double plc, const Vector& LL)
{int i;
 dlc = plc;
 for (i=0;i<MAX_SPHERES;i++) predlo[i] = p[i];
 lix = LL;
}

// normalizes an angle in the 0..2*pi range
double normangle(double angle)
{
 angle *= M_1_PI;
 if (angle<0)
 	angle+=2;
 if (angle>2)
 	angle-=2;
 if (angle<0 || angle >2)
 	{printf("angle error!"); fflush(stdout);}
 return angle;
}


/*************************************************************************
 * search exact dist -- find the distance to the sphere's surface        *
 *                                                                       *
 * based on the fact, that there is always a factor Q, such that         *
 *                                                                       *
 * dist(sphere, cur + Q*v) = sphere.diameter                             *
 *                                                                       *
 * it yeilds to the following quadratic equation:                        *
 *                                                                       *
 * q^2*(vx^2+vy^2+vz^2)+2*q*(fx*vx+fy*vy+fz*vz)+fx^2+fy^2+fz^2-d.d^2 = 0 *
 * where f is a vector (cur - sphere.center), d.d is sphere radius       *
 *                                                                       *
 * There should be two solutions; we are interested in the smaller       *
 * of them only.                                                         *
 *                                                                       *
 *************************************************************************/
static inline double search_exact_dist(Sphere *a, const Vector& c, const Vector& v)
{Vector f;
 f.make_vector(c, a->pos);
 double ua, ub, uc, Dt;

 ua = v.lengthSqr();
 ub = f*v*2.0;
 uc = f.lengthSqr() - sqr(a->d);
 Dt = sqr(ub) - 4.0f*ua*uc;
 //if (Dt<0) { printf("SearchExactDist:error: negative determinant! it should only be positive!!!\n"); fflush(stdout); Dt=0;}

 // return the smaller root of the equation
 return ((-ub-fast_sqrt(Dt))/(2.0f*ua));
}


/* FindNormalVector - finds the normal of the plane, defined by the points a, b and c.

   returns 0 if the math operation is unapplicable (that is, the points are colinear)
   returns 1 and a vector in v  otherwise
*/
static inline int find_normal_vector(const Vector& a, const Vector& b, const Vector& c, Vector& v)
{Vector u = b-a, w = c-a;
 // are u and w colinear?
 if (eq(u[0]/w[0], u[1]/w[1]) && eq(u[1]/w[1], u[2]/w[2]))
 	return 0;
 // if they are not colinear, we can calculate the cross product of u and w, which will give us the normal of the plane
 v = u^w;
 return 1;
}


// returns the color of the ray v, originating from `cur'.
// `iteration' is a counter which prevents a ray from entering an endless cycle (very rare, but possible!)
// `recursive' is a boolean flag: if 0, the procedure won't check if the ray hits any sphere.
//                                if 1 and a sphere is being hit the same procedure is recursively called for the ray
//                                that goes out of the sphere then.
// `last_object' a pointer to the object the ray is traced from. This avoids a ray to hit again the sphere it just hit.
// `finfo' is a helper to determine which mipmap level to use

Uint32 Raytrace(const Vector& cur, Vector& v, int recursive, int iteration, Object *last_object, FilteringInfo & finfo)
{
	double ydist,scalefactor;
	double cx, cy, dist, mind;
	char context[128];
	float dp;
	bool is_trio = false;

	Object *z;
	int i;
	if (iteration > MAX_RAYTRACE_ITERATIONS)
		return RAYTRACE_BLEND_COLOR;
	if (recursive&RECURSIVE) {
		v.norm();
		mind = 999999999.0;
		z = NULL;
		for (i = 0; i < spherecount; i++) if (sp + i != last_object) {
			if (sp[i].intersect(v, cur, context)) {
				dist = sp[i].intersection_dist(context);
				if (dist > 0.0f && dist < mind) {
					mind = dist;
					z = sp + i;
				}
			}
		}
		int group_to_avoid = -1;
		if (last_object->get_type() == OB_TRIANGLE && 0==(static_cast<Triangle*>(last_object)->flags & RECURSE_SELF)) {
			group_to_avoid = static_cast<Triangle*>(last_object)->get_mesh_index();
			is_trio = true;
		}
		for (int j = 0; j < mesh_count; j++) {
			if (j != group_to_avoid && mesh[j].testintersect(cur,v)) {
				if (g_speedup && mesh[j].sdtree) {
					Triangle *t;
					if (mesh[j].sdtree->testintersect(cur, v, context, &t)) {
						dist = t->intersection_dist(context);
						if (dist < mind) {
							mind = dist;
							z = t;
						}
					}
				} else {
					for (i = 0; i < mesh[j].triangle_count; i++) {
						Triangle * t = trio + i + (j << TRI_ID_BITS);
	
						if (!t->okplane(cur)) {
							i += t->tri_offset;
							continue;
						}
	
						if (t != last_object && t->intersect(v, cur, context)) {
							dist = t->intersection_dist(context);
							if (dist > 0.0f && dist < mind) {
								mind = dist;
								z = t;
							}
						}
					}
				}
			}
		}
		if (z!=NULL) {
			z->intersect(v, cur, context);
			return z->shade(v, cur, lix, 1.0, dp, context, iteration + 1, finfo);
		}
	}
	if (BackgroundMode == BACKGROUND_MODE_VOXEL) {
		return voxel_raytrace(cur, cur+v);
	}
	ydist = fabs(v.v[1]);
	if (ydist<DST_THRESHOLD)
		return RAYTRACE_BLEND_COLOR;

	dp             = v.v[1] < 0.0f ? -daFloor : +daCeiling;
	dp            -= copysign(cur.v[1], v.v[1]);
	int ysqrd_raytrace = v.v[1] < 0.0f ? ysqrd_floor : ysqrd_ceil;
	scalefactor = dp/ydist;
	cx = cur.v[0] + v.v[0]*scalefactor;
	cy = cur.v[2] + v.v[2]*scalefactor;
#ifdef TEX_OPTIMIZE
	int level = iteration == 1 ? last_object->get_best_miplevel(cx, cy, finfo) : TEX_S;
	/*
	int level = TEX_S;
	if (1 == iteration) {
		if (is_trio) {
			if (finfo.ml->obj == NULL) {
				finfo.ml->obj = last_object;
				finfo.ml->value = last_object->GetBestMipLevel(cx, cy, finfo);
			}
			level = finfo.ml->value;
		} else {
			level = last_object->GetBestMipLevel(cx, cy, finfo);
		}
	}
	*/
#else
	int level = TEX_S;
#endif
	return (recursive&RAYTRACE_BILINEAR_MASK)?
		texture_handle_bilinear(cx, cy, level, ysqrd_raytrace):
		texture_handle_nearest((int) cx, (int) cy, level, ysqrd_raytrace );
}


// performs time calculation.
//
//  time -> frame difference, in seconds
//  resistance -> air resistance, see below
//  value -> current speed

// resistance is:
// 0-1 for no resistance
// 2 - ball slows down to 50% of its speed for a second
// 9 - ball slows down to 1/9 of its speed for a second
// etc. etc.

// dir is direction. if 1 the direction is forward (air resisance is applied), otherwise is backward (resistance is UNapplied)
double time_calc(double time, double resistance, double value, int dir)
{
    double res;
    if (resistance <= 1.0) return value;
    if (dir == 1)
    	res = value / exp(log(resistance) * time);
	else
	res = value * exp(log(resistance) * time);
    return res;
}

// adds the gravity to the Y component of the vector
void apply_gravity(Sphere *a, double t)
{
 if (a->flags & GRAVITY) {
 	a->mov.v[1] -= sv_gravity*(t-a->time);
 }
}

// mask is a bitmask: which components will the air be applied to
// Use APPLY_ALL to mean all components
// dir is direction: if 1, the air resistance is applied, otherwise it is undone ("un-applied")
void apply_air(Sphere *a, double t, int mask, int dir)
{
 double multiplyer;
 if (a->flags & AIR) {
 	if (a->flags & GRAVITY) multiplyer = sv_air * AIRTHICKNESS_NORMAL;
 			 else   multiplyer = sv_air * AIRTHICKNESS_HIGH;

 	if (mask & APPLY_X) a->mov.v[0] = time_calc((t-a->time) , multiplyer, a->mov.v[0], dir);
 	if (mask & APPLY_Y) a->mov.v[1] = time_calc((t-a->time) , multiplyer, a->mov.v[1], dir);
 	if (mask & APPLY_Z) a->mov.v[2] = time_calc((t-a->time) , multiplyer, a->mov.v[2], dir);
 }
}


double get_hit_time(double a, double b, double c)
{
	double Dt;
	Dt = b*b - 4*a*c;
	if (Dt<0) {
		//printf("Bug: Determinant in get_hit_time < 0!\n");
		Dt = 0;
	}
	return (-b-fast_sqrt(Dt)) / (2*a);
}

/********************************
 * void advance:                *
 *      input:                  *
 *      *a - sphere to advance; *
 *      t - sync time.          *
 *------------------------------*
 * moves a sphere depending on  *
 * its movement vector and the  *
 * time remaining to the sync   *
 * time. Checks if the sphere   *
 * hits the floor or the        *
 * ceiling - if so, a bounce is *
 * generated.                   *
 ********************************/

void advance(Sphere *a, double t)
{double ytoadd, x, rtime = t - a->time;
 int bModif = 0;
 if (rtime == 0) return;

 a->pos.v[0] += a->mov[0] * rtime;
 a->pos.v[2] += a->mov[2] * rtime;


 ytoadd = a->mov[1] * rtime;
 if ((a->flags & GRAVITY) && !(a->flags & NOFLOORBOUNCE))
 	if (a->pos[1] + ytoadd < daFloor + a->d) {// check for floor bounce
 		bModif = 1;
		/* so we found out that the bounce occurs. So we should make some correction to gravity, because
		 * `apply_gravity' assumes the acceleration towards the floor occurs all the way in the same direction.
		 * This ain't true. We should apply the gravity prior to the bounce, then change the sign of the Y component
		 * of the vector, and finally apply the needed amount of gravity after the bounce. (Thinking how gravity
		 * applies to the movement vector, it applies with different signs before and after the bounce)
		 */

		// first, we un-do the gravity, which has been previously applied by `apply_gravity'//
		// before that, undo air resistance:
		apply_air(a,t, APPLY_Y, -1);
		// undo gravity
		a->mov.v[1]+= sv_gravity * rtime;

		// calculate when it hits the floor.
		x = a->time + get_hit_time(-sv_gravity, a->mov[1], a->pos[1] - a->d);

		if (x>t) {
			//printf("Bug: bounce time was outside the sync time!\n");
			x = t;
			} else if (x<a->time) {
			//printf("Bug: bounce time was before start time!\n");
			x = a->time;
			}
		a->mov.v[1] = -(a->mov.v[1] - (x-a->time) * sv_gravity) * BOUNCE_COEFF;
		a->pos.v[1] = a->d + daFloor + (t-x)*a->mov.v[1];
		a->mov.v[1] -= (t-x)*sv_gravity;

		if (a->mov[1]<YI_THRESH) {
		// if the ball is slowed down enough, we may consider it stopped jumping and avoid
		// any further bounce checks
			a->mov.v[1]=0;
			a->pos.v[1] = a->d+daFloor;
			a->flags &= ~GRAVITY;     // don't do gravity anymore.
			}
 		}
 if ((a->flags & GRAVITY) && !(a->flags & NOFLOORBOUNCE))
 	if (a->pos[1] + ytoadd > daCeiling - a->d) {// check for ceiling bounce
	// we use much simplier routine, since a ball cannot roll on the ceiling, right?
	// (except sv_gravity is below zero, but ... :)
 		bModif = 1;
		ytoadd = (a->pos[1] + ytoadd) - (daCeiling - a->d);
		a->pos.v[1] = daCeiling - a->d - ytoadd;
		a->mov.v[1] = -a->mov[1] * BOUNCE_COEFF;
 		}
 if (!bModif)
	a->pos.v[1] += ytoadd;
 a->time = t; // fix the ball's time to the sync
}

// inits the "m0ving" array: a sphere is considered moving iff its speed vector is more than EPS
void check_moving(void)
{
	int i;
	for (i=0; i < spherecount; i++)
		m0ving[i] = ( sp[i].mov.lengthSqr() > MOVING_THRESH);
}

int can_collide(int i, int j)
{
	return (m0ving[i]||m0ving[j]);
}

// calculates the distance between a and b at time t
static inline double time_dist(Sphere *a, Sphere *b, double t)
{double t1, t2;
 t1 = t - a->time;
 t2 = t - b->time;
 Vector A = a->pos + a->mov * t1;
 Vector B = b->pos + b->mov * t2;
 return A.distto(B);
}

// checks if two spheres collide until time `t'.
// if collision occurs it returns the exact collision time. Else a value of -1 is returned
double collide (Sphere *a, Sphere *b, double t)
{double l, r, m, d0, d1;
 m = r = t;
 l = (a->time>b->time?a->time:b->time); // get the latter of the a's and b's times
 d0 = time_dist(a, b, l);
 while (r-l>TIME_EPS) {
 	m = (l+r)/2.0;
	d0 = time_dist(a, b, m);
	d1 = time_dist(a, b, m-TIME_EPS);
	if (d1 < d0)
		r = m;
	else
		l = m;
 	}
 if (d0 > a->d+b->d)
 	return -1; // the spheres do not collide
 //
#ifdef PRINT_COLLISIONS
 printf("Collision detected!\nA:\n");
 print_sphere_info(stdout, a);
 printf("B:\n");
 print_sphere_info(stdout, b);
 printf("--> lowest time : %.6lf\n", m);
 fflush(stdout);
#endif
 l = (a->time>b->time?a->time:b->time); // get the latter of the a's and b's times

 do {
 	m = (l+r) / 2.0;
	d0 = time_dist(a, b, m);
	if (d0 > a->d + b->d)
		l = m;
	else
		r = m;
 	} while (r-l > TIME_EPS);
#ifdef PRINT_COLLISIONS
 printf("--> collision time : %.6lf\n", l); fflush(stdout);
#endif
 sphere_check_for_first_hit();
 return l;
}

// assumes that a and b collide at time t. Upon that assumption it calculates the new motion vectors for a and b
void process_incident(Sphere *a, Sphere *b, double t)
{
	double strength, d1;
	a->pos += a->mov * (t - a->time);
	b->pos += b->mov * (t - b->time);
	a->time = t;
	b->time = t;
	d1 = time_dist(a, b, t);
	strength = d1 - time_dist(a, b, t+TIME_EPS);
	d1 *= 0.5;
	if (strength<0) return; // the hit is too weak
	strength *= T_1_TIME_EPS / (a->mass + b->mass) / d1;
	if (!(a->flags & STATIC)) {
		a->mov += (a->pos-b->pos)*(b->mass * strength);
	}
	if (!(b->flags & STATIC)) {
		b->mov += (b->pos-a->pos)*(a->mass * strength);
	}
	// custom onHit modification goes here:
	if (!(a->flags & STATIC)) {
		a->flags &= ~ANIMATED;
		a->flags |= GRAVITY|AIR;
	}
	if (!(b->flags & STATIC)) {
		b->flags &= ~ANIMATED;
		b->flags |= GRAVITY|AIR;
	}
}


void set_gravity(double new_gravity) {
	sv_gravity = new_gravity;
}

int ograve=0;

void switch_gravity(void)
{
	int i;
	double sign=1;
	ograve++;
	switch (ograve%4) {
		case 0:
		case 2: sv_gravity = 0.001; break;
		case 1: sign = -1;
		case 3: sv_gravity = -sign * 166.0;
			for (i=0;i<spherecount;i++)
				if (sp[i].flags & AIR)
					sp[i].flags |= GRAVITY;
			break;
	}

}

/** ** ** ** ** ** ** ** ** ** ** ** **
 * ** ** @class FrustrumClipper ** ** **
 * *** ** ** ** ** ** ** ** ** ** ** **
*/

const Vector imatrix[3] = {
	Vector(1.0, 0.0, 0.0),
	Vector(0.0, 1.0, 0.0),
	Vector(0.0, 0.0, 1.0)
};

FrustrumClipper::FrustrumClipper(const Vector& camera, const Vector& ul, const Vector& x, const Vector &y)
{
	Vector e[4];
	e[0] = ul;
	e[1] = ul + x;
	e[2] = ul + y;
	e[3] = ul + x + y;
	
	form_plane(camera, e[1], e[0], 0);
	form_plane(camera, e[0], e[2], 1);
	form_plane(camera, e[3], e[1], 2);
	form_plane(camera, e[2], e[3], 3);
	
	
}

void FrustrumClipper::form_plane(const Vector &a, const Vector &b, const Vector &c, int index)
{
	Vector x = b - a;
	Vector y = c - a;
	Vector nor = x ^ y;
	nor.norm();
	n[index] = nor;
	D[index] = -(a*nor);
}

double FrustrumClipper::eval(const Vector &v, int which)
{
	if (which == -1) which = defeval;
	return D[which] + (v * n[which]);
}

void FrustrumClipper::clip(ClipRes& cr, Vector trio[], int which)
{
	if (which == 0) {
		cr.n = 3;
		for (int i = 0; i < 3; i++) {
			cr.v[i] = trio[i];
			cr.bary[i] = imatrix[i];
		}
		return;
	}
	if (which & (which - 1)) {
		printf("%s: Clipping against more than one plane is (still) not supported\n", __FUNCTION__);
		cr.n = 0;
		return;
	}
	defeval = 0;
	while (0 == (which & (1 << defeval))) defeval++;
	
	cr.n = 0;
	bool vis[3];
	for (int i = 0; i < 3; i++) {
		vis[i] = eval(trio[i]) > -1;
	}
	for (int i = 0; i < 3; i++) {
		int j = (i + 1) % 3;
		if (vis[i]) {
			cr.v[cr.n] = trio[i];
			cr.bary[cr.n++] = imatrix[i];
		}
		if (vis[i] ^ vis[j]) {
			double av = eval(trio[i]);
			double bv = eval(trio[j]);
			double p = - av / (bv - av);
			if (p < 0) p = 0;
			if (p > 1) p = 1;
			cr.v[cr.n] = trio[i] * (1.0-p) + trio[j] * p;
			cr.bary[cr.n++] = imatrix[i] * (1.0-p) + imatrix[j] * p;
		}
	}
}

Array<vec2f> convex_hull(Array<vec2f> input)
{
	sort(&input[0], input.size());
	
	Array<vec2f> l, r;
	for (int i = 0; i < 2; i++) {
		l += input[i];
		r += input[i];
	}
	
	for (int i = 2; i < input.size(); i++) {
		l += input[i];
		r += input[i];
		while (l.size() > 2 && face(l[l.size()-3], l[l.size()-2], l[l.size()-1]) > -1e-6) {
			l.erase(l.size()-2);
		}
		while (r.size() > 2 && face(r[r.size()-3], r[r.size()-2], r[r.size()-1]) < +1e-6) {
			r.erase(r.size()-2);
		}
	}
	
	Array<vec2f> ret;
	for (int i = 0; i < r.size(); i++)
		ret += r[i];
	for (int i = l.size() - 2; i > 0; i--)
		ret += l[i];
	return ret;
}


Vector perpendicular(const Vector &v)
{
	int max_compo = -1;
	double max_value = 0.0;
	
	for (int i = 0; i < 3; i++) {
		double t = fabs(v[i]);
		if (t > max_value) {
			max_value = t;
			max_compo = i;
		}
	}
	switch (max_compo) {
		case  0: return Vector(-(v * Vector(0,1,0))/v[0], 1, 0);
		case  1: return Vector(0, -(v * Vector(0,0,1))/v[1], 1);
		case  2: return Vector(0, 1, -(v * Vector(0,1,0))/v[2]);
		case -1:
		default:
			return Vector(0,0,0);
	}
}

////////////////////////

void vectormath_close(void)
{
 //printf("diffuse: %.2lf, specular: %.2lf\n", diffuse, specular);
}


