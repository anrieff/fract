/***************************************************************************
 *   Copyright (C) 2004 by Veselin Georgiev                                *
 *   vesko@ViruS                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef __SPHERE_H__
#define __SPHERE_H__
#include <stdio.h>
#include "bitmap.h"
#include "common.h"
#include "vector3.h"
#include "object.h"

#define M_SIDE_CONSTANT 6.6666


// the color for larger spheres
#define KOLOR1 0xf7bd4a
// the color for da small ones ;)
#define KOLOR2 0x16a6ff
//
#define KOLOR3 0xffe7ef

struct sphere_intersection_context{
	double determinant, gB;
};


struct Sphere : public Object, public ShadowCaster {
/*32*/ Vector pos; // coordinates
/*32*/ Vector mov; // movement vector
/* 8*/ double d; // radius
/*16*/ double mass, time; /* Here, `time' is expressed in seconds and means that the coordinates
			of the sphere are valid at that time only. This allows to have spheres
			that are different in age. At the end of each frame calculation, `time'-s of
			all spheres are equalized and the needed movement is processed by then */
/* 8*/ double dist; //distance from the user
/* 4*/ float refl; //reflection strength
/* 4*/ float opacity; // opacity if the object is SEETHROUGH
/* 4*/ int AniIndex;// index in the array (see sphere.cpp). Used if the object is ANIMATED
/* 4*/ RawImg *tex; // used if the object is MAPPED - points to the texture to map.
/*12*/ int r, g, b; //sphere color
/* 4*/ int flags;
/* 8*/ int occluders_ind, occluders_size; // Index and size in the occluders array

// -> sizeof(sphere) == 128

// implement the functions we need to comply with the `object' class
	double get_depth(const Vector &camera);
	bool is_visible(const Vector & camera, Vector w[3]);
	int calculate_convex(Vector pt[], Vector camera);
	void calculate_fixed_convex(Vector pt[], Vector source, int sides);
	void map2screen(Uint32 *framebuffer, int color, int sides, Vector pt[], Vector camera, Vector w[3],
		int & min_y, int & max_y);
	int get_best_miplevel(double x0, double z0, FilteringInfo & fi);
	inline bool fastintersect(const Vector& ray, const Vector& camera, const double& rls, void *IntersectContext) const
	{
		sphere_intersection_context *spc = (sphere_intersection_context*) IntersectContext;
		Vector t;
		double C;
		t.make_vector(camera, pos);
		spc->gB = t*ray;
		C = t.lengthSqr() - d * d;
		spc->determinant = spc->gB*spc->gB - rls*C;
		return (spc->determinant >= 0.0f && spc->gB <= 0.0f);
	}
	inline bool intersect(const Vector& ray, const Vector &camera, void *IntersectContext)
	{
		sphere_intersection_context *spc = (sphere_intersection_context*) IntersectContext;
		Vector t;
		t.make_vector(camera, pos);
		spc->gB = t*ray;
		spc->determinant = spc->gB*spc->gB - t.lengthSqr() + d*d;
		return (spc->determinant >= 0.0f && spc->gB <= 0.0f);
	}
	inline double intersection_dist(void *IntersectContext) const
	{
		sphere_intersection_context *spc = (sphere_intersection_context*) IntersectContext;
		return (-spc->gB - fsqrt(spc->determinant));
	}
	inline OBTYPE get_type(bool generic = true) const
	{
		return OB_SPHERE;
	}

	Uint32 shade(Vector& ray, const Vector& camera, const Vector& light, double rlsrcp,
			float *opacity, void *IntersectContext, int iteration, FilteringInfo& finfo);
	
	/* From ShadowCaster */
	bool sintersect(const Vector & start, const Vector & dir, int opt = 0);

private:
	float _shadow_test(const Vector &I, const Vector & light, int opt);
};

typedef struct {
	double cx, cy, cz;
	double radius;
	double alpha, beta;
	double speed, betaspeed;
	double unused;
}AniSphere;


void setcolor(Sphere *a, int col);
void create_fract_array(Sphere *a, int *count);
void create_voxel_sphere_array(Sphere *a, int *count);
void calculate_XYZ(Sphere *a, double time, Vector& o);
void print_sphere_info(FILE *f, Sphere *a);
void sphere_check_for_first_hit(void);
void sphere_close(void);


#endif // __SPHERE_H__
