/***************************************************************************
 *   Copyright (C) 2006 by Veselin Georgiev                                *
 *   anrieff@mailg.com (convert to gmail)                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/


#include "MyTypes.h"
#include "object.h"
#include "triangle.h"
#include "mesh.h"
#include "sphere.h"
#include "sor.h"
#include "vectormath.h"
#include "light.h"
#include "cross_vars.h"
#include "progress.h"
#include "random.h"

static const float divs[3] = { 1.0f, 1 / 7.0f, 1 / 13.0f };
static const int iterations[3] = { 1, 7, 13 };
static const float BIAS = 7.0f;

/**
 * @Class PointLight
 */
 
PointLight::PointLight()
{
	size = 0;
	for (int i = 0; i < 6; i++)
		maps[i] = NULL;
}

PointLight::~PointLight()
{
	for (int i = 0; i < 6; i++) {
		if (maps[i]) free(maps[i]);
		maps[i] = NULL;
	}
}

float PointLight::closest_intersection(const Vector &o, const Vector &dir)
{
	double mdist = 1e99, cd;
	char ctx[128];
	
	for (int i = 0; i < spherecount; i++) {
		if (sp[i].intersect(dir, o, ctx)) {
			cd = sp[i].intersection_dist(ctx);
			if (cd > 0 && cd < mdist) mdist = cd;
		}
	}
	
	for (int i = 0; i < mesh_count; i++) {
		if (mesh[i].testintersect(o, dir)) {
			if (CVars::g_speedup && mesh[i].sdtree) {
				Triangle *t;
				if (mesh[i].sdtree->testintersect(o, dir, ctx, &t)) {
					cd = t->intersection_dist(ctx);
					if (cd > 0 && cd < mdist) mdist = cd;
				}
			} else {
				for (int j = 0; j < mesh[i].triangle_count; j++) {
					Triangle * t = trio + j + (i << TRI_ID_BITS);
	
					if (!t->okplane(o)) {
						continue;
					}
	
					if (t->intersect(dir, o, ctx)) {
						cd = t->intersection_dist(ctx);
						if (cd > 0 && cd < mdist) mdist = cd;
					}
				}

			}
		}
	}
	for (int i = 0; i < sorcount; i++) {
		if (sor[i].intersect(dir, o, ctx)) {
			cd = sor[i].intersection_dist(ctx);
			if (cd > 0 && cd < mdist) mdist = cd;
		}
	}
	
	return mdist < 1e50 ? (float) mdist : 0.0f;
}

float* PointLight::build_lightmap_side(const Vector &dv)
{
	Task task(1.0);
	task.set_steps(size);
	float *a = (float *) malloc(sizeof(float) * size * size);
	if (!a) return NULL;
	bool has_intersection = false;
	
	for (int i = 0; i < size; i++) {
		for (int j = 0; j < size; j++) {
			double co[2] = { (2.0 * i) / size - 1.0, (2.0 * j) / size - 1.0 };
			int k = 0;
			Vector v = dv;
			for (int l = 0; l < 3; l++)
				if (v[l] == 0) {
					v[l] = co[k++] + (drandom()*2-1)/size;
				}

			
			v.norm();
			float res = closest_intersection(p, v);
			a[i*size + j] = res;
			if (res > 0) has_intersection = true;
		}
		task++;
	}
	
	if (!has_intersection) {
		free(a);
		a = NULL;
	}
	return a;
}

int PointLight::in_shadow(const Vector & point)
{
	if (CVars::shadow_algo == 1)
		return pure_raytrace_shadow(point);
	Vector dir;
	dir.make_vector(point, p);
	float dist = dir.lengthSqr();
	
	// see max component
	double mc = 0;
	int bi = -1;
	for (int i = 0; i < 3; i++) {
		if (fabs(dir[i]) > mc) {
			mc = fabs(dir[i]);
			bi = i;
		}
	}
	
	dir.scale(1.0 / mc);
	float * map = maps[bi * 2 + (dir[bi] > 0 ? 0 : 1)];
	if (!map) return -1;
	
	int mx, my;
	switch (bi) {
		case 0:
			mx = (int) ((dir[1] * 0.5 + 0.5) * size);
			my = (int) ((dir[2] * 0.5 + 0.5) * size);
			break;
		case 1:
			mx = (int) ((dir[0] * 0.5 + 0.5) * size);
			my = (int) ((dir[2] * 0.5 + 0.5) * size);
			break;
		case 2:
			mx = (int) ((dir[0] * 0.5 + 0.5) * size);
			my = (int) ((dir[1] * 0.5 + 0.5) * size);
			break;
		default:
			mx = my = 0;
	}
	if (mx >= size) mx = size;
	if (my >= size) my = size;
	
	float mapdist = map[mx * size + my];
	return (mapdist != 0 && mapdist * mapdist + 2*BIAS*mapdist + BIAS*BIAS < dist);
}

void PointLight::rebuild_lightmap(void)
{
	size = CVars::lmsize;
	if (size <= 0) return;
	for (int i = 0; i < 6; i++) {
		Vector dv(0.0, 0.0, 0.0);
		dv[i/2] = i % 2 == 0 ? +1.0 : -1.0;
		if (maps[i]) free(maps[i]);
		maps[i] = build_lightmap_side(dv);
	}
}

int PointLight::pure_raytrace_shadow(const Vector& point)
{
	Vector dir;
	dir.make_vector(this->p, point);
	dir.norm();
	for (int i = 0; i < spherecount; i++)
		if (sp[i].sintersect(point, dir, 0))
			return 1;
	for (int i = 0; i < mesh_count; i++)
		if (mesh[i].sintersect(point, dir, 0))
			return 1;
	return 0;
}


/**
 * @class Light
 */

Light::Light()
{
	p = Vector(256.0, 125.0, 256.0);
	setradius(7.5f);
}

void Light::setradius(float new_radius)
{
	radius = new_radius;
	reposition();
}

void Light::reposition(void)
{
	double R = radius;
	points[0].p = p;
	points[1].p = p + Vector( +R, 0.0, 0.0);
	points[2].p = p + Vector( -R, 0.0, 0.0);
	points[3].p = p + Vector(0.0,  +R, 0.0);
	points[4].p = p + Vector(0.0,  -R, 0.0);
	points[5].p = p + Vector(0.0, 0.0,  +R);
	points[6].p = p + Vector(0.0, 0.0,  -R);
	double R1 = R * 0.707106781186;
	points[ 7].p = p + Vector(+R1, +R1, 0.0);
	points[ 8].p = p + Vector(-R1, -R1, 0.0);
	points[ 9].p = p + Vector(+R1, 0.0, +R1);
	points[10].p = p + Vector(-R1, 0.0, -R1);
	points[11].p = p + Vector(0.0, +R1, +R1);
	points[12].p = p + Vector(0.0, -R1, -R1);
}

float Light::shadow_density(const Vector & point)
{
	int n = iterations[CVars::shadow_quality];
	int sum = 0;
	for (int i = 0; i < n; i++) {
		int r = points[i].in_shadow(point);
		if (r == -1) return 0.0f; // optimization hack
		else sum += r;
	}
	return sum * divs[CVars::shadow_quality];
}

void Light::rebuild_lightmaps(void)
{
	if (mode != LIGHTMAP) return;
	progressman.full_reset("Rebuilding lightmaps");
	progressman.add_weight(13*6);
	for (int i = 0; i < 13; i++) {
		points[i].rebuild_lightmap();
	}
}

void Light::postload_init(void)
{
	if (mode == LIGHTMAP) {
		if (CVars::lmsize == 0)
			CVars::lmsize = 128;
		rebuild_lightmaps();
	}
}


Light light;
