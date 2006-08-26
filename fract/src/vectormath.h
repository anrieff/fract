/***************************************************************************
 *   Copyright (C) 2004 by Veselin Georgiev                                *
 *   vesko@ViruS                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "infinite_plane.h"
#include "vector3.h"
#include "object.h"
#define APPLY_X		1
#define APPLY_Y		2
#define APPLY_Z		4
#define APPLY_ALL	(APPLY_X|APPLY_Y|APPLY_Z)

#define RAYTRACE_BILINEAR_MASK 0x10000
#define MOVING_THRESH 0.0001

// this is obtained as the refraction factor Ni/Nr, where Ni is the refraction index of the air (1.0002926), and Nr is
// currently 1.51714, which is the refraction index of the glass
#define REFRACT_FACTOR 0.65932781417667
#define REFRACT_FACTOR_RECIPROCAL 1.5166962

// this is the square of the refract factor
#define REFRACT_FACTOR_SQR 0.43471316654699
// 1 - REFRACT_FACTOR_SQR
#define REFRACT_FACTOR_SQR_M1 0.56528683345301

//#define REFRACT_FACTOR 0.9
//#define REFRACT_FACTOR_SQR 0.81
//#define REFRACT_FACTOR_SQR_M1 0.19


void r0tate(double *c, double alpha, double beta);
void r0tate2d(double *x, double *y, double angle);
void calc_grid_basics(const Vector &c, double alpha, double beta, Vector w[3]);
int  project_sphere(Sphere *a, const Vector& cur, const Vector w[3], int *dx, int *dy, int xres, int yres);
int  project_point(int *dx, int *dy, const Vector& d, const Vector& cur, Vector w[3], int xres, int yres);
int  project_point(float *x, float *y, const Vector& d, const Vector& cur, Vector w[3], int xres, int yres);
int  project_point_shadow(const Vector &vs, const Vector& l, int *x_2d, int *y_2d, int xres, int yres, Vector& cur, Vector& tt, Vector& a, Vector& b, bool & isfloor);
int  project_point_shadow(const Vector &vs, const Vector& l, float *coords, int xres, int yres, Vector &cur, Vector &tt, Vector &a, Vector &b, bool & isfloor, Vector& casted);
double normangle(double angle);
//int testsphere(Sphere *a, double *v, double *cur);
Uint32 Raytrace(const Vector& src, Vector& v, int recursive, int iteration, Object *last_object, FilteringInfo &);
void pass_pre(double *p, double plc, const Vector& LL);
void apply_gravity(Sphere *a, double t);
void apply_air(Sphere *a, double t, int mask, int dir);
void advance(Sphere *a, double t);
void check_moving(void);
int can_collide(int i, int j);
double collide (Sphere *a, Sphere *b, double btm);
void process_incident(Sphere *a, Sphere *b, double t);

void set_gravity(double new_gravity);
void switch_gravity(void);
//void modify(int which, double amount);
void vectormath_close(void);

/**
 * @struct	ClipRes
 * @brief	Defines a result from FrustrumClipper::clip function
 * 
 * @param n	- Number of points in the resulting poly (may be 0 -
 * 		  this means the triangle is out of the frustrum)
 * @param v	- Vertices of the resulting poly
 * @param bary	- The barycentric coordinates of the resulting poly,
 *		  relative to the original triangle
*/
#define MAX_CLIP_RESULT 16
struct ClipRes {
	int n;
	Vector v[MAX_CLIP_RESULT];
	Vector bary[MAX_CLIP_RESULT];
};

/**
 * @class	FrustrumClipper
 * @date	2006-06-08
 * @author	Veselin Georgiev
 * @brief	Intersects and clips triangles against a view frustrum
*/ 
class FrustrumClipper {
	double D[4];
	Vector n[4];
	int defeval;
	
	void form_plane(const Vector &a, const Vector &b, const Vector &c, int index);
	double eval(const Vector &v, int which = -1);
public:
	/**
	 * Constructor:
	 * @param camera	- The location of the camera
	 * @param upleft	- a point that lies along the upleft corner ray of the view frustrum
	 * @param xdir		- X direction (upleft + xdir must be the upright corner of the frustrum)
	 * @param ydir		- Y direction (upleft + ydir must be the downleft corner of the frustrum)
	 */ 
	FrustrumClipper(const Vector &camera, const Vector& upleft, const Vector& xdir, const Vector &ydir);
	
	/**
	 * Performs clipping of the given triangle, with the specified planes
	 * @param clipres	Struct to store result
	 * @param trio		Array of 3 Vectors (the triangle itself)
	 * @param which		A bit mask of the view planes to clip against (may be ORed together):
	 * 	1	- up plane
	 * 	2	- left plane
	 * 	4	- right plane
	 * 	8	- down plane
	 * 	(the default, 15, is "clip against all planes")
	*/ 
	void clip(ClipRes& clipres, Vector trio[], int which = 15);
};
