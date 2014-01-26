/***************************************************************************
 *   Copyright (C) 2004 by Veselin Georgiev                                *
 *   vesko@ViruS                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef __TRIANGLE_H__
#define __TRIANGLE_H__
 
#include "MyTypes.h"
#include "vector3.h"
#include "object.h"


#define REFRACT_CACHE_SIZE 12

struct triangle_intersect_context {
	float u, v;
	double dist;
	int last_refract[REFRACT_CACHE_SIZE];
	int rfc_size;
};

/* ========================== Classes =================================== */

void retransformate(float ma[], float mb[], float ha, float hb, float& p, float &q);

struct Triangle : public Object {
	Vector vertex[3];
	Vector normal, crossproduct, a, b, ah, hb;
	Vector center;
	float mapcoords[3][2], ma[2], mb[2];
	Vector bdx, bdy;
	int nm_index[3];
	double ZDepth;
	Uint32 color;
	int flags;
	int tri_offset;
	float refl, opacity;
	bool visited, visible;

	void recalc_normal(void)
	{
		a.make_vector(vertex[1], vertex[0]);
		b.make_vector(vertex[2], vertex[0]);
		normal = crossproduct = a^b;
		normal.norm();
		center.add3(vertex[0], vertex[1], vertex[2]);
		center.scale(0.3333333333333);
		visited = false;
		ma[0] = mapcoords[1][0] - mapcoords[0][0];
		ma[1] = mapcoords[1][1] - mapcoords[0][1];
		mb[0] = mapcoords[2][0] - mapcoords[0][0];
		mb[1] = mapcoords[2][1] - mapcoords[0][1];
		Vector ta = a, tb = b;
		ta.norm();
		tb.norm();
		float p, q;
		retransformate(ma, mb, 1.0f, 0.0f, p, q);
		bdx = ta * p + tb * q;
		retransformate(ma, mb, 0.0f, 1.0f, p, q);
		bdy = ta * p + tb * q;
	}
	void recalc_zdepth(const Vector & camera)
	{
		ZDepth = center.distto(camera);
		Vector h;
		h.make_vector(camera, vertex[0]);
		ah = a^h;
		hb = h^b;
	}

	int get_mesh_index(void) const;
	int get_triangle_index(void) const;
	
// implement the functions we need to comply with the `object' class
	OBTYPE get_type(bool generic = true) const
	{
		return OB_TRIANGLE;
	}
	bool okplane(const Vector & camera);
	double get_depth(const Vector &camera);
	bool is_visible(const Vector & camera, Vector w[3]);
	int calculate_convex(Vector pt[], const Vector& camera);
	void map2screen(Uint32 *framebuffer, int color, int sides, Vector pt[], const Vector& camera, Vector w[3],
		int & min_y, int & max_y);
	bool fastintersect(const Vector& ray, const Vector& camera, const double& rls, void *IntersectContext) const;
	bool intersect(const Vector& ray, const Vector &camera, void *IntersectContext);
	double intersection_dist(void *IntersectContext) const;
	Uint32 shade(Vector& ray, const Vector& camera, const Vector& light, double rlsrcp,
			float &opacity, void *IntersectContext, int iteration, FilteringInfo& finfo);
	int get_best_miplevel(double x0, double z0, FilteringInfo & fi);
};

extern Triangle trio[MAX_TRIANGLES];

void create_triangle_array(void);
void triangle_close(void);

#endif // __TRIANGLE_H__
