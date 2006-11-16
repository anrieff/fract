/***************************************************************************
 *   Copyright (C) 2004 by Veselin Georgiev                                *
 *   vesko@ViruS                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *-------------------------------------------------------------------------*
 *   Renders voxel scenes                                                  *
 ***************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include "MyGlobal.h"
#include "antialias.h"
#include "bitmap.h"
#include "memory.h"
#include "cmdline.h"
#include "cross_vars.h"
#include "cpu.h"
#include "cvars.h"
#include "fract.h"
#include "gfx.h"
#include "infinite_plane.h"
#include "profile.h"
#include "progress.h"
#include "render.h"
#include "threads.h"
#include "vector3.h"
#include "vector2f.h"
#include "vectormath.h"
#include "voxel.h"
#include "light.h"
#include "shaders.h"
#include "physics.h"

//#define BENCH

// we need those neat class...
#include "hierarchy.h"
#define MAX_HITS 5

/** !!!!!!!------------ Constants -------------- -!!!!!!!!!!!!!!!!!!!!!!!**/
info_t vxInput[NUM_VOXELS] = {
	//{ "data/heightmap_f.fda", "data/aardfdry256.bmp", 0, 0.5, 1},
	//{ "data/heightmap_c.fda", "data/aardfdry256.bmp", 200, 0.5, 0}
	
//FOR RADIOSITY:
//	{ "data/down.fda", "data/whitefloor.bmp", 0, 0.50, 1},
//	{ "data/up.fda", "data/whiteceil.bmp", 100, 0.50, 0},


//NORMAL
	{ "data/down.fda", "data/brt_down.bmp", 0, 0.50, 1},
	{ "data/up.fda", "data/brt_up.bmp", 100, 0.50, 0},

	//{ "data/heightmap_c.fda", "data/aardfdry256.bmp", 200, 0.5, 0}
};

/** --------=========== Prototypes ============= ------------------------**/

void voxel_precalc(void);

/** --------=========== Variables ============== ------------------------**/

vox_t vox[NUM_VOXELS];
float zbuffer[RES_MAXX * RES_MAXY];
Uint32 * voxfb;
int vox_xr, vox_yr;
const int render_dist_min = 300;
const int render_dist_max = 1500;
bool reciprocals_precalculated = false;
float adapt = 10.0f;
Vector gti, gtti;
int voxel_rendering_method = 1, shadow_casting_method = 0;
bool light_moving = false;
bool must_recalc_light = false;
bool shooting;

Barrier bar1(0), lightOuter(0), bar(0), precalculation(0);



/** --------=========== Precalculated Storage == ------------------------**/

// to get 1/x consult prec_rcp_x[x*RESOLUTION];
static float prec_rcp_x[PREC_MAX_DIST*RESOLUTION];

// to get 1/(x*sqrt(x)) consult prec_rcp_x[x*RESOLUTION];
static float prec_rsqrt_x[PREC_MAX_DIST*RESOLUTION];

void vx_free(int current_index)
{
	int i;
	for (i=0;i<current_index;i++) {
		free(vox[i].heightmap);
		free(vox[i].texture);
		free(vox[i].input_texture);
		sse_free(vox[i].normal_map);
	}
}

void err_tex(const char *missing_file, int current_index)
{
	vx_free(current_index);
	printf("Could not load needed texture file: %s\n", missing_file);
}

int texture_check(RawImg *a, const char *fn)
{
	if (a->get_x() != a->get_y() || power_of_2(a->get_x())==-1) {
		printf("Texture file (%s) seems to be invalid...\n", fn);
		return 1;
	}
	return 0;
}

int fiabs(int a)
{
	return (a<0)?-a:a;
}

// converts from double to fixedpoint int 16.16
static inline int dbl2int16(double x)
{
	return (int) (x * 65536.0 + 0.5);
}

// filters four ints with bilinear filtering. x and y are .16 fixedpoints
static inline double filter4(unsigned x, unsigned y, float x0y0, float x1y0, float x0y1, float x1y1)
{
	return (((((65535u-x)*(65535u-y) >> 16)) * x0y0 +
		 (((       x)*(65535u-y) >> 16)) * x1y0 +
		 (((65535u-x)*(       y) >> 16)) * x0y1 +
		 (((       x)*(       y) >> 16)) * x1y1)
	)*0.0000152587890625; // divide by 65536.0
}

static void create_loft(float * fmap, Uint32 *cmap, int n, vec2f center)
{
	vec2f mc(n/2.0f, n/2.0f);
	vec2f d1 = mc - center; d1.norm();
	for (int j = 0; j < n; j++) {
		for (int i = 0; i < n; i++) {
			vec2f v(i, j);
			vec2f d2 = v - center;
			if (d2.length() > 0) d2.norm();
			float eff_radius = 55.0f + 20.0f * (d1 * d2);
			float f = v.distto(center);
			if (f < eff_radius) {
				f /= eff_radius; f = 1.0f - f;
				f = sin(f * M_PI / 2.0f);
				fmap[j*n+i] += f * 240.0f;
				cmap[j*n+i] = multiplycolorf(cmap[j*n+i], 1.0f-f*0.75f);
			}
		}
	}
}

int voxel_init(void) //returns 0 on success, 1 on failure
{
	int k, i;
	RawImg a, b;
	for (k = 0; k < NUM_VOXELS; k++) {
		if (!a.load_fda(vxInput[k].heightmap)) {
			err_tex(vxInput[k].heightmap, k);
			return 1;
		}
		if (!b.load_bmp(vxInput[k].texture)) {
			err_tex(vxInput[k].texture, k);
			return 1;
		}

		//gridify_texture(b);

		if (texture_check(&a, vxInput[k].heightmap) || texture_check(&b, vxInput[k].texture)) {
			vx_free(k);
			return 1;
		}
		vox[k].floor = vxInput[k].is_floor;
		vox[k].size = a.get_x();
		vox[k].yoffs = (int) vxInput[k].y_offset;
		vox[k].texture = (Uint32 *) malloc(b.get_size()*sizeof(Uint32));
		if (!vox[k].texture) {
			printf("Voxel_init: out of memory!!!!\n");
			return 1;
		}
		memcpy(vox[k].texture, b.get_data(), b.get_size()*sizeof(Uint32));
		vox[k].input_texture = (Uint32 *) malloc(b.get_size()*sizeof(Uint32));
		if (!vox[k].input_texture) {
			printf("Voxel_init: out of memory!!!!\n");
			free(vox[k].texture);
			return 1;
		}
		memcpy(vox[k].input_texture, b.get_data(), b.get_size()*sizeof(Uint32));
		vox[k].raytrace_mip = (Uint32 *)malloc((b.get_size()>>
			(2*HEIGHTFIELD_RAYTRACE_MIPLEVEL))*sizeof(Uint32));

		vox[k].heightmap = (float *) malloc(a.get_size()*sizeof(float));
		if (!vox[k].heightmap) {
			printf("Voxel_init: out of memory!!!!\n");
			free(vox[k].texture);
			free(vox[k].input_texture);
			return 1;
		}
		memcpy(vox[k].heightmap, a.get_data(), b.get_size()*sizeof(float));
		
		if (option_exists("--loft") && k == 1) {
			create_loft(vox[k].heightmap, vox[k].texture, vox[k].size, vec2f(182, 382));
		}
		
		if (!option_exists("--nosmooth") && !option_exists("--no-smooth")) {
			blur_array_2d(vox[k].heightmap, vox[k].size, 5);
		}
		
		double med_val = 0.0;
		for (i=0;i<a.get_size();i++) {
			med_val += (vox[k].heightmap[i] = vox[k].yoffs + vox[k].heightmap[i]*vxInput[k].scale);
		}
		med_val /= (vox[k].size * vox[k].size);
		vox[k].med_val = med_val;
	}
	
	for (i = 0; i < a.get_size(); i++) {
		if (vox[0].heightmap[i] > vox[1].heightmap[i]) {
			float mid = (vox[0].heightmap[i] + vox[1].heightmap[i]) * 0.5;
			vox[0].heightmap[i] = vox[1].heightmap[i] = mid;
		}
	}
	
	for (k = 0; k < NUM_VOXELS; k++) {
		vox[k].hierarchy.build_hierarchy(vox[k].size, vox[k].heightmap, vox[k].floor);
	}
	voxel_precalc();
	voxel_water_init();
	return 0;
}

static inline float get_height(float *p, int sz, double x, double z, float clamp)
{
	if (x < 1.0 || x > sz-2.0 || z < 1.0 || z > sz-2.0) {
		return clamp;
	}
	int ix = (int) x, iz = (int) z;
	int base = iz*sz + ix;
	float px = x - ix, pz = z - iz;
	//return p[base];
	return  p[base         ]*(1.0f-px)*(1.0f-pz) +
		p[base + 1     ]*(     px)*(1.0f-pz) +
		p[base     + sz]*(1.0f-px)*(     pz) +
		p[base + 1 + sz]*(     px)*(     pz);
}

static inline void clip_coords(float & x, float & y, const float clp_size)
{
	if (x < 0.0) x = 0.0;
	if (x > clp_size) x = clp_size;

	if (y < 0.0) y = 0.0;
	if (y > clp_size) y = clp_size;
}

//tex_paint_scale(vox + k, index, x, y, ambient);

static inline void tex_paint_scale(vox_t * vox, int index, float x, float y, int scaling)
{
	vox->texture[index] = multiplycolor(vox->input_texture[index], scaling);
}

void voxel_precalc(void)
{
	int size, k;
	if (!reciprocals_precalculated) {
		reciprocals_precalculated = true;
		for (int i = 1 ; i < PREC_MAX_DIST * RESOLUTION; i++) {
			float x = (float) i / RESOLUTION;
			prec_rcp_x[i] = 1.0f / x ;
			prec_rsqrt_x[i] = 1.0f / (x*sqrt(x));
		}
		prec_rcp_x[0] = prec_rsqrt_x[0] = 1e9;
	}

	for (k = 0; k < NUM_VOXELS; k++) {
		size = vox[k].size;
		vox[k].normal_map = (float*) sse_malloc(size*size * __4*sizeof(float));
		memset(vox[k].normal_map, 0, size*size*__4*sizeof(float));
		for (int y = 0; y < size-1; y++) {
			for (int x = 0; x < size-1; x++) {
				Vector a, b;
				float 	org = vox[k].heightmap[y*size+x],
					xp1 = vox[k].heightmap[y*size+x+1],
					yp1 = vox[k].heightmap[y*size+x+size];
				a.make_vector(Vector(x+1, xp1, y), Vector(x, org, y));
				b.make_vector(Vector(x, yp1, y+1), Vector(x, org, y));
				a = a^b;
				a.norm();
				int index = __4*(y*size+x);
				vox[k].normal_map[index    ] = a[0];
				vox[k].normal_map[index + 1] = a[1];
				vox[k].normal_map[index + 2] = a[2];
			}
		}
	}

}

void voxel_light_recalc1(int thread_idx)
{
	int k, size;

	for (k = 0; k < NUM_VOXELS; k++) {
		size = vox[k].size;
		int rollsize = (int) (size * LIGHT_PRECALC_ACCURACY);
		float sMul = vox[k].floor ? +1 : -1;
		for (int sd = 0; sd < 4; sd++) {
			float ex = sd < 2 ? 0 : size - 1;
			float ey = ex;
			float dir = sd < 2 ? 1 : -1;
			dir /= LIGHT_PRECALC_ACCURACY;
			float exi = sd % 2 == 0 ? dir : 0  ;
			float eyi = sd % 2 == 0 ? 0   : dir;
			for (int i = 0; i < rollsize; i++, ex+=exi,ey+=eyi) if (i % cpu.count== thread_idx) {
				float x = lx(), y = lz();
				float dst = sqrt(sqr(ex-lx()) + sqr(ey-lz()));
				float rcp_dst = 1.0f / dst;
				float xi= (ex-lx()) * rcp_dst;
				float yi= (ey-lz()) * rcp_dst;
				float min_cotg = 1e19;
				int idist = 1;
				for (int tt = (int) dst; tt > 0; --tt, idist += 10, x += xi, y += yi) {
					clip_coords(x, y, size-1);
					int index = (((int) y) * size) + (int) x;
					float vDist = light.p[1] - get_height(vox[k].heightmap, size, x, y, FLOOR_LOW);
					float new_cotg = sMul*vDist * prec_rcp_x[idist];
					if (new_cotg < min_cotg) {
						min_cotg = new_cotg;
						//Vector ray(lx - x, ly-height, lz-y);
						float *vec = &vox[k].normal_map[index*__4];
						float vX = lx() - x, vZ = lz() - y;
						float ll = sqrt(vX*vX + vZ*vZ + vDist*vDist);

						float mul = (vX*vec[0]+ vDist*vec[1] + vZ * vec[2]) * 5.0 * prec_rsqrt_x[(int)(ll*RESOLUTION)];
						tex_paint_scale(vox + k, index, x, y, iambient + dbl2int16(fabs(mul)));
					} else {
						// the point is in shadow
						tex_paint_scale(vox + k, index, x, y, iambient);
					}
				}
			}
		}
	}
}

#define uint unsigned int
#define fastrsqrt FastReciprocalSquareRoot
static inline float fast_reciprocal_square_root(float val)
{
	union {
		float f;
		uint i;
	} temp;
	const float magicValue = 1597358720.0f;
	temp.f = val;
	temp.f = (float) temp.i;
	float tmp = temp.f;
	tmp = (tmp * -0.5) + magicValue;
	temp.i = (uint) tmp;
	return temp.f;
}

static inline bool point_is_in_shadow(vox_t *v, int x, int z)
{
	Vector l = light.p;
	Vector p(x, v->heightmap[x + z * v->size], z);
	Vector o;
	double d1 = l.distto(p);
	double d2 = v->hierarchy.ray_intersect(l, p, o);
	return (d2 < d1 - 1);
}

static inline unsigned get_state(vox_t *v, int x, int z)
{
	int sz = v->size;
	if (x >= sz || z >= sz) return 0x02;
	int index = x + z * sz;
	Uint32 &texel = v->texture[index];
	if (0xffffffff != texel) {
		return texel >> 24;
	}
	if (point_is_in_shadow(v, x, z)) { // the texel is exposed to light
		texel = 0x02000000;
		return 0x02;
	} else {
		texel = 0x01000000;
		return 0x01;
	}

}

void subdivshadow(vox_t *v, int x, int z, int sz)
{
	if (sz == 2) {
		int index = z * v->size + x;
		int state[4] = {
			get_state(v, x    , z    ),
			get_state(v, x + 2, z    ),
			get_state(v, x    , z + 2),
			get_state(v, x + 2, z + 2),
		};
		if (state[0]==0x01) {
			tex_paint_scale(v, index, x, z, iambient);
		} else {
			float *vec = &(v->normal_map[index*__4]);
			float vX = lx() - x, vZ = lz() - z;
			float vDist = light.p[1] - v->heightmap[index];
			float ll = sqrt(vX*vX + vZ*vZ + vDist*vDist);
			float mul = (vX*vec[0]+ vDist*vec[1] + vZ * vec[2]) * 5.0 * prec_rsqrt_x[(int)(ll*RESOLUTION)];
			tex_paint_scale(v, index, x, z, iambient + dbl2int16(fabs(mul)));
		}
		index++, x++;
		if (state[1]==0x01) {
			tex_paint_scale(v, index, x, z, iambient);
		} else {
			float *vec = &(v->normal_map[index*__4]);
			float vX = lx() - x, vZ = lz() - z;
			float vDist = light.p[1] - v->heightmap[index];
			float ll = sqrt(vX*vX + vZ*vZ + vDist*vDist);
			float mul = (vX*vec[0]+ vDist*vec[1] + vZ * vec[2]) * 5.0 * prec_rsqrt_x[(int)(ll*RESOLUTION)];
			tex_paint_scale(v, index, x, z, iambient + dbl2int16(fabs(mul)));
		}
		x--;
		index+=v->size-1;z++;
		if (state[2]==0x01) {
			tex_paint_scale(v, index, x, z, iambient);
		} else {
			float *vec = &(v->normal_map[index*__4]);
			float vX = lx() - x, vZ = lz() - z;
			float vDist = light.p[1] - v->heightmap[index];
			float ll = sqrt(vX*vX + vZ*vZ + vDist*vDist);
			float mul = (vX*vec[0]+ vDist*vec[1] + vZ * vec[2]) * 5.0 * prec_rsqrt_x[(int)(ll*RESOLUTION)];
			tex_paint_scale(v, index, x, z, iambient + dbl2int16(fabs(mul)));
		}
		index++, x++;
		if (state[3]==0x01) {
			tex_paint_scale(v, index, x, z, iambient);
		} else {
			float *vec = &(v->normal_map[index*__4]);
			float vX = lx() - x, vZ = lz() - z;
			float vDist = light.p[1] - v->heightmap[index];
			float ll = sqrt(vX*vX + vZ*vZ + vDist*vDist);
			float mul = (vX*vec[0]+ vDist*vec[1] + vZ * vec[2]) * 5.0 * prec_rsqrt_x[(int)(ll*RESOLUTION)];
			tex_paint_scale(v, index, x, z, iambient + dbl2int16(fabs(mul)));
		}
		return;
	}
	unsigned state = get_state(v, x, z);
	if (get_state(v, x + sz, z     ) == state &&
	    get_state(v, x     , z + sz) == state &&
	    get_state(v, x + sz, z + sz) == state) {
	    	if (state == 0x01) {// shadow
			int index = z * v->size + x;
			for (int j = 0; j < sz; j++, index+=v->size-sz)
				for (int i = 0; i < sz; i++, index++) {
					tex_paint_scale(v, index, x + i, z + j, iambient);
				}
		} else { //lit
			int index = z * v->size + x;
			for (int j = 0; j < sz; j++, index += v->size - sz)
				for (int i = 0; i < sz; i++, index++) {
//					float ll = light.distto(c);
					float *vec = &(v->normal_map[index*__4]);
					float vX = lx() - x, vZ = lz() - z;
					float vDist = light.p[1] - v->heightmap[index];
					float ll = sqrt(vX*vX + vZ*vZ + vDist*vDist);

					float mul = (vX*vec[0]+ vDist*vec[1] + vZ * vec[2]) * 5.0 * prec_rsqrt_x[(int)(ll*RESOLUTION)];

					tex_paint_scale(v, index, x, z, iambient + dbl2int16(fabs(mul)));
				}
		}
		return;
	    }
	sz /= 2;
	subdivshadow(v, x     , z     , sz);
	subdivshadow(v, x + sz, z     , sz);
	subdivshadow(v, x     , z + sz, sz);
	subdivshadow(v, x + sz, z + sz, sz);
}

void voxel_light_recalc2(int thread_idx)
{
	int k, size;

	for (k = 0; k < NUM_VOXELS; k++) {
		size = vox[k].size;
		multithreaded_memset(vox[k].texture, 0xffffffff, size*size, thread_idx, cpu.count);
		/* explanation:
			If a texel has 0xff in its MSB (alpha channel) it is still
			not determined whether it is in shadow or not.

			0x01 in the MSB -> under shadow
			0x02 or 0x00 in the MSB -> lit

		*/
		/*
		int index = 0;
		for (int z = 0; z < size; z++)
			for (int x = 0; x < size; x++) {
				Vector c(x, vox[k].heightmap[index], z), crossing;
				index = z * size + x;
				float ll = light.distto(c);
				float res = vox[k].hierarchy.ray_intersect(light, c, crossing);
				if (res > ll - 1.0f) { // the texel is exposed to light
					float *vec = &vox[k].normal_map[index*__4];
					float vX = lx - x, vZ = lz - z;
					float vDist = ly - vox[k].heightmap[index];

					float mul = (vX*vec[0]+ vDist*vec[1] + vZ * vec[2]) * 5.0 * prec_rsqrt_x[(int)(ll*RESOLUTION)];

					tex_paint_scale(vox + k, index, x, z, iambient + dbl2int16(fabs(mul)));
				} else {
					// the point is in shadow
					tex_paint_scale(vox + k, index, x, z, iambient);
				}

			}*/
		for (int z = 0,all=0; z < size; z += SHADOW_CAST_ACCURACY)
			for (int x = 0; x < size; x += SHADOW_CAST_ACCURACY,all++) if (all % cpu.count == thread_idx) {

				vox[k].texture[x + z * size] = point_is_in_shadow(vox + k, x, z) ? 0x01000000 : 0x02000000;

		}
		bar1.checkout();
		for (int z = 0, all=0; z < size; z += SHADOW_CAST_ACCURACY)
			for (int x = 0; x < size; x += SHADOW_CAST_ACCURACY,all++) if (all % cpu.count == thread_idx) {
				subdivshadow(vox + k, x, z, SHADOW_CAST_ACCURACY);
			}
	}
}

void recalc_miplevel(int thread_idx)
{
	int r, g, b, p;
	// FIXME (for multithreading):
	if (thread_idx) return;
	int smsize = 1 << HEIGHTFIELD_RAYTRACE_MIPLEVEL;
	for (int k = 0; k < NUM_VOXELS; k++) {
		p = 0;
		int lgsize = vox[k].size / smsize;
		for (int i = 0; i < lgsize; i++) {
			for (int j = 0; j < lgsize; j++,p++) {
				r = g = b = 0;
				int base = j * smsize + (i*smsize * vox[k].size);
				for (int ii = 0; ii < smsize; ii++) {
					for (int jj=0; jj < smsize; jj++,base++) {
						b += vox[k].texture[base] & 0xff;
						g += vox[k].texture[base] & 0xff00;
						r += vox[k].texture[base] & 0xff0000;
					}
					base += vox[k].size - smsize;
				}
				vox[k].raytrace_mip[p] =
					((r >> (2*HEIGHTFIELD_RAYTRACE_MIPLEVEL)) & 0xff0000) |
					((g >> (2*HEIGHTFIELD_RAYTRACE_MIPLEVEL)) & 0x00ff00) |
					((b >> (2*HEIGHTFIELD_RAYTRACE_MIPLEVEL)) & 0x0000ff) ;
			}
		}
	}
}

void voxel_light_recalc(int thread_idx)
{
	if (!must_recalc_light) return;
	if (shadow_casting_method) {
		voxel_light_recalc2(thread_idx);
	} else {
		voxel_light_recalc1(thread_idx);
	}
	recalc_miplevel(thread_idx);
	lightOuter.checkout();
	if (0 == thread_idx) {
		for (int k = 0; k < NUM_VOXELS; k++)
			shader_blur(vox[k].texture, vox[k].texture, vox[k].size, vox[k].size, 5);
	}
}


void show_light(Uint32 *fb, int xr, int yr)
{
	Vector w[3];
	calc_grid_basics(cur, CVars::alpha, CVars::beta, w);
	int x, y;
	project_point(&x, &y, light.p, cur, w, xr, yr);
	if (x >= 0 && x < xr && y >= 0 && y < yr)
		fb[xr * y + x ] = 0xffffff;
}


//static void subdiv(Vector 


struct AdaptiveVoxelRenderer {
	Uint32 *fb;
	int xr, yr, ires;
	Vector tt, ti, tti;
	Vector W[3];
	InterlockedInt bruteray;


	struct RasterSample {
		int y;
		Uint32 color;
		void set(int _y, Uint32 _color) { y = _y; color = _color; }
	};

	RasterSample rsa[RES_MAXX * RES_MAXY];
	
	void init(const Vector &tt, const Vector &ti, const Vector &tti, Uint32 *fb) // single-threaded
	{
		//
		if (CVars::v_ires > 1.0)
			CVars::v_ires = 1.0;

		this->tt = tt;
		this->ti = ti;
		this->tti = tti;
		this->fb = fb;
		xr = xsize_render(xres());
		yr = ysize_render(yres());
		memset(fb, 0, xr * yr * 4);
		memset(rsa, 0xee, xr * yr * sizeof(RasterSample));
		ires = (int) (1 / CVars::v_ires);
		W[0] = tt;
		W[1] = tt + ti * xr;
		W[2] = tt + tti * yr;
		bruteray = 0;
	}
	
	void stage1_interpolate(int x0, int y0, Uint32 c0, int x1, int y1, Uint32 c1, int index_y)
	{
		if (x0 == x1) {
			rsa[x1 + index_y * xr].set(y1, c1);
			return;
		}
		int inc = x1 > x0 ? +1 : -1;
		int r0, g0, b0, rd, gd, bd;
	
		b0 = c0 & 0xff;
		bd = (c1 & 0xff) - b0;
		c0 >>= 8;
		c1 >>= 8;
		g0 = c0 & 0xff;
		gd = (c1 & 0xff) - g0;
		c0 >>= 8;
		c1 >>= 8;
		r0 = c0 & 0xff;
		rd = (c1 & 0xff) - r0;
		int idx = index_y * xr;
		int ende = x1 + inc;
		int div = (int) (65536.0/(x1-x0));
		int yinc = (y1-y0)*inc*div;
		int rinc = rd * div;
		int ginc = gd * div;
		int binc = bd * div;
		int y = y0 << 16;
		int r = r0 << 16;
		int g = g0 << 16;
		int b = b0 << 16;
	
//	if (CVars::fast_interpol) {
		for (int i = x0; i != ende; i += inc, y += yinc, r += rinc, g += ginc, b += binc) {
			rsa[idx + i].set(y/65536, ((r/65536) << 16)|((g/65536) << 8)|(b/65536));
		}
//	} else {
	
//		for (int i = x0; i != ende; i += inc) {
//			y = y0 + (y1-y0) * (i-x0) / (x1-x0);
//			r = r0 + rd * (i-x0) / (x1-x0);
//			g = g0 + gd * (i-x0) / (x1-x0);
//			b = b0 + bd * (i-x0) / (x1-x0);
//			rsa[idx + i].set(y, (r << 16)|(g << 8)|b);
//		}
//	}
	}

	void stage2_interpolate(int x, RasterSample & prev, RasterSample & cur)
	{
		Uint32 *fbo = fb;
		fbo += x;
		if (cur.y == prev.y)
		{
			fbo[cur.y + xr] = cur.color;
			return;
		}
		int inc = cur.y > prev.y ? +1 : -1;
		int ende = cur.y + inc;
		Uint32 c0 = prev.color, c1 = cur.color;
		int r0, g0, b0, rd, gd, bd;
	
		b0 = c0 & 0xff;
		bd = (c1 & 0xff) - b0;
		c0 >>= 8;
		c1 >>= 8;
		g0 = c0 & 0xff;
		gd = (c1 & 0xff) - g0;
		c0 >>= 8;
		c1 >>= 8;
		r0 = c0 & 0xff;
		rd = (c1 & 0xff) - r0;

		int div = (int) (65536.0/(cur.y - prev.y));
		int ri = rd * div;
		int gi = gd * div;
		int bi = bd * div;
		int r = r0 << 16;
		int g = g0 << 16;
		int b = b0 << 16;
		for (int i = prev.y; i != ende; i += inc, r += ri, g += gi, b += bi) {
			fbo[i * xr] = ((r/65536) << 16)|((g/65536) << 8)|(b/65536);
		}
	}

	
	void subdiv(vox_t& vox, Vector corners[4], int size, int starty)
	{
		double depths[4], tot = 0;
		Vector res[4];
		bool good_corner = false, bad_corner = false;
		for (int i = 0; i < 4; i++) {
			tot += depths[i] = vox.hierarchy.ray_intersect(cur, corners[i], res[i]);
			if (depths[i] > 1e6)
				bad_corner = true;
			else
				good_corner = true;
		}
		if (!good_corner) return;
		tot *= 0.25;
		
		bool bad = false;
		for (int i = 0; i < 4; i++) {
			if (fabs(depths[i] - tot) > 10.0) {
				bad = true;
				break;
			}
		}
		
		if (!bad || size <= ires) {
			if (bad_corner) return; //FIXME!!
			int m = (int) (size * CVars::v_ires);
			for (int r = 0; r < m; r++) {
				int realy = (int) (starty + r / CVars::v_ires);
				Vector lhs = res[0] + (res[2] - res[0]) * ((double)r / m);
				Vector rhs = res[1] + (res[3] - res[1]) * ((double)r / m);
				rhs -= lhs;
				rhs.scale(1.0 / m);
				int lastx=0, lasty=0;
				int lastvalid=-2;
				Uint32 lastcolor=0;
				for (int c = 0; c <= m; c++, lhs += rhs) {
					float fx = lhs[0], fy = lhs[2];
					clip_coords(fx, fy, vox.size-2);
					if (CVars::bilinear) {
						lhs[1] = get_height(vox.heightmap, 
								vox.size,
								fx, fy, 0);
					} else {
						lhs[1] = vox.heightmap[(int)fx + (((int) fy)*vox.size)];
					}
					int xx, yy;Uint32 color=0xff0000;
					if (!project_point(&xx, &yy, lhs, cur, W, xr,yr)) continue;
					if (xx < 0 || xx >= xr-1 || yy < 0 || yy >= yr-1) continue;
					if (CVars::bilinear) {
						int o = (int) fx + (((int)fy)*vox.size);
						Uint32 *p = vox.texture;
						Uint32 x0y0 = p[o];
						Uint32 x1y0 = p[o + 1];
						Uint32 x0y1 = p[o + vox.size];
						Uint32 x1y1 = p[o + 1 + vox.size];
						color = bilinea_p5(x0y0, x1y0, x0y1, x1y1, 
								0xffff & (unsigned) (fx*65536.0f),
								0xffff & (unsigned) (fy*65536.0f));
					} else {
						color = vox.texture[(int)fx + (((int)fy)*vox.size)];
					}
					if (lastvalid == c-1) {
						stage1_interpolate(
								lastx, lasty, lastcolor,
						xx,    yy,    color,
						realy);
					}
					lastx = xx;
					lasty = yy;
					lastvalid = c;
					lastcolor = color;
				}
			}

		} else {
			Vector temp[5];
			temp[0] = (corners[0] + corners[1]) * 0.5;
			temp[1] = (corners[0] + corners[2]) * 0.5;
			temp[2] = (corners[1] + corners[2]) * 0.5;
			temp[3] = (corners[1] + corners[3]) * 0.5;
			temp[4] = (corners[2] + corners[3]) * 0.5;
			
			Vector xt[4][4] = {
				{ corners[0], temp[0], temp[1], temp[2] },
				{ temp[1], corners[1], temp[2], temp[3] },
				{ temp[1], temp[2], corners[2], temp[4] },
				{ temp[2], temp[3], temp[4], corners[3] },
			};
			for (int i = 0; i < 4; i++)
				subdiv(vox, xt[i], size/2, starty + (size/2) * (i > 1));
		}
		
	}

	void render(int thread_idx)
	{
	
		// FIXME for multithreading 
		if (thread_idx) return;
		
		int n = CVars::v_ores;
	//	int m = (int) (n * CVars::v_ires);
		for (int k = 0; k < NUM_VOXELS; k++) {
		
			/* Stage 1 : Data gathering */
			prof_enter(PROF_STAGE1);
			for (int j = 0; j < yr; j += n) {
				for (int i = 0; i < xr; i += n) {
					Vector cp = tt + ti * i + tti * j;
					Vector cs[4];
					for (int corner = 0; corner < 4; corner++) {
						Vector c = cp;
						if (corner & 1) c += ti * n;
						if (corner & 2) c += tti * n;
						cs[corner] = c;
					}
					subdiv(vox[k], cs, n, j);
				}
			}
			prof_leave(PROF_STAGE1);
		
			/* Stage 2 : Raster & fill */
			prof_enter(PROF_STAGE2);
			for (int i = 0; i < xr; i++) {
				int lj = -1;
				for (int j = 0; j < yr; j++) {
					RasterSample &rs = rsa[i + j * xr];
					if ((unsigned)rs.y != 0xeeeeeeee) {
						if (lj == -1) { lj = j; continue; }
						//
						stage2_interpolate(i, rsa[i + lj * xr], rs);
						//
						lj = j;
					}
				}
			}
			prof_leave(PROF_STAGE2);
		}
	}
	
	/*
	void render(int thread_idx)
	{
		int j;
		
		while ((j = bruteray++) < yr) {
			for (int k = 0; k < NUM_VOXELS; k++) {
				Vector v = tt + tti * j;
				Uint32 *fbo = &fb[j * xr];
				for (int i = 0; i < xr; i++, v += ti) {
					Vector res;
					double depth = vox[k].hierarchy.ray_intersect(cur, v, res);
					if (depth > 1e6) continue;
					float fx = res[0], fy = res[2];
					clip_coords(fx, fy, vox[k].size-2);
					if (CVars::bilinear) {
						int o = (int) fx + (((int)fy)*vox[k].size);
						Uint32 *p = vox[k].texture;
						Uint32 x0y0 = p[o];
						Uint32 x1y0 = p[o + 1];
						Uint32 x0y1 = p[o + vox[k].size];
						Uint32 x1y1 = p[o + 1 + vox[k].size];
						fbo[i] = bilinea_p5(x0y0, x1y0, x0y1, x1y1, 
								0xffff & (unsigned) (fx*65536.0f),
								0xffff & (unsigned) (fy*65536.0f));
					} else {
						fbo[i] = vox[k].texture[(int)fx + (((int)fy)*vox[k].size)];
					}
				}
			}
		}
	}
	*/

	
};

static AdaptiveVoxelRenderer adaptive_voxel_renderer;

void voxel_single_frame_do1(Uint32 *fb, int thread_index, Vector & tt, Vector & ti, Vector & tti)
{
	adaptive_voxel_renderer.render(thread_index);
}

Uint32 shootcolr;
Uint32 shootcolrs[4] = {0xff, 0xffff00, 0xff00, 0xffffff};

void castray(const Vector & p, Uint32 & color, float & depth, int k)
{
	Vector cross;
 	depth = vox[k].hierarchy.ray_intersect(cur, p, cross);
	if (depth < 10000.0) {
		if (cross[0] < 0.0 || cross[0] > vox[k].size-1 || cross[2] < 0.0 || cross[2] > vox[k].size) {
			color = 0;
			return;
		}
		int xx = (int) (cross[0]*65536.0);
		int yy = (int) (cross[2]*65536.0);
		int index = ((yy>>16)*vox[k].size + (xx>>16));
		if (shooting) {
			vox[k].texture[index] = shootcolr;
		}
		color = bilinea_p5(
				vox[k].texture[index],
				vox[k].texture[index+1],
				vox[k].texture[index+vox[k].size],
				vox[k].texture[index+vox[k].size + 1],
				xx & 0xffff, yy & 0xffff);
	} else {
		color = 0;
	}
}

static inline void cached_castray(const Vector & p, int x, int y, Uint32 & color, float & depth, int k)
{
	if (x >= vox_xr || y >= vox_yr) {
		castray(p, color, depth, k);
		return;
	}
	int index = y * vox_xr + x;
	if (IS_NEGATIVE(zbuffer[index])) {
		castray(p, color, depth, k);
		zbuffer[index] = depth;
		voxfb  [index] = color;
	} else {
		depth = zbuffer[index];
		color = voxfb  [index];
	}
}

static inline bool too_much_color_difference(Uint32 * colors)
{
	int decomposed[4][3] = {
		{ colors[0] & 0xff, (colors[0]>>8) & 0xff, (colors[0]>>16) & 0xff},
		{ colors[1] & 0xff, (colors[1]>>8) & 0xff, (colors[1]>>16) & 0xff},
		{ colors[2] & 0xff, (colors[2]>>8) & 0xff, (colors[2]>>16) & 0xff},
		{ colors[3] & 0xff, (colors[3]>>8) & 0xff, (colors[3]>>16) & 0xff}
	};
	for (int i = 0 ; i < 3 ; i++) {
		int avg = (2 + decomposed[0][i] + decomposed[1][i] + decomposed[2][i] + decomposed[3][i]) / 4;
		if (iabs(decomposed[0][i]-avg) > CVars::v_maxdiff) return true;
		if (iabs(decomposed[1][i]-avg) > CVars::v_maxdiff) return true;
		if (iabs(decomposed[2][i]-avg) > CVars::v_maxdiff) return true;
		if (iabs(decomposed[3][i]-avg) > CVars::v_maxdiff) return true;
	}
	return false;
}

void subdivide(Uint32 * blockPtr, const Vector & base, int x, int y, int size, int k)
{
	if (2 == size) {
		Uint32 cc; float dd;
		cached_castray(base             , x    , y    , cc, dd, k);
		cached_castray(base + gti       , x + 1, y    , cc, dd, k);
		cached_castray(base + gtti      , x    , y + 1, cc, dd, k);
		cached_castray(base + gti + gtti, x + 1, y + 1, cc, dd, k);
	} else {
		float  depth[4];
		Uint32 color[4];
		Vector x1y0, x0y1, x1y1;
		x1y0.macc(base, gti, size);
		x0y1.macc(base, gtti,size);
		x1y1.macc(x1y0, gtti,size);
		cached_castray(base, x       , y       , color[0], depth[0], k);
		cached_castray(x1y0, x + size, y       , color[1], depth[1], k);
		cached_castray(x0y1, x       , y + size, color[2], depth[2], k);
		cached_castray(x1y1, x + size, y + size, color[3], depth[3], k);

		float davg = 0.25 * (depth[0] + depth[1] + depth[2] + depth[3]);
		float noninfsum = 0.0, noninfdiv = 0.0;
		for (int i = 0; i < 4; i++)
			if (depth[i] < 10000) {
				noninfsum += depth[i];
				noninfdiv += 1.0;
			}
		if (noninfdiv==0.0) adapt = 9999;
			else adapt = noninfsum / noninfdiv / 10.0;
		// check if the difference is too large
		if (fabs(depth[0] - davg) > adapt || fabs(depth[1] - davg) > adapt ||
		    fabs(depth[2] - davg) > adapt || fabs(depth[3] - davg) > adapt ||
		    too_much_color_difference(color)) {
		    	int sz2 = size / 2;
			x1y0.macc(base, gti, sz2);
			x0y1.macc(base, gtti,sz2);
			x1y1.macc(x1y0, gtti,sz2);
			subdivide(blockPtr                     , base, x      , y      , sz2, k);
			subdivide(blockPtr + sz2               , x1y0, x + sz2, y      , sz2, k);
			subdivide(blockPtr       + sz2 * vox_xr, x0y1, x      , y + sz2, sz2, k);
			subdivide(blockPtr + sz2 + sz2 * vox_xr, x1y1, x + sz2, y + sz2, sz2, k);
			return;
		}

		if (davg < 10000) {
			if (CVars::bilinear) {
				int stepsrcp = (int) (65536.0f / size);

				int cx0y0[3], cx1y0[3], cx0y1[3], cx1y1[3];
				int li[3], ri[3];
				decompose(color[0], cx0y0);
				decompose(color[1], cx1y0);
				decompose(color[2], cx0y1);
				decompose(color[3], cx1y1);
				init_color_interpolation(cx0y1, cx0y0, li, stepsrcp);
				init_color_interpolation(cx1y1, cx1y0, ri, stepsrcp);

				for (int j = 0; j < size; j++) {
					int ct[3], cti[3];
					for (int i = 0; i < 3; i++) ct[i] = cx0y0[i];
					init_color_interpolation(cx1y0, cx0y0, cti, stepsrcp);
					for (int i = 0; i < size; i++) {
						blockPtr[j*vox_xr+i] =
								((ct[0]&0xff0000) >> 16) |
								((ct[1]&0xff0000) >> 8 ) |
								(ct[2]&0xff0000);
						step_interpolate(ct, cti);
					}
					step_interpolate(cx0y0, li);
					step_interpolate(cx1y0, ri);
				}
			} else {
			}
		}
	}
}

bool needs_bruteforce(Uint32 * blockPtr, const Vector & base, int x, int y, int size)
{
	Uint32 kolor = blockPtr[0] >> 24;
	bool needs = false, fullrange = false;
	for (int i = 1; i < 4; i++) {
		int offs = 0, xx = x, yy = y;
		if (i & 1) {
			xx += size;
			offs += size;
		}
		if (i & 2) {
			yy += size;
			offs += vox_xr * size;
		}
		if (xx >= vox_xr || y >= vox_yr) {
			needs = true; break;
		}
		if (kolor != (blockPtr[offs] >> 24)) {
			needs = true; 
			fullrange = true;
			break;
		}
	}
	if (needs) {
		int mink = kolor;
		int maxk = kolor; if (fullrange) { mink = 0; maxk = NUM_VOXELS-1; }
		/*
		for (int i = 0; i < size; i++) {
			if (y + i >= vox_yr) break;
			for (int j = 0; j < size; j++) {
				if (x + j >= vox_xr) break;
				blockPtr[i * vox_xr + j] = 0xff0000;
			}
		}
		//
		return true;
		*/

		for (int j = 0; j < size; j++) {
			Vector p = base + gtti * j;
			Uint32 *co = &blockPtr[j * vox_xr];
			if (y + j >= vox_yr) break;
			for (int i = 0; i < size; i++, p += gti, co++) {
				if (x + i >= vox_xr) break;
				if (!fullrange) {
					float dd;
					castray(p, *co, dd, mink);
				} else {
					Uint32 colors[2];
					float depths[2];
					for (int k = mink; k <= maxk; k++) {
						castray(p, colors[k], depths[k], k);
					}
					if (depths[0] < depths[1]) 
						*co = colors[0];
					else
						*co = colors[1];
				}
			}
		}
	}
	return needs;
}

static InterlockedInt task1, task2;

struct PixelInfo {
	float depth;
	Uint32 color;
	float u, v;
	int vox_num;
};

/*
void castray(const Vector & p, Uint32 & color, float & depth, int k)
{
	Vector cross;
 	depth = vox[k].hierarchy.ray_intersect(cur, p, cross);
	if (depth < 10000.0) {
		if (cross[0] < 0.0 || cross[0] > vox[k].size-1 || cross[2] < 0.0 || cross[2] > vox[k].size) {
			color = 0;
			return;
}
		int xx = (int) (cross[0]*65536.0);
		int yy = (int) (cross[2]*65536.0);
		int index = ((yy>>16)*vox[k].size + (xx>>16));
		if (shooting) {
			vox[k].texture[index] = shootcolr;
}
		color = bilinea_p5(
				vox[k].texture[index],
				vox[k].texture[index+1],
				vox[k].texture[index+vox[k].size],
				vox[k].texture[index+vox[k].size + 1],
				xx & 0xffff, yy & 0xffff);
} else {
		color = 0;
}
}
*/

void pixel_get_info(const Vector & t, PixelInfo &pinfo)
{
	Vector temp;
	if (pinfo.vox_num == -1) {
		Vector t1, t2;
		pinfo.depth = vox[0].hierarchy.ray_intersect(cur, t, t1);
		float td    = vox[1].hierarchy.ray_intersect(cur, t, t2);
		if (td < pinfo.depth) {
			pinfo.depth = td;
			temp = t2;
			pinfo.vox_num = 1;
		} else if (pinfo.depth < 1e6) {
			temp = t1;
			pinfo.vox_num = 0;
		}
	} else {
		pinfo.depth = vox[pinfo.vox_num].hierarchy.ray_intersect(cur, t, temp);
	}
	pinfo.color = 0;
	if (pinfo.depth < 1e6) {
		pinfo.u = temp[0];
		pinfo.v = temp[2];
		int n = vox[pinfo.vox_num].size;
		if (pinfo.u >= 0 && pinfo.u < n - 1 &&
		    pinfo.v >= 0 && pinfo.v < n - 1)
		{
			Uint32* base = &vox[pinfo.vox_num].texture[(((int)pinfo.v) * n) + ((int) pinfo.u)];
			int xx = (int) (65536.0 * (pinfo.u - floor(pinfo.u)));
			int yy = (int) (65536.0 * (pinfo.v - floor(pinfo.v)));
			pinfo.color = bilinea_p5(base[0], base[1], base[n], base[1+n], xx, yy);
		}
	}
}

bool needs_bruteforce(int x, int y, const Vector& base, PixelInfo *pinfo, int size)
{
	bool needs = false;
	int vox_num = -1;
	for (int i = 0; i < 4; i++) {
		if (pinfo[i].vox_num != -1) {
			if (vox_num == -1)
				vox_num = pinfo[i].vox_num;
			else
				if (vox_num != pinfo[i].vox_num) {
					needs = true;
					break;
				}
		}
	}
	if (needs) {
		for (int j = 0; j < size; j++) if (j + y < vox_yr) {
			Vector ybase = base + gtti * j;
			for (int i = 0; i < size; i++) if (i + x < vox_xr) {
				PixelInfo pi;
				pi.vox_num = -1;
				pixel_get_info(ybase + gti * i, pi);
				voxfb[(y+j) * vox_xr + x+i] = pi.color;
			}
		}
	}
	return needs;
}

void subdivide(int x, int y, const Vector & base, PixelInfo *pinfo, int size)
{
	if (size <= 2) {
		for (int j = 0; j < 2; j++) if (y + j < vox_yr) {
			for (int i = 0; i < 2; i++) if (x + i < vox_xr) {
				PixelInfo p;
				if (i == 0 && j == 0) {
					p = pinfo[0];
				} else {
					Vector t = base;
					if (i ) t += gti;
					if (j ) t += gtti;
					pixel_get_info(t, p);
				}
				voxfb[(y + j) * vox_xr + (x + i)] = p.color;
				//voxfb[(y + j) * vox_xr + (x + i)] = 0xff0000;
			}
		}
		return ;
	}
	int vox_num = -1;
	bool needs_subdivide = false;
	for (int i = 0; i < 4; i++) {
		if (pinfo[i].vox_num != -1) {
			vox_num = pinfo[i].vox_num;
		} else {
			needs_subdivide = true;
		}
	}
	if (vox_num == -1) {
		// just black square
		for (int j = 0; j < size; j++) if (y + j < vox_yr) {
			for (int i = 0; i < size; i++) if (x + i < vox_xr) {
				voxfb[(y + j) * vox_xr + (x + i)] = 0;
			}
		}
		return;
	}
	
	if (!needs_subdivide) {
		float adapt, davg = 0.0f;
		float noninfsum = 0.0f, noninfdiv = 0.0f;
		for (int i = 0; i < 4; i++) {
			davg += pinfo[i].depth;
			if (pinfo[i].depth < 10000) {
				noninfsum += pinfo[i].depth;
				noninfdiv += 1.0f;
			}
		}
		davg *= 0.25f;
		if (noninfdiv==0.0f) adapt = 9999;
			else adapt = noninfsum / noninfdiv / 10.0;
			
		if (fabs(pinfo[0].depth - davg) > adapt || 
		    fabs(pinfo[1].depth - davg) > adapt || 
		    fabs(pinfo[2].depth - davg) > adapt || 
		    fabs(pinfo[3].depth - davg) > adapt) {
			needs_subdivide = true;
		}
		
		/*
		float rcp_avg_depth = 4.0f / (pinfo[0].depth + pinfo[1].depth + pinfo[2].depth + pinfo[3].depth);
		for (int i = 0; i < 4; i++) {
			float t = pinfo[i].depth * rcp_avg_depth;
			if (t < 0.95f || t > 1.05f) {
				needs_subdivide = true;
				break;
			}
		}
		*/
		if (!needs_subdivide) {
		
			Uint32 colors[4];
			for (int i = 0; i < 4; i++) {
				colors[i] = pinfo[i].color;
			}
			needs_subdivide = too_much_color_difference(colors);
		}
	}
	
	if (needs_subdivide) {
		int sz2 = size / 2;
		PixelInfo new_pixels[5];
		Vector vecs[5];
		for (int i = 0; i < 5; i++) {
			new_pixels[i].vox_num = vox_num;
			vecs[i] = base;
			if (i % 2 == 0) vecs[i] += gti * sz2;
			if (i > 0) vecs[i] += gtti * sz2;
		}
		vecs[3] += gti * size;
		vecs[4] += gtti * sz2;
		for (int i = 0; i < 5; i++) {
			pixel_get_info(vecs[i], new_pixels[i]);
		}
		PixelInfo divs[4][4] = {
			{ pinfo[0], new_pixels[0], new_pixels[1], new_pixels[2] },
			{ new_pixels[0], pinfo[1], new_pixels[2], new_pixels[3] },
			{ new_pixels[1], new_pixels[2], pinfo[2], new_pixels[4] },
			{ new_pixels[2], new_pixels[3], new_pixels[4], pinfo[3] },
		};
		subdivide(x      , y      , base,    divs[0], sz2);
		subdivide(x + sz2, y      , vecs[0], divs[1], sz2);
		subdivide(x      , y + sz2, vecs[1], divs[2], sz2);
		subdivide(x + sz2, y + sz2, vecs[2], divs[3], sz2);
	} else {
		/*
		Uint32 color = 0xff;
		if (size == 8) color = 0xff00;
		if (size == 4) color = 0xffff00;
		for (int j = 0; j < size; j++) if (y + j < vox_yr) {
			for (int i = 0; i < size; i++) if (x + i < vox_xr) {
				voxfb[(j + y) * vox_xr + (i + x)] = color;
			}
		}
		*/
		if (CVars::bilinear) {
			float scaler = 1.0f / size;
			int n = vox[vox_num].size;
			vec2f lhs(pinfo[0].u, pinfo[0].v), lhi(pinfo[2].u, pinfo[2].v);
			vec2f rhs(pinfo[1].u, pinfo[1].v), rhi(pinfo[3].u, pinfo[3].v);
			lhi -= lhs;
			rhi -= rhs;
			lhi.scale(scaler);
			rhi.scale(scaler);
			for (int j = 0; j < size; j++, lhs += lhi, rhs += rhi) if (y + j < vox_yr) {
				vec2f v = lhs;
				vec2f vi = rhs - lhs; vi.scale(scaler);
				for (int i = 0; i < size; i++, v += vi) if ( x + i < vox_xr) {
					if (v[0] >= 0 && v[0] < n - 1 && v[1] >= 0 && v[1] < n - 1) {
						Uint32 *base = &vox[vox_num].texture[(((int)v[1]) * n) + ((int) v[0])];
						int xx = (int) (65536.0 * (v[0] - floor(v[0])));
						int yy = (int) (65536.0 * (v[1] - floor(v[1])));
						voxfb[(j + y) * vox_xr + i + x] =
							bilinea_p5(base[0], base[1], base[n], base[1+n], xx, yy);
					}
				}
			}
		} else {
			for (int j = 0; j < size; j++) if (y + j < vox_yr) {
				for (int i = 0; i < size; i++) if (x + i < vox_xr) {
					voxfb[(y + j) * vox_xr + (x + i)] = pinfo[0].color;
				}
			}
		}
	}
}

void voxel_single_frame_do2(Uint32 *fb, int thread_index, Vector & tt, Vector & ti, Vector & tti)
{
#ifdef __MINGW32__
#warning this function triggers an error, please fix it!
#else
	static PixelInfo precalc_pi[(RES_MAXX/8+1) * (RES_MAXY/8+1)];
	int xr = xsize_render(xres());
	int yr = ysize_render(yres());
	gti = ti;
	gtti= tti;

	vox_xr = xr; vox_yr = yr;
	voxfb = fb;

#ifdef BENCH
	/***/
	// if we're benchmarking - don't use multiple cpu
	if (thread_index) return;
	static bool didbenchmark = false;
	if (didbenchmark) return;
	didbenchmark = true;

	printf("Benchmarking raycasting speeds\n");
	printf("\tRandom texels: [");
	fflush(stdout);
	double cc = bTime();
	float depth;
	Uint32 tc;
	for (int i = 0; i < 1500000; i++) {
		double xx = rand() % 512000 / 1000.0;
		double zz = rand() % 512000 / 1000.0;
		Vector d(xx, 0, zz);
		castray(d, tc, depth, 0);
		if (i % (128*1024) == 0) {printf("#"); fflush(stdout);}
	}
	cc = bTime()-cc;
	printf("] \t%.0lf raycasts / sec\n", 1500000.0 / cc);
	printf("\tAdjascent texels: [");
	fflush(stdout);
	cc = bTime();
	for (int i = 0; i < 1500000; i++) {
		//double xx = rand() % 512000 / 1000.0;
		//double zz = rand() % 512000 / 1000.0;
		double xx = i % 512;
		double yy = i / 512 % 512;
		Vector d(xx, 0, yy);
		castray(d, tc, depth, 0);
		if (i % (128*1024) == 0) {printf("#"); fflush(stdout);}
	}
	cc = bTime()-cc;
	printf("] \t%.0lf raycasts / sec\n", 1500000.0 / cc);
	WantToQuit = true;
	return;
	/***/
#endif
	long long blockstart = 0;
	int blocksize = 8;

	Vector xStride = ti * blocksize, yStride = tti* blocksize;

	prof_enter(PROF_STAGE1);
	int n = (yr-1) / blocksize + 2;
	int m = (xr-1) / blocksize + 2;
	
	int j = 0;
	while ((j = task1++) < n) {
		Vector t = tt + yStride * j;
		for (int i = 0; i < m; i++, t += xStride) {
			PixelInfo &p = precalc_pi[i + j * m];
			p.vox_num = -1;
			pixel_get_info(t, p);
		}
	}
	

	precalculation.checkout();
	prof_leave(PROF_STAGE1);
	prof_enter(PROF_STAGE2);
	Vector walky(tt);
	int all_rays = 0;
	int next_id = task2++;
	for (int j = 0; j < n - 1; j++) {
		Vector t(walky);
		for (int i = 0; i < m - 1; i++, all_rays++) {
			if (all_rays == next_id) {
				if (CVars::v_showspeed)
					blockstart = prof_rdtsc();
				PixelInfo pinfo[4] = {
					precalc_pi[(j  ) * m + (i  )],
					precalc_pi[(j  ) * m + (i+1)],
					precalc_pi[(j+1) * m + (i  )],
					precalc_pi[(j+1) * m + (i+1)]
				};
				if (!needs_bruteforce(i*blocksize, j*blocksize, t, pinfo, blocksize)) {
					subdivide(i*blocksize, j*blocksize, t, pinfo, blocksize);
				}
				if (CVars::v_showspeed) {
					const int max_block_time = 180000;
					blockstart = prof_rdtsc() - blockstart;
					if (blockstart < 0) 
						blockstart = 0;
					if (blockstart > max_block_time) 
						blockstart = max_block_time;
					int t = (int) blockstart;
					t = (t * 767) / max_block_time;
					float opc; Uint32 pcolor;
					if (t < 256) {
						opc = t / 768.0f;
						pcolor = 0xff00;
					} else if (t < 512) {
						t -= 256;
						opc = 0.3333f;
						pcolor = 0xff00 + (t << 16);
					} else {
						t -= 512;
						opc = 0.3333f;
						pcolor = 0xffff00 - (t << 8);
					}
					for (int jj = 0; jj < blocksize; jj++) {
						for (int ii = 0; ii < blocksize; ii++) {
							fb[(j*blocksize+jj)*xr + i*blocksize + ii] = 
									blend(
										pcolor, 
										fb[(j*blocksize+jj)*xr + i*blocksize + ii], 
										opc
									);
						}
					}
				}

				next_id = task2++;
			}
			t += xStride;
		}
		walky += yStride;
	}
	prof_leave(PROF_STAGE2);
#endif // __MINGW32__
}

Uint32 voxel_raytrace(const Vector & cur, const Vector & v)
{
	int bk = -1;
	float mdist = 1e6;
	Vector bv;
	for (int k = 0; k < NUM_VOXELS; k++) {
		Vector vv;
		float t = vox[k].hierarchy.ray_intersect(cur, v, vv);
		if (t < mdist) {
			mdist = t;
			bk = k;
			bv = vv;
		}
	}
	if (-1 == bk) return 0;
	int xx = ((int) (bv[0]*65536.0));
	int yy = ((int) (bv[2]*65536.0));
	int index = ((yy>>16)*vox[bk].size + (xx>>16));
	return bilinea_p5(
			vox[bk].texture[index],
			vox[bk].texture[index+1],
			vox[bk].texture[index+vox[bk].size],
			vox[bk].texture[index+vox[bk].size+1],
			xx & 0xffff, yy & 0xffff);
}

void voxel_single_frame_do(Uint32 *fb, int, int, Vector & tt, Vector & ti, Vector & tti, int thread_index, InterlockedInt&)
{
	prof_enter(PROF_RENDER_VOXEL);
	task1 = 0;
	task2 = 0;
	voxel_light_recalc(thread_index);
	if (voxel_rendering_method) {
		voxel_single_frame_do2(fb, thread_index, tt, ti, tti);
	} else {
		voxel_single_frame_do1(fb, thread_index, tt, ti, tti);
	}
	prof_leave(PROF_RENDER_VOXEL);
	must_recalc_light = false;
}
void voxel_frame_init(const Vector& tt, const Vector &ti, const Vector &tti, Uint32* fb)
{
	// initialize barriers
	bar1.set_threads(cpu.count);
	bar.set_threads(cpu.count);
	precalculation.set_threads(cpu.count);
	lightOuter.set_threads(cpu.count);

	
	if (light_moving) {
		light.p[0] = (256 + 128 * sin(bTime()*0.1));
		light.p[2] = (256 + 128 * cos(bTime()*0.1));
		light.reposition();
		must_recalc_light = true;
	}
	if (shadow_casting_method > 2) {
		shadow_casting_method %= 2;
		must_recalc_light = true;
	}
	if (voxel_rendering_method == 0) {
		adaptive_voxel_renderer.init(tt, ti, tti, fb);
	}
}

void voxel_close(void)
{
	vx_free(NUM_VOXELS);
}

void voxel_shoot(void)
{
	shooting = true;
}




/**
 * @class Water
 */
  
class Water: public Object {
	float waterlevel;
	Vector *bounding_poly; int bpsize;
	double radius;
	bool inited;
	double thistime;
	double eta;
	int num_hits;
	struct WaterHit {
		double time;
		Vector pos;
	} hits[MAX_HITS];
public:
	Water() { inited = false ; eta = 0.75; bounding_poly = NULL; bpsize = 0; }
	~Water() { if (bounding_poly) delete [] bounding_poly; }
	double get_depth(const Vector &camera)
	{
		return 3000.0; // ensure it stays in the background
	}

	bool is_visible(const Vector & camera, Vector w[3])
	{
		thistime = bTime();
		if (bpsize <= 0) return false;
		
		for (int i = 0; i < bpsize; i++) {
			int ui, uj;
			if (project_point(&ui, &uj, bounding_poly[i], camera, w, xsize_render(xres()), ysize_render(yres())))
				return true;
		}
		return false;
	}

	int calculate_convex(Vector pt[], const Vector& camera)
	{
		for (int i = 0; i < bpsize; i++)
			pt[i] = bounding_poly[i];
		return bpsize;
	}

	void map2screen(
			Uint32 *framebuffer,
	int color,
	int sides,
	Vector pt[],
	const Vector& camera,
	Vector w[3],
	int & min_y,
	int & max_y) {
		int L[RES_MAXY], R[RES_MAXY];
		int ys, ye, bs, be;
		min_y = ys = accumulate(pt, sides, camera, w, fun_less, 999666111, bs);
		max_y = ye = accumulate(pt, sides, camera, w, fun_more,-999666111, be);
		int size = (be + sides - bs) % sides;
		project_hull_part(L, pt, bs, +1, size, sides, color, camera, w, fun_min);
		project_hull_part(R, pt, bs, -1, sides - size, sides, color, camera, w, fun_max);
		map_hull(framebuffer, L, R, ys, ye, color);
	}

	bool fastintersect(const Vector& ray, const Vector& camera, const double& rls, void *IntersectContext) const 
	{
		Vector t = camera + ray, u;
		double r;
		if ((r=vox[0].hierarchy.ray_intersect(camera, t, u)) > 1e6) return false;
		if (u[1] > waterlevel) return false;
		if (vox[1].hierarchy.ray_intersect(camera, t, u) < 1e6) return false;
		
		*(double*)IntersectContext = r;
		return true;
	}

	bool intersect(const Vector& ray, const Vector &camera,	void *IntersectContext)
	{
		Vector t = camera + ray, u;
		double r;
		if ((r=vox[0].hierarchy.ray_intersect(camera, t, u)) > 1e6) return false;
		if (u[1] > waterlevel) return false;
		if (vox[1].hierarchy.ray_intersect(camera, t, u) < 1e6) return false;
		
		*(double*)IntersectContext = r;
		return true;
	}

	double intersection_dist(void *IntersectContext) const
	{
		return *(double*)IntersectContext;
	}
	
	Vector surface(Vector pos)
	{
		Vector p(pos);
		p[0] += 2.0 * thistime;
		p[2] += 2.6 * thistime;
		Vector waves(0.0035 * sin(p[0]*0.8) + 0.0045 * cos(p[0]*0.72), 
			      0.99,
			      0.005 * sin(p[2]*0.50) + 0.0025 * cos(p[2] * 1.03));
		
		Vector circles;
		circles.zero();
		for (int i = 0; i < num_hits; i++) {
			double dist = pos.distto(hits[i].pos);
			double td = (thistime - hits[i].time)*25;
			if (dist < td) {
				double x = td - dist;
				double y = sin(x*0.5) / (3 + 4*x);
				Vector q = pos - hits[i].pos;
				q.norm();
				circles += q * y;
			}
		}
		
		return waves + circles;
	}

	Uint32 shade(
			Vector& ray,
	const Vector& camera,
	const Vector& light,
	double rlsrcp,
	float &opacity,
	void *IntersectContext,
	int iteration,
	FilteringInfo& finfo
			    )
	{
		prof_enter(PROF_SOLVE3D);
		opacity = 1.0f;
		double h = (waterlevel - camera[1]) / ray[1];
		Vector I;
		I.macc(camera, ray, h);
		Vector r2 = ray;
		
		r2.norm();
		
		Vector n = surface(I);
		
		double cosI = n * r2;
		//result = fresnel * upwards + (1 - fresnel) * downwards
		double fresnel = 0.9 + 0.8 * cosI; 
		
		double k = 1.0 - eta * eta * (1.0 - cosI * cosI);
		Uint32 downwards_color = 0, upwards_color = 0;
		prof_leave(PROF_SOLVE3D);
		prof_enter(PROF_RAYTRACE);
		if (k >= 0) {
			Vector down = r2 * eta - n * (eta * cosI + sqrt(k));
			Vector res;
			double temp = vox[0].hierarchy.ray_intersect(I, I + down, res);
			if (temp < 1e6) {
				downwards_color = vox[0].texture[(((int) res[2]) * vox[0].size) + (int) res[0]];
			}
		}
		Vector up = r2 - n * ((n * r2) *2);
		float mind = 1e6;
		for (int w = 0; w < 2; w++) {
			Vector res;
			float temp = vox[w].hierarchy.ray_intersect(I, I + up, res);
			if (temp < mind) {
				mind = temp;
				upwards_color = vox[w].texture[(((int) res[2]) * vox[0].size) + (int) res[0]];
			}
		}
		prof_leave(PROF_RAYTRACE);
		
		return blend(upwards_color, downwards_color, fresnel);
		
	}
	int get_best_miplevel(double x0, double z0, FilteringInfo & fi)
	{
		return TEX_S;
	}

	OBTYPE get_type(bool generic = true) const
	{
		return OB_WATER;
	}

	
	void init(int _waterlevel)
	{
		if (inited) return;
		inited = true;
		waterlevel = _waterlevel * vxInput[0].scale;
		int n = vox[0].size;
		
		const int probe_step = 2;
		
		int dirs[8][2] = { 
			{ -1, -1 }, { -1, 0 }, { -1, 1 }, { 0, 1 },
			{  1,  1 }, {  1, 0 }, {  1, -1}, { 0, -1}
		};
		Array<vec2f> arr;
		
		for (int i = 0; i < n / probe_step; i++) {
			for (int j = 0; j < n / probe_step; j++) {
				if (vox[0].heightmap[(i*n+j)*probe_step] > waterlevel) continue;
				bool works = false;
				for (int q = 0; q < 8; q++) {
					int ii = i + dirs[q][0];
					int jj = j + dirs[q][1];
					if (ii < 0 || ii >= n / probe_step || jj < 0 || jj >= n / probe_step) continue;
					if (vox[0].heightmap[(ii*n+jj)*probe_step] <= waterlevel) {
						works = true;
						break;
					}
				}
				if (works) {
					arr += vec2f((float)i * probe_step, (float)j * probe_step);
				}
			}
		}
		Array<vec2f> result = convex_hull(arr);
		bpsize = result.size();
		bounding_poly = new Vector[bpsize];
		for (int i = 0; i < bpsize; i++) {
			bounding_poly[bpsize-1-i] = Vector(result[i][1], waterlevel, result[i][0]);
		}
	}
	
	double get_level() const
	{
		return waterlevel;
	}
	
	void remove_old_hits(double time)
	{
		int j = 0;
		while (j < num_hits && time - hits[j].time > 10) j++;
		if (!j) return;
		int i = 0;
		while (j < num_hits) {
			hits[i] = hits[j];
			i++,j++;
		}
		num_hits = i;
	}
	
	void add_hit(double time, Vector pos)
	{
		if (num_hits > MAX_HITS) return;
		pos[1] = waterlevel;
		hits[num_hits].time = time;
		hits[num_hits].pos = pos;
		++num_hits;
	}
};

class WaterDrops : public PhysicsHook {
	double lasttime, prevt;
	Water *water;
	Vector drop_position;
public:
	void preprocess(double time)
	{
	}
	void postprocess(double time)
	{
		water->remove_old_hits(time);
		if (spherecount) {
			Sphere &s = sp[0];
			if (s.pos[1] < water->get_level()) {
				spherecount = 0;
				double cury = s.pos[1];
				double prevy = s.pos[1] - s.mov[1] * (time-prevt);
				if (prevy < water->get_level())
					prevy = water->get_level();
				double hittime = prevt + (time-prevt)*(prevy-water->get_level())/(prevy-cury);
				
				water->add_hit(hittime, s.pos);
			}
		}
		prevt = time;
		if (lasttime == -1) {
			lasttime = time-5.0;
			return;
		}
		
		if (time - lasttime > 7.0) {
			Sphere &s = sp[spherecount];
			s.pos = drop_position;
			s.mov.zero();
			s.d = 2.5;
			s.mass = 1;
			s.time = time;
			s.refl = 0.25;
			s.opacity = 1;
			s.r = s.g = s.b = 255;
			s.flags = RAYTRACED | RECURSIVE | SEETHROUGH | GRAVITY | AIR | NOFLOORBOUNCE;
			++spherecount;
			lasttime = time;
		}
	}
	
	void init(Water *w)
	{
		lasttime = -1;
		water = w;
		float minv = 1e10;
		for (int j = 128; j < 384; j++) {
			for (int i = 128; i < 384; i++) {
				Vector t(i+0.5, vox[1].heightmap[j*vox[1].size+i], j+0.5);
				if (t[1] < minv) {
					minv = t[1];
					drop_position = t;
				}
			}
		}
	}
};

static Water water;
static WaterDrops water_drops;

Object* voxel_water_object(void)
{
	return &water;
}

void voxel_water_init(void)
{
	static bool inited = false;
	if (inited) return;
	inited = true;
	water.init(76);
	water_drops.init(&water);
	physics_add_hook(&water_drops);
}

double voxel_get_waterlevel(void)
{
	return water.get_level();
}
