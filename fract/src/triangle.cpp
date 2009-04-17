/***************************************************************************
 *   Copyright (C) 2004 by Veselin Georgiev                                *
 *   vesko@ViruS                                                           *
 *                                                                         *
 *   This program is free software; you can redepthribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *-------------------------------------------------------------------------*
 *                                                                         *
 * TRIANGLE.CPP - Copes with vertex meshes and its primitives (triangles)  *
 *        - triangle pre-projection                                        *
 *        - phong shading, reflections                                     *
 *        - physics engine for meshes                                      *
 *                                                                         *
 ***************************************************************************/

#include "MyGlobal.h"

#include "antialias.h"
#include "common.h"
#include "fract.h"
#include "gfx.h"
#include "profile.h"
#include "render.h"
#include "triangle.h"
#include "mesh.h"
#include "vector3.h"
#include "vectormath.h"
#include "light.h"
#include "cvars.h"

// ********************** GLOBAL STORAGE ***********************************/
Triangle trio[MAX_TRIANGLES];
extern int spherecount;

/** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * @class Triangle                                                          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/

int Triangle::get_triangle_index() const
{
	return (this - trio);
}

int Triangle::get_mesh_index() const
{
	return (this - trio) >> TRI_ID_BITS;
}

double Triangle::get_depth(const Vector &camera)
{
	return ZDepth;
}

bool Triangle::okplane(const Vector & camera)
{
	Vector vec;
	vec.make_vector(center, camera);
	return (vec * normal) < 0.0;
}


bool Triangle::is_visible(const Vector & camera, Vector w[3])
{
	if (visited) return visible;
	visited = true;
	bool vert_in_screen = false;
	int x, y, xr, yr;
	xr = xsize_render(xres());
	yr = ysize_render(yres());
	for (int i = 0; i < 3; i++) {
		if (!project_point(&x, &y, vertex[i], camera, w, xr, yr)) {
			visible = false;
			return false;
		}
		if (x >= 0 && x < xr && y >= 0 && y < yr) vert_in_screen = true;
	}
	if (!vert_in_screen) {
		visible = false;
		return false;
	}
	visible = okplane(camera);
	return visible;
}


int Triangle::calculate_convex(Vector pt[], const Vector& camera)
{
	for (int i = 0; i < 3; i++)
		pt[i] = vertex[2-i];
	return 3;
}

void Triangle::map2screen(
			   Uint32 *framebuffer, 
			   int color, 
			   int sides, 
			   Vector pt[], 
			   const Vector& camera, 
			   Vector w[3], 
			   int & min_y,
			   int & max_y)
{
	int L[RES_MAXY], R[RES_MAXY];
	int ys, ye, bs, be;
	min_y = ys = accumulate(pt, sides, camera, w, fun_less, 999666111, bs);
	max_y = ye = accumulate(pt, sides, camera, w, fun_more,-999666111, be);
	if (be == bs) {
		be++;
		be %= sides;
	}
	int size = (be + sides - bs) % sides;
	project_hull_part(L, pt, bs, +1, size, sides, color, camera, w, fun_min, fround);
	project_hull_part(R, pt, bs, -1, sides - size, sides, color, camera, w, fun_max);
	map_hull(framebuffer, L, R, ys, ye, color, 0);
}

bool Triangle::fastintersect(const Vector& ray, const Vector& camera, const double& rls, void *IntersectContext) const
{
	if (!visible) return false;
	triangle_intersect_context *tic = (triangle_intersect_context*)IntersectContext;
#if defined __SSE__ && defined USE_ASSEMBLY
	float Dcr= ray * crossproduct;
#else
	double Dcr = ray * crossproduct;
#endif
	tic->u = (hb * ray);
	tic->v = (ah * ray);
	if (fabs(Dcr) < 1e-9 || tic->u * Dcr < 0 || tic->v * Dcr < 0) return false;


#if defined __SSE__ && defined USE_ASSEMBLY
	float rDcr = sse_rcp(Dcr);//1.0 / Dcr;
#else
	double rDcr = 1.0 / Dcr;
#endif
	//Vector h;

	//h.make_vector( camera, vertex[0]);
	tic->u *= rDcr;
	tic->v *= rDcr;
	if (tic->u + tic->v > 1.0) return false;
	//tic->dist = -rDcr * ma3x(a,b,h)  / fast_sqrt(rls);
	return true;
}

bool Triangle::intersect(const Vector& ray, const Vector &camera, void *IntersectContext)
{
	triangle_intersect_context *tic = (triangle_intersect_context*)IntersectContext;
	Vector h;
	h.make_vector( camera, vertex[0] );
	Vector hr = h ^ ray;
#if defined __SSE__ && defined USE_ASSEMBLY
	float Dcr= ray * crossproduct;
#else
	double Dcr = ray * crossproduct;
#endif
	tic->u = -(b * hr);
	tic->v =  (a * hr);
	if (fabs(Dcr) < 1e-9 || tic->u * Dcr < 0 || tic->v * Dcr < 0) return false;
#if defined __SSE__ && defined USE_ASSEMBLY
	float rDcr = sse_rcp(Dcr);
#else
	double rDcr = 1.0 / Dcr;
#endif
	tic->u *= rDcr;
	tic->v *= rDcr;
	if (tic->u + tic->v > 1.0) return false;
	tic->dist = -rDcr * (crossproduct * h);

	return true;
}

double Triangle::intersection_dist(void *IntersectContext) const
{
	triangle_intersect_context *tic = (triangle_intersect_context*)IntersectContext;

	return tic->dist;
}

static Vector refract(const Vector &I, const Vector &N, double eta)
{
	double dot = N * I;
	double k = 1.0 - eta * eta * (1.0 - dot * dot);
	if (k < 0) {
		return Vector(0.0, 0.0, 0.0);
	} else {
		Vector temp = I * eta - N * (eta * dot + sqrt(k));
		temp.norm();
		return temp;
	}
}

/** 
 * fresnel() - given the cosine of incidence incoming angle, return
 * the fresnel coefficient of the reflection/refraction. The returned
 * value represents what part of the total radiance is transmitted,
 * i.e. a value of 0.2 will mean that 20% of light is refracted and
 * 80% is reflected
 * NOTE: ior is assumed to be 1.5
 * @param x the cosine of the incidence angle (must be in [0..1])
 * @returns refracted/total ratio, [0..1]
 */
static float fresnel(float cosThetaI)
{
	
	float sinThetaI = sqrtf(1 - cosThetaI * cosThetaI);
	float sinThetaT = sinThetaI/1.5f;
	float ThetaI = asin(sinThetaI);
	float ThetaT = asin(sinThetaT);
	//
	float Rs = sin(ThetaT - ThetaI)/sin(ThetaT + ThetaI); Rs *= Rs;
	float Rp = tan(ThetaT - ThetaI)/tan(ThetaT + ThetaI); Rp *= Rp;
	float R = (Rs + Rp) * 0.5f;
	return 1.0f - R;
}

Uint32 Triangle::shade(Vector& ray, const Vector& camera, const Vector& L, double rlsrcp,
		float &opacity, void *IntersectContext, int iteration, FilteringInfo& finfo)
{
	Vector CI;
	Vector ray_one, ray_reflected;
	Vector I;
	triangle_intersect_context *tic = (triangle_intersect_context*)IntersectContext;
	float specular_power = 0;

#ifdef MAKE_PROFILING
	if (!iteration) prof_enter(PROF_SOLVE3D);
#endif

	opacity = 1.0f;
	float intensity = ambient;
	if (flags & STOCHASTIC) {
		intensity += ambient * 0.5 * perlin(tic->u, tic->v);
	}
	Vector Normal;
	float gloss = mesh[get_mesh_index()].glossiness;
	if (flags & NORMAL_MAP) {
		Mesh *m = mesh + (get_mesh_index());
		Vector xu, xv;
		xu.make_vector(m->normal_map[nm_index[1]], m->normal_map[nm_index[0]]);
		xv.make_vector(m->normal_map[nm_index[2]], m->normal_map[nm_index[0]]);
		Normal.add3(
				m->normal_map[nm_index[0]],
				xu*(tic->u),
				xv*(tic->v)
			   );
		Normal.norm();
	} else {
		Normal= normal;
	}
	if (flags & BUMPMAPPED) {
		int meshidx = get_mesh_index();
		Mesh* m = mesh + meshidx;
		float *bump = m->bump_map;
		unsigned xx = m->bump_w;
		unsigned yy = m->bump_h;
		float x, y;
		x = mapcoords[0][0] + ma[0] * tic->u + mb[0] * tic->v;
		y = mapcoords[0][1] + ma[1] * tic->u + mb[1] * tic->v;
		Uint32 offset = ((int)(x * xx)) + ((int) (y * yy)) * xx;
		if (offset >= xx * yy) offset -= xx*yy;
		Vector mod = bdx * bump[offset * 2 + 0] + bdy * bump[offset * 2 + 1];
		Normal += mod;
		Normal.norm();
		/*
		Uint32 res = 0;
		res = ((Uint32)((Normal.x*0.5+0.5)*255.0) << 16) +
		      ((Uint32)((Normal.y*0.5+0.5)*255.0) <<  8) +
		      ((Uint32)((Normal.z*0.5+0.5)*255.0)      );
		return res;*/
	}

	Vector lightray;
	lightray.make_vector(vertex[0], L);
	float cp = -(Normal * lightray);

	// should we find the intersection point?
	if (cp > 0.0f || (flags & RAYTRACED)) {

		// find the crossing point I of "ray" and the triangle
		I.macc(vertex[0], a, tic->u);
		I += b * tic->v;
		
	}
	// if the triangle is lit...
	if (cp > 0.0f) {
		Vector LI;
		LI.make_vector(I, L);

		ray_one = ray* - fsqrt(rlsrcp);

		// diffuse multiplier in `cp'
		cp = -(Normal * LI)/LI.length();

		// calculate light reflection
		Vector ray_two;
		Vector temp = Normal * (LI * Normal);
		ray_two.macc(LI, temp, -2.0);

		// calculate phong illumination
		specular_power = ((ray_one * ray_two)/ray_two.length());
		//printf("[%.2f] ", product);
		if (specular_power < 0.0f) {
			//printf("product < 0!!!\n");
			specular_power = 0.0f;
		}
		// add up specular and diffuse
		specular_power *= 1.08f;
		specular_power *= specular_power;
		specular_power *= specular_power;
		specular_power *= specular_power;
		specular_power *= specular_power;
		specular_power *= specular_power;
		
		if (light.mode == LIGHTMAP) {
			float xm = 1.0f - light.shadow_density(I);
			specular_power *= xm;
			cp *= xm;
		}
		
		intensity = specular * specular_power + diffuse*cp + ambient;
	}
	Uint32 kolor;
	if (flags & MAPPED) {
		// get texture coordinates and fetch a texel
		RawImg *map = mesh[get_mesh_index()].image;
		Uint32 *data = (Uint32*)map->get_data();
		unsigned xx = map->get_x();
		unsigned yy = map->get_y();
		float x, y;
		x = mapcoords[0][0] + ma[0] * tic->u + mb[0] * tic->v;
		y = mapcoords[0][1] + ma[1] * tic->u + mb[1] * tic->v;
		Uint32 offset = ((int)(x * xx)) + ((int) (y * yy)) * xx;
		kolor = data[offset>=xx*yy?offset-xx*yy:offset];
	} else {
		kolor = this->color;
	}
	int result = multiplycolor(kolor, (int) (intensity*100000.0f));
#ifdef MAKE_PROFILING
	if (!iteration) {
		prof_leave(PROF_SOLVE3D);
		prof_enter(PROF_RAYTRACE);
	}
#endif
	if (flags & RAYTRACED && CVars::photomode) {
		//calculate the reflected ray
		Uint32 reflected = 0, refracted = 0;
		if (refl > 0.01f || (flags & FRESNEL)) {
			Vector temp = Normal * (ray * Normal);
			ray_reflected.macc(ray, temp, -2.0);

			finfo.IP = I;
			reflected = Raytrace(I, ray_reflected, flags & RECURSIVE | RAYTRACE_BILINEAR_MASK, 
					     iteration +1, this, finfo, gloss);
//			result = blend(
//				, result, refl);
		}
		// calculate the refracted ray
		if (flags & SEETHROUGH) {
			Vector xray = refract(ray, Normal, 1.0/1.5);
			Vector myI = I;
			bool good = false;
			int max_bounces = CVars::photomode ? 100 : 8;
			// trace up to 100 bounces inside the object
			for (int probe = 0; probe < max_bounces; probe++) {
				double mind = 1e99;
				Triangle *is = NULL;
				char ctx[128];
				// for each bounce see the next triangle which the ray hits
				for (int i = 0; i < mesh[get_mesh_index()].triangle_count; i++) {
					Triangle *t = trio + mesh[get_mesh_index()].get_triangle_base() + i;
					if (t == this) continue;
					double d;
					if (t->intersect(xray, myI, ctx) && ((d = t->intersection_dist(ctx))>1e-4)) {
						if (d < mind) {
							is = t;
							mind = d;
						}
					}
				}
				if (!is) {
					// no triangle? bad, but possible, will handle later
					break;
				}
				is->intersect(xray, myI, ctx);
				// obtain normal at intersection point. Since the normal
				// is usually meant to be on the other side, just flip it
				triangle_intersect_context *mytic = (triangle_intersect_context*) ctx;
				Vector Ni;
				if (flags & NORMAL_MAP) {
					Mesh *m = mesh + (get_mesh_index());
					Vector xu, xv;
					xu.make_vector(m->normal_map[is->nm_index[1]], m->normal_map[is->nm_index[0]]);
					xv.make_vector(m->normal_map[is->nm_index[2]], m->normal_map[is->nm_index[0]]);
					Ni.add3(
							m->normal_map[is->nm_index[0]],
					xu*(mytic->u),
					xv*(mytic->v)
						   );
					Ni.norm();
				} else {
					Ni = is->normal;
				}
				// Reflection or refraction? See what happens
				Ni.scale(-1.0);
				myI.macc(is->vertex[0], is->a, mytic->u);
				myI += is->b * mytic->v;
				Vector newray = refract(xray, Ni, 1.5);
				if (newray.lengthSqr() > 0) {
					xray = newray;
					good = true;
					break;
				} else {
					xray += Ni * ((Ni * xray) * -2.0);
					xray.norm();
				}
			}
			int newflags = flags & RECURSIVE | RAYTRACE_BILINEAR_MASK;
			if (!good) newflags &= ~RECURSE_SELF;
			finfo.IP = myI;
			finfo.lectionType = REFRACTION;
			
			refracted = Raytrace(myI, xray, newflags, iteration + 1, this, finfo, gloss);

			//result = blend(
					//, result, opacity);
		}
		if (flags & FRESNEL) {
			Uint32 total_rr = blend(refracted, reflected, fresnel(-(ray * Normal)));
			specular_power *= 0.1f;
			specular_power *= specular_power;
			specular_power *= specular_power;
			Uint32 temp_spec = multiplycolor(result, (int) (65536 * specular_power));
			result = blend(result, total_rr, this->opacity);
			result = add_clamp_rgb(temp_spec, result);
		} else {
			if (refl > 0.01f)
				result = blend(reflected, result, refl);
			if (flags & SEETHROUGH)
				result = blend(result, refracted, this->opacity);
		}
	}
#ifdef MAKE_PROFILING
	if (!iteration) prof_leave(PROF_RAYTRACE);
#endif
	return result;
}
int Triangle::get_best_miplevel(double x0, double z0, FilteringInfo & fi)
{
	if (fi.lectionType == REFRACTION) return 1;
	int& last_val = fi.last_val;
	Vector through0, through1;
	through0.add2(fi.through, fi.xinc);
	through1.add2(fi.through, fi.yinc);
	Vector v0, v1;
	v0.make_vector(through0, fi.camera);
	v1.make_vector(through1, fi.camera);
	//
	Vector temp0, temp1;
	if (flags & NORMAL_MAP) {
		Vector normal0, normal1;
		triangle_intersect_context tic;
		v0.norm();
		v1.norm();
		fastintersect(v0, fi.camera, 1.0, &tic);
		Mesh *m = mesh + (get_mesh_index());
		Vector xu, xv;
		xu.make_vector(m->normal_map[nm_index[1]], m->normal_map[nm_index[0]]);
		xv.make_vector(m->normal_map[nm_index[2]], m->normal_map[nm_index[0]]);
		normal0.add3(
				m->normal_map[nm_index[0]],
				xu*(tic.u),
				xv*(tic.v)
			   );
		normal0.norm();
		fastintersect(v1, fi.camera, 1.0, &tic);
		normal1.add3(
				m->normal_map[nm_index[0]],
				xu*(tic.u),
				xv*(tic.v)
			   );
		normal1.norm();
		temp0 = normal0 * (v0 * normal0);
		temp1 = normal1 * (v1 * normal1);
	} else {
		temp0 = normal * (v0 * normal);
		temp1 = normal * (v1 * normal);
	}
	temp0.scale(-2);
	temp1.scale(-2);
	v0 += temp0;
	v1 += temp1;
//
	double ydist0 = fabs(v0.v[1]), ydist1 = fabs(v1.v[1]);
	if ( ydist0 < DST_THRESHOLD || ydist1 < DST_THRESHOLD )
		return (LOG2_MAX_TEXTURE_SIZE);
	double dp0, dp1, scalefactor0, scalefactor1;
	dp0		 = v0.v[1] < 0.0f ? -daFloor : + daCeiling;
	dp1		 = v1.v[1] < 0.0f ? -daFloor : + daCeiling;
	dp0		-= copysign(fi.IP.v[1], v0.v[1]);
	dp1		-= copysign(fi.IP.v[1], v1.v[1]);
	scalefactor0	 = dp0 / ydist0;
	scalefactor1	 = dp1 / ydist1;
	double x1, z1, x2, z2;
	x1 = fi.IP[0] + v0[0] * scalefactor0;
	z1 = fi.IP[2] + v0[2] * scalefactor0;
	x2 = fi.IP[0] + v1[0] * scalefactor1;
	z2 = fi.IP[2] + v1[2] * scalefactor1;
	float fin_area = fabs(x0 * z1 + x1 * z2 + x2 * z0 - x2 * z1 - x1 * z0 - x0 * z2);
	last_val = (int) (0.5f*log2(fin_area*18.14f));
	if (last_val > LOG2_MAX_TEXTURE_SIZE)
		last_val = LOG2_MAX_TEXTURE_SIZE;
	if (last_val < 0)
		last_val = 0;
	return last_val;
}

/** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Old Vanilla C functions                                                  *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
void create_triangle_array(void)
{
	if (spherecount == 6) {
		/*
		mesh[0].ReadFromObj("data/truncated_cube.obj");
		mesh[0].scale(50);
		mesh[0].translate(Vector(120, 30, 200));
		mesh[0].bound(1, daFloor, daCeiling);
		mesh[0].translate(Vector(0, 1, 0));
		mesh[0].set_flags( RAYTRACED | SEETHROUGH , 0.0026, 0.5, TYPE_OR, 0xdbf6e3);
		mesh_count = 1;
		*/
		mesh[0].read_from_obj("data/medusa.obj");
		mesh[0].scale(75);
		mesh[0].translate(Vector(120, 10, 200));
		mesh[0].bound(1, daFloor, daCeiling);
		mesh[0].translate(Vector(0, 1, 0));
		mesh[0].set_flags( NORMAL_MAP, 0.26, 0.5, TYPE_OR, 0xdbf6e3);
		mesh_count = 1;

	} else {
		mesh[0].read_from_obj("data/3pyramid.obj");
		mesh[0].scale(66);
		mesh[0].translate(Vector(200, 0, 330));
		mesh[0].bound(1, daFloor, daCeiling);
		mesh[0].translate(Vector(0, 1, 0));
		mesh[0].set_flags( RAYTRACED | RECURSIVE, 0.75, 0.0, TYPE_OR, 0xeeeeee);
		mesh_count = 1;
	}
}

void triangle_close(void)
{
	for (int i = 0; i < MAX_TRIANGLES; i++) {
		trio[i].color = 0;
		trio[i].flags = 0;
		trio[i].opacity = 0;
		trio[i].refl = 0;
	}
}

void retransformate(float ma[], float mb[], float h0, float h1, float& p, float &q)
{
	float Dcr = ma[0] * mb[1] - ma[1] * mb[0];
	if (fabs(Dcr) < 1e-5) {
		p = q = 0.0f;
		return;
	}
	float rDcr = 1.0f / Dcr;
	p = (h0 * mb[1] - h1 * mb[0]) * rDcr;
	q = (ma[0] * h1 - ma[1] * h0) * rDcr;
}

