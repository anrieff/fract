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
#include "vectormath.h"
#include "voxel.h"
#include "light.h"

//#define BENCH

// we need those neat class...
#include "hierarchy.h"

/** !!!!!!!------------ Constants -------------- -!!!!!!!!!!!!!!!!!!!!!!!**/
info_t vxInput[NUM_VOXELS] = {
	//{ "data/heightmap_f.fda", "data/aardfdry256.bmp", 0, 0.5, 1},
	//{ "data/heightmap_c.fda", "data/aardfdry256.bmp", 200, 0.5, 0}
	{ "data/heightmap_f.fda", "data/aardfdry512.bmp", /*-200*/0, 0.55/*2.5*/, 1},
	//{ "data/heightmap_c.fda", "data/aardfdry256.bmp", 200, 0.5, 0}
};

/** --------=========== Externals ============== ------------------------**/

extern Vector cur;
extern double fov;
extern int vframe;
extern bool WantToQuit;

#ifdef ACTUALLYDISPLAY
extern SDL_Surface *screen;
#endif

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
int voxel_rendering_method = 0, shadow_casting_method = 0;
bool light_moving = true;
bool must_recalc_light;
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
		///HACK:
		//memset(vox[k].heightmap, 0, b.get_size()*sizeof(float));
		double med_val = 0.0;
		for (i=0;i<a.get_size();i++) {
			med_val += (vox[k].heightmap[i] = vox[k].yoffs + vox[k].heightmap[i]*vxInput[k].scale);
		}
		med_val /= (vox[k].size * vox[k].size);
		vox[k].med_val = med_val;
		vox[k].hierarchy.build_hierarchy(vox[k].size, vox[k].heightmap, vox[k].floor);
#ifdef ACTUALLYDISPLAY
		intro_progress(screen, prog_vox_init_base + prog_vox_init*(k+1.0)/NUM_VOXELS);
#endif
	}
	voxel_precalc();
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
#ifdef ACTUALLYDISPLAY
			if ((y&63)==0) {
				intro_progress(screen, prog_normal_precalc_base + prog_normal_precalc*(y+k*vox[k].size)
					/(NUM_VOXELS*vox[k].size));
			}
#endif
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
			/*if (show)
				intro_progress(screen, prog_vox_precalc_base + prog_vox_precalc*(k*4+sd / 4.0*NUM_VOXELS));*/
		}
	}
}

#define uint unsigned int
#define fastrsqrt FastReciprocalSquareRoot
static inline float fast_reciprocal_square_root(float val)
{
    const float magicValue = 1597358720.0f;
    float tmp = (float) *((uint*)&val);
    tmp = (tmp * -0.5) + magicValue;
    uint tmp2 = (uint) tmp;
    return *(float*)&tmp2;
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

		int index = 0;
		/*for (int z = 0; z < size; z++)
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
	int r, g, b, p=0;
	// FIXME (for multithreading):
	if (thread_idx) return;
	int smsize = 1 << HEIGHTFIELD_RAYTRACE_MIPLEVEL;
	for (int k = 0; k < NUM_VOXELS; k++) {
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

struct RasterSample {
	int y;
	Uint32 color;
	void set(int _y, Uint32 _color) { y = _y; color = _color; }
};

RasterSample rsa[RES_MAXX * RES_MAXY];

static void stage1_interpolate(int x0, int y0, Uint32 c0, int x1, int y1, Uint32 c1, int xr, int index_y)
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

static void stage2_interpolate(Uint32 *fb, int xr, int x, RasterSample & prev, RasterSample & cur)
{
	fb += x;
	if (cur.y == prev.y)
	{
		fb[cur.y + xr] = cur.color;
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
		fb[i * xr] = ((r/65536) << 16)|((g/65536) << 8)|(b/65536);
	}
}

void voxel_single_frame_do1(Uint32 *fb, int thread_index, Vector & tt, Vector & ti, Vector & tti)
{
	prof_enter(PROF_BUFFER_CLEAR);
	if (CVars::v_ires > 1.0)
		CVars::v_ires = 1.0;
	// FIXME for multithreading 
	if (thread_index) return;
	int xr = xsize_render(xres());
	int yr = ysize_render(yres());
	memset(fb, 0, xr * yr * 4);
	memset(rsa, 0xee, xr * yr * sizeof(RasterSample));
	prof_leave(PROF_BUFFER_CLEAR);
	
	
	Vector W[3];
	W[0] = tt;
	W[1] = tt + ti * xr;
	W[2] = tt + tti * yr;
	
	int n = CVars::v_ores;
	int m = (int) (n * CVars::v_ires);
	for (int k = 0; k < NUM_VOXELS; k++) {
		
		/* Stage 1 : Data gathering */
		prof_enter(PROF_STAGE1);
		for (int j = 0; j < yr; j += n) {
			for (int i = 0; i < xr; i += n) {
				Vector cp = tt + ti * i + tti * j;
				Vector res[4];
				bool good = true;
				for (int corner = 0; corner < 4; corner++) {
					Vector c = cp;
					if (corner & 1) c += ti * n;
					if (corner & 2) c += tti * n;
					if (vox[k].hierarchy.ray_intersect(cur, c, res[corner]) > 1e6) {
						good = false;
						break;
					}
				}
				if (!good) continue;
				for (int r = 0; r < m; r++) {
					int realy = (int) (j + r / CVars::v_ires);
					Vector lhs = res[0] + (res[2] - res[0]) * ((double)r / m);
					Vector rhs = res[1] + (res[3] - res[1]) * ((double)r / m);
					rhs -= lhs;
					rhs.scale(1.0 / m);
					int lastx=0, lasty=0;
					int lastvalid=-2;
					Uint32 lastcolor=0;
					for (int c = 0; c <= m; c++, lhs += rhs) {
						float fx = lhs[0], fy = lhs[2];
						clip_coords(fx, fy, vox[k].size-2);
						if (CVars::bilinear) {
							lhs[1] = get_height(vox[k].heightmap, 
									vox[k].size,
									fx, fy, 0);
						} else {
							lhs[1] = vox[k].heightmap[(int)fx + (((int) fy)*vox[k].size)];
						}
						int xx, yy;Uint32 color=0xff0000;
						if (!project_point(&xx, &yy, lhs, cur, W, xr,yr)) continue;
						if (xx < 0 || xx >= xr-1 || yy < 0 || yy >= yr-1) continue;
						if (CVars::bilinear) {
							int o = (int) fx + (((int)fy)*vox[k].size);
							Uint32 *p = vox[k].texture;
							Uint32 x0y0 = p[o];
							Uint32 x1y0 = p[o + 1];
							Uint32 x0y1 = p[o + vox[k].size];
							Uint32 x1y1 = p[o + 1 + vox[k].size];
							color = bilinea_p5(x0y0, x1y0, x0y1, x1y1, 
									0xffff & (unsigned) (fx*65536.0f),
									0xffff & (unsigned) (fy*65536.0f));
						} else {
							color = vox[k].texture[(int)fx + (((int)fy)*vox[k].size)];
						}
						if (lastvalid == c-1) {
							stage1_interpolate(
									lastx, lasty, lastcolor,
									xx,    yy,    color,
									xr, realy);
						}
						lastx = xx;
						lasty = yy;
						lastvalid = c;
						lastcolor = color;
					}
				}
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
					stage2_interpolate(fb, xr, i, rsa[i + lj * xr], rs);
					//
					lj = j;
				}
			}
		}
		prof_leave(PROF_STAGE2);
	}
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
	color = (depth < 10000) ? vox[k].texture[((int) cross[2] * vox[k].size + (int) cross[0])%(sqr(vox[k].size))] : 0;
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
		if (iabs(decomposed[0][i]-avg) > MAX_COLOR_DIFF) return true;
		if (iabs(decomposed[1][i]-avg) > MAX_COLOR_DIFF) return true;
		if (iabs(decomposed[2][i]-avg) > MAX_COLOR_DIFF) return true;
		if (iabs(decomposed[3][i]-avg) > MAX_COLOR_DIFF) return true;
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
				for (int j = 0; j < size; j++)
					for (int i = 0; i < size; i++)
						blockPtr[j*vox_xr+i] = color[0];
			}
		}
	}
}

void voxel_single_frame_do2(Uint32 *fb, int thread_index, Vector & tt, Vector & ti, Vector & tti)
{
#ifdef __MINGW32__
#warning this function triggers an error, please fix it!
#else
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
	//memset(fb, 0, xr*yr*4);
	//for (int i = 0; i < xr*yr; i++)
	//	zbuffer[i] = -1.0f;
	multithreaded_memset(fb, 0, xr*yr, thread_index, cpu.count);
	float minus_one = -1.0f;
	unsigned pattern = *(unsigned *) &minus_one;
	multithreaded_memset(zbuffer, pattern, xr*yr, thread_index, cpu.count);
	Vector xStride = ti * 8.0f,
	       yStride = tti* 8.0f;
	Vector walky(tt);
	bar.checkout();
	// precalculate the big grid
	for (int k = 0; k < NUM_VOXELS; k++) {
		int all_rays = 0;
		for (int j = 0; j < yr; j+=8) {
			Vector t(walky);
			for (int i = 0; i < xr; i+=8, all_rays++) {
				if (all_rays % cpu.count == thread_index) {
					shootcolr = shootcolrs[(i/8)%4];
					castray(t, fb[j*xr+i], zbuffer[j*xr+i], k);
				}
				t += xStride;
			}
			walky += yStride;
		}
		precalculation.checkout();
		all_rays = 0;
		shooting = false;
		walky = tt;
		// do the raytracing
		for (int j = 0; j < yr; j+=8) {
			Vector t(walky);
			for (int i = 0; i < xr; i+=8, all_rays++) {
				if (all_rays % cpu.count == thread_index) 
					subdivide(fb + j*xr + i, t, i, j, 8, k);
				t += xStride;
			}
			walky += yStride;
		}
	}
#endif // __MINGW32__
}

Uint32 voxel_raytrace(const Vector & cur, const Vector & v)
{
	Vector cross;
	for (int k = 0; k < NUM_VOXELS; k++) {
		double depth = vox[k].hierarchy.ray_intersect(cur, v, cross);
		int mipsize = vox[k].size >> HEIGHTFIELD_RAYTRACE_MIPLEVEL;
		if (depth < 10000.0 && cross[0] >= 0.0 && cross[0] < vox[k].size - (1<<HEIGHTFIELD_RAYTRACE_MIPLEVEL) &&
				       cross[2] >= 0.0 && cross[2] < vox[k].size - (1<<HEIGHTFIELD_RAYTRACE_MIPLEVEL)  ) {


			int xx = ((int) (cross[0]*65536.0)) >> HEIGHTFIELD_RAYTRACE_MIPLEVEL;
			int yy = ((int) (cross[2]*65536.0)) >> HEIGHTFIELD_RAYTRACE_MIPLEVEL;
			int index = ((yy>>16)*mipsize + (xx>>16));
			return bilinea_p5(
				vox[k].raytrace_mip[index],
				vox[k].raytrace_mip[index+1],
				vox[k].raytrace_mip[index+mipsize],
				vox[k].raytrace_mip[index+mipsize + 1],
				xx & 0xffff, yy & 0xffff);

		}
	}
	return 0;
}

void voxel_single_frame_do(Uint32 *fb, int, int, Vector & tt, Vector & ti, Vector & tti, int thread_index, InterlockedInt&)
{
	prof_enter(PROF_RENDER_VOXEL);
	voxel_light_recalc(thread_index);
	if (voxel_rendering_method) {
		voxel_single_frame_do2(fb, thread_index, tt, ti, tti);
	} else {
		voxel_single_frame_do1(fb, thread_index, tt, ti, tti);
	}
	prof_leave(PROF_RENDER_VOXEL);
}
void voxel_frame_init(void)
{
	must_recalc_light = false;

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
}

void voxel_close(void)
{
	vx_free(NUM_VOXELS);
}

void voxel_shoot(void)
{
	shooting = true;
}
