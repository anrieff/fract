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

struct Triangle : public object {
	Vector vertex[3];
	Vector normal, crossproduct, a, b, ah, hb;
	Vector center;
	float mapcoords[3][2], ma[2], mb[2];
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
	}
	void recalc_zdepth(const Vector & camera)
	{
		ZDepth = center.distto(camera);
		Vector h;
		h.make_vector(camera, vertex[0]);
		ah = a^h;
		hb = h^b;
	}

	int GetMeshIndex(void) const;
	int GetTriangleIndex(void) const;
	
// implement the functions we need to comply with the `object' class
	OBTYPE GetType(bool generic = true) const
	{
		return OB_TRIANGLE;
	}
	bool OkPlane(const Vector & camera);
	double GetDepth(const Vector &camera);
	bool IsVisible(const Vector & camera, Vector w[3]);
	int CalculateConvex(Vector pt[], Vector camera);
	void MapToScreen(Uint32 *framebuffer, int color, int sides, Vector pt[], Vector camera, Vector w[3],
		int & min_y, int & max_y);
	bool FastIntersect(const Vector& ray, const Vector& camera, const double& rls, void *IntersectContext) const;
	bool Intersect(const Vector& ray, const Vector &camera, void *IntersectContext);
	double IntersectionDist(void *IntersectContext) const;
	Uint32 Solve3D(Vector& ray, const Vector& camera, const Vector& light, double rlsrcp,
			float *opacity, void *IntersectContext, int iteration, FilteringInfo& finfo);
	int GetBestMipLevel(double x0, double z0, FilteringInfo & fi);
};

extern Triangle trio[MAX_TRIANGLES];

#include "mesh.h"

void create_triangle_array(void);
void triangle_close(void);

#endif // __TRIANGLE_H__
