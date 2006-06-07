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

// ********************** GLOBAL STORAGE ***********************************/
Triangle trio[MAX_TRIANGLES];
extern int spherecount;

// ********************* Utility functions *********************************/


float noise(unsigned x)
{
	x = (x<<13) ^ x;
	return ( 1.0 - ( (x * (x * x * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0);
}
/// perlin:
/// generates fractal perlin noise
/// @param x - should be in [0..1]
/// @param y - should be in [0..1]
/// @returns a float [-1..+1]
float perlin(float x, float y)
{
	float res = 0.0f, amp = 1.0f;
	unsigned size = 1;
	for (int i = 0; i <6; i++, size*=2) {
		unsigned x0 = (unsigned) (x*size);
		unsigned y0 = (unsigned) (y*size);
		unsigned q = x0 + y0 *size;
		float fx = x*size - x0;
		float fy = y*size - y0;
		float nf = noise(q       )*(1.0f-fx)*(1.0f-fy) +
		           noise(q+1     )*(     fx)*(1.0f-fy) +
			   noise(q  +size)*(1.0f-fx)*(     fy) +
			   noise(q+1+size)*(     fx)*(     fy);
		res += amp * nf;
		amp *= 0.72;
	}
	return res;
}

/** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * @class Triangle                                                          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/

int Triangle::GetTriangleIndex() const
{
	return (this - trio);
}

int Triangle::GetMeshIndex() const
{
	return (this - trio) >> TRI_ID_BITS;
}

double Triangle::GetDepth(const Vector &camera)
{
	return ZDepth;
}

bool Triangle::OkPlane(const Vector & camera)
{
	Vector vec;
	vec.make_vector(center, camera);
	return (vec * normal) < 0.0;
}


bool Triangle::IsVisible(const Vector & camera, Vector w[3])
{
	if (visited) return visible;
	visited = true;
	bool vert_in_screen = false;
	int x, y, xr, yr;
	xr = xsize_render(xres());
	yr = ysize_render(yres());
	for (int i = 0; i < 3; i++) {
		if (!ProjectPoint(&x, &y, vertex[i], camera, w, xr, yr)) {
			visible = false;
			return false;
		}
		if (x >= 0 && x < xr && y >= 0 && y < yr) vert_in_screen = true;
	}
	if (!vert_in_screen) {
		visible = false;
		return false;
	}
	visible = OkPlane(camera);
	return visible;
}


int Triangle::CalculateConvex(Vector pt[], Vector camera)
{
	for (int i = 0; i < 3; i++)
		pt[i] = vertex[2-i];
	return 3;
}

void Triangle::MapToScreen(
			   Uint32 *framebuffer, 
			   int color, 
			   int sides, 
			   Vector pt[], 
			   Vector camera, 
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
	ProjectHullPart(L, pt, bs, +1, size, sides, color, camera, w, fun_min, fround);
	ProjectHullPart(R, pt, bs, -1, sides - size, sides, color, camera, w, fun_max);
	MapHull(framebuffer, L, R, ys, ye, color, 0);
}

bool Triangle::FastIntersect(const Vector& ray, const Vector& camera, const double& rls, void *IntersectContext) const
{
	if (!visible) return false;
	triangle_intersect_context *tic = (triangle_intersect_context*)IntersectContext;
#ifdef __SSE__
	float Dcr= ray * crossproduct;
#else
	double Dcr = ray * crossproduct;
#endif
	tic->u = (hb * ray);
	tic->v = (ah * ray);
	if (fabs(Dcr) < 1e-9 || tic->u * Dcr < 0 || tic->v * Dcr < 0) return false;


#ifdef __SSE__
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

bool Triangle::Intersect(const Vector& ray, const Vector &camera, void *IntersectContext)
{
	triangle_intersect_context *tic = (triangle_intersect_context*)IntersectContext;
	Vector h;
	h.make_vector( camera, vertex[0] );
	Vector hr = h ^ ray;
#ifdef __SSE__
	float Dcr= ray * crossproduct;
#else
	double Dcr = ray * crossproduct;
#endif
	tic->u = -(b * hr);
	tic->v =  (a * hr);
	if (fabs(Dcr) < 1e-9 || tic->u * Dcr < 0 || tic->v * Dcr < 0) return false;
#ifdef __SSE__
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

double Triangle::IntersectionDist(void *IntersectContext) const
{
	triangle_intersect_context *tic = (triangle_intersect_context*)IntersectContext;

	return tic->dist;
}

Uint32 Triangle::Solve3D(Vector& ray, const Vector& camera, const Vector& light, double rlsrcp,
		float *opacity, void *IntersectContext, int iteration, FilteringInfo& finfo)
{
	Vector CI;
	Vector ray_one, ray_reflected;
	Vector I;
	triangle_intersect_context *tic = (triangle_intersect_context*)IntersectContext;

#ifdef MAKE_PROFILING
	if (!iteration) prof_enter(PROF_SOLVE3D);
#endif

	*opacity = 1.0f;
	float intensity = ambient;
	if (flags & STOCHASTIC) {
		intensity += ambient * 0.5 * perlin(tic->u, tic->v);
	}
	Vector Normal;
	if (flags & NORMAL_MAP) {
		Mesh *m = mesh + (GetMeshIndex());
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

	Vector lightray;
	lightray.make_vector(vertex[0], light);
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
		LI.make_vector(I, light);

		ray_one = ray* - fsqrt(rlsrcp);

		// diffuse multiplier in `cp'
		cp = -(Normal * LI)/LI.length();

		// calculate light reflection
		Vector ray_two;
		Vector temp = Normal * (LI * Normal);
		ray_two.macc(LI, temp, -2.0);

		// calculate phong illumination
		float product = ((ray_one * ray_two)/ray_two.length());
		//printf("[%.2f] ", product);
		if (product < 0.0f) {
			//printf("product < 0!!!\n");
			product = 0.0f;
		}
		// add up specular and diffuse
		product *= 1.08f;
		product *= product;
		product *= product;
		product *= product;
		product *= product;
		product *= product;
		intensity = specular * product + diffuse*cp + ambient;
	}
	Uint32 kolor;
	if (flags & MAPPED) {
		// get texture coordinates and fetch a texel
		RawImg *map = mesh[GetMeshIndex()].image;
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
	if (flags & RAYTRACED) {
		//calculate the reflected ray
		if (refl > 0.01f) {
			Vector temp = Normal * (ray * Normal);
			ray_reflected.macc(ray, temp, -2.0);

			finfo.IP = I;
			result = blend(
				Raytrace(I, ray_reflected, flags & RECURSIVE | RAYTRACE_BILINEAR_MASK, 
					 iteration +1, this, finfo), result, refl);
		}
		// calculate the refracted ray
		if (flags & SEETHROUGH) {
			Vector Cp;
			Vector vis;
			vis.make_vector(I, camera);
			double vis_len_sqr = vis.lengthSqr();
			double I_Cp_len = -(vis * Normal);
			Cp.macc(I, Normal, I_Cp_len);
			Cp.scale(-1);
			Cp += camera;
			double C_Cp_len = sqrt(vis_len_sqr - I_Cp_len * I_Cp_len);
			double vis_len = sqrt(vis_len_sqr);
			double sin_alpha = C_Cp_len / vis_len;
			//double sin_beta = REFRACT_FACTOR * sin_alpha;
			//double x_div_a = REFRACT_FACTOR * sqrt((1 - sin_alpha * sin_alpha)/(1 - sin_beta * sin_beta));
			double x_div_a = REFRACT_FACTOR * sqrt( 1- sin_alpha * sin_alpha * sin_alpha );
			Vector out;
			out.macc(vis, Cp, 1 - x_div_a);
			out.norm();
			// inside vector found. See where it breaks out
			Mesh &M = mesh[GetMeshIndex()];
			int tbase = M.GetTriangleBase();
			double totdist = 0;
			bool found = false;
			Triangle *last_tri = this;
			for (int inner_hits = 0; inner_hits < 6; inner_hits++)
			{
				double mindist = 1e99;
				Triangle *bt = NULL;
				Vector O;
				int bestj = 0;
				if (tic->rfc_size < 0 || tic->rfc_size > REFRACT_CACHE_SIZE) {
					tic->rfc_size = 0;
				}
				for (int j = 0; j < tic->rfc_size; j++) {
					int i = tic->last_refract[j];
					if (i < 0 || i >= M.triangle_count) {
						tic->rfc_size = 0;
						break;
					}
					Triangle *t = trio + tbase + i;
					triangle_intersect_context icontext;
					if (t != last_tri) {
						if (t->normal * out < 0 || !t->Intersect(out, I, &icontext)) continue;
						double dist = t->IntersectionDist(&icontext);
						if (dist > 0 && dist < mindist) {
							mindist = dist;
							bt = t;
							O.macc(t->vertex[0], t->a, icontext.u);
							O += t->b * icontext.v;
							bestj = j;
							if ((flags & RECURSE_SELF) == 0) {
								break;
							}
						}
					}
				}
				if (bt && bestj) {
					int x = tic->last_refract[0];
					tic->last_refract[0] = tic->last_refract[bestj];
					tic->last_refract[bestj] = x;
				}
				if (!bt) {
					for (int i = 0; i < M.triangle_count; i++) {
						Triangle *t = trio + tbase + i;
						triangle_intersect_context icontext;
						if (t != last_tri) {
							if (t->normal * out < 0 || !t->Intersect(out, I, &icontext)) continue;
							double dist = t->IntersectionDist(&icontext);
							if (dist > 0 && dist < mindist) {
								mindist = dist;
								bt = t;
								O.macc(t->vertex[0], t->a, icontext.u);
								O += t->b * icontext.v;
								
								tic->rfc_size += (tic->rfc_size == 0);
								if (tic->rfc_size < REFRACT_CACHE_SIZE) {
									tic->last_refract[tic->rfc_size++] = tic->last_refract[0];
								}
								tic->last_refract[0] = i;
								if ((flags & RECURSE_SELF) == 0) break;
							}
						}
					}
				}
				if (bt) {
					Vector Ip;
					//
					totdist += mindist;
					double o_ip_dist = mindist * (bt->normal * out);
					Ip.macc(O, bt->normal, -o_ip_dist);
					double i_ip_dist = sqrt(mindist * mindist - o_ip_dist * o_ip_dist);
					double sin_gamma = i_ip_dist / mindist;
					double sin_delta = REFRACT_FACTOR_RECIPROCAL * sin_gamma;
					if (sin_delta > 0.9999998) {
						//result = blend(65535, result, this->opacity);
						double temp = out * bt->normal;
						out += bt->normal * (-2.0 * temp);
						out.norm();
						I = O;
						last_tri = bt;
					} else {
						double a_div_x = 
								REFRACT_FACTOR_RECIPROCAL * sqrt((1 - sqr(sin_gamma))/
									(1 - sqr(sin_delta)));
						Vector q;
						q.make_vector(O,I);
						Vector temp;
						temp.make_vector(Ip, I);
						q += temp * (a_div_x-1);
						result = blend (
								Raytrace(O, q, flags & RECURSIVE | RAYTRACE_BILINEAR_MASK,
								iteration + 3, this, finfo), result, this->opacity
					       		);
						found = true;
						break;
					}
				} else {
					result = blend(0xff0000, result, this->opacity);
					break;
				}
			}
			if (!found)
				result = blend(0xff, result, this->opacity);
		}
	}
#ifdef MAKE_PROFILING
	if (!iteration) prof_leave(PROF_RAYTRACE);
#endif
	return result;
}
int Triangle::GetBestMipLevel(double x0, double z0, FilteringInfo & fi)
{
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
		FastIntersect(v0, fi.camera, 1.0, &tic);
		Mesh *m = mesh + (GetMeshIndex());
		Vector xu, xv;
		xu.make_vector(m->normal_map[nm_index[1]], m->normal_map[nm_index[0]]);
		xv.make_vector(m->normal_map[nm_index[2]], m->normal_map[nm_index[0]]);
		normal0.add3(
				m->normal_map[nm_index[0]],
				xu*(tic.u),
				xv*(tic.v)
			   );
		normal0.norm();
		FastIntersect(v1, fi.camera, 1.0, &tic);
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
		mesh[0].ReadFromObj("data/medusa.obj");
		mesh[0].scale(75);
		mesh[0].translate(Vector(120, 10, 200));
		mesh[0].bound(1, daFloor, daCeiling);
		mesh[0].translate(Vector(0, 1, 0));
		mesh[0].set_flags( 0, 0.26, 0.5, TYPE_OR, 0xdbf6e3);
		mesh_count = 1;

	} else {
		mesh[0].ReadFromObj("data/3pyramid.obj");
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
