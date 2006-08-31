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

//#define BENCH

// we need those neat class...
#include "hierarchy.h"

/** !!!!!!!------------ Constants -------------- -!!!!!!!!!!!!!!!!!!!!!!!**/
info_t vxInput[NUM_VOXELS] = {
	//{ "data/heightmap_f.fda", "data/aardfdry256.bmp", 0, 0.5, 1},
	//{ "data/heightmap_c.fda", "data/aardfdry256.bmp", 200, 0.5, 0}
	{ "data/heightmap_f.fda", "data/green.bmp", /*-200*/0, 0.55/*2.5*/, 1},
	//{ "data/heightmap_c.fda", "data/aardfdry256.bmp", 200, 0.5, 0}
};

/** --------=========== Externals ============== ------------------------**/

extern Vector cur;
extern double alpha, beta;
extern double fov;
extern int bilfilter;
extern int vframe, lx, ly, lz;
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
Vector gti, gtti, light;
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
				float x = lx, y = lz;
				float dst = sqrt(sqr(ex-lx) + sqr(ey-lz));
				float rcp_dst = 1.0f / dst;
				float xi= (ex-lx) * rcp_dst;
				float yi= (ey-lz) * rcp_dst;
				float min_cotg = 1e19;
				int idist = 1;
				for (int tt = (int) dst; tt > 0; --tt, idist += 10, x += xi, y += yi) {
					clip_coords(x, y, size-1);
					int index = (((int) y) * size) + (int) x;
					float vDist = ly - get_height(vox[k].heightmap, size, x, y, FLOOR_LOW);
					float new_cotg = sMul*vDist * prec_rcp_x[idist];
					if (new_cotg < min_cotg) {
						min_cotg = new_cotg;
						//Vector ray(lx - x, ly-height, lz-y);
						float *vec = &vox[k].normal_map[index*__4];
						float vX = lx - x, vZ = lz - y;
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
	Vector l(lx, ly, lz);
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
			float vX = lx - x, vZ = lz - z;
			float vDist = ly - v->heightmap[index];
			float ll = sqrt(vX*vX + vZ*vZ + vDist*vDist);
			float mul = (vX*vec[0]+ vDist*vec[1] + vZ * vec[2]) * 5.0 * prec_rsqrt_x[(int)(ll*RESOLUTION)];
			tex_paint_scale(v, index, x, z, iambient + dbl2int16(fabs(mul)));
		}
		index++, x++;
		if (state[1]==0x01) {
			tex_paint_scale(v, index, x, z, iambient);
		} else {
			float *vec = &(v->normal_map[index*__4]);
			float vX = lx - x, vZ = lz - z;
			float vDist = ly - v->heightmap[index];
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
			float vX = lx - x, vZ = lz - z;
			float vDist = ly - v->heightmap[index];
			float ll = sqrt(vX*vX + vZ*vZ + vDist*vDist);
			float mul = (vX*vec[0]+ vDist*vec[1] + vZ * vec[2]) * 5.0 * prec_rsqrt_x[(int)(ll*RESOLUTION)];
			tex_paint_scale(v, index, x, z, iambient + dbl2int16(fabs(mul)));
		}
		index++, x++;
		if (state[3]==0x01) {
			tex_paint_scale(v, index, x, z, iambient);
		} else {
			float *vec = &(v->normal_map[index*__4]);
			float vX = lx - x, vZ = lz - z;
			float vDist = ly - v->heightmap[index];
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
					float vX = lx - x, vZ = lz - z;
					float vDist = ly - v->heightmap[index];
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
	light = Vector(lx, ly, lz);
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
	calc_grid_basics(cur, alpha, beta, w);
	int x, y;
	Vector light(lx, ly, lz);
	project_point(&x, &y, light, cur, w, xr, yr);
	if (x >= 0 && x < xr && y >= 0 && y < yr)
		fb[xr * y + x ] = 0xffffff;
}

void voxel_single_frame_do1(Uint32 *fb, int thread_index, Vector & tt, Vector & ti, Vector & tti)
{
	double fx, fz, fxi, fzi, dCX;
	float rcpwf[MAX_VOXEL_DIST];
	int x, z, xi, zi, index, index2;
	Uint32 lastcol;

	int k, i, wf, j, xr, yr, sz, lasty, newy, xr2, yr2, txadd, texandmask, tempx;
	int yrhl, newymp, opty;
	int sizemask, sizeshift;
	int render_dist;

	Uint32 *tex;
	Uint32 *c_column;
	float  *h;
	Uint32 color;


	xr = xsize_render(xres());
	yr = ysize_render(yres());
	xr2 = xr/2;
	yr2 = yr/2;

	yrhl = xr*yr;


	dCX = 1.0/tan(fov*0.75*FOURTY_FIVE_DEGREES);
	double tgB = tan(beta);
	float sinb = sin(-beta), cosb = cos(-beta);
	double Thing = sqrt(1+ sqr(tgB));
	double rcpThing = 1.0 / Thing;


	prof_enter(PROF_BUFFER_CLEAR);
	if (1 == cpu.count) {
		memset(fb, 0, xr*yr*4);
	} else {
		// crap; we need to zero the memory in parallel; COLUMNWISE!
		for (i = thread_index; i < xr; i+=cpu.count)
			for (j=0;j<yr;j++)
				fb[j*xr+i] = 0;
	}
	prof_leave(PROF_BUFFER_CLEAR);

	// do the actual work
	for (k=0;k<NUM_VOXELS;k++) {
		bool isfloor = vox[k].floor;
		int swOne = isfloor?-1:1;
		//render_dist = render_dist_min + (int) ((render_dist_max-render_dist_min)*component/(daing-daFloor));
		render_dist = 730;
		for (i=1;i<=render_dist;i++) rcpwf[i] = 1.0f / i;
		tex = vox[k].texture; h = vox[k].heightmap;
		sz = vox[k].size;
		sizemask = sz-1;
		sizeshift = power_of_2(sz);
		texandmask = sz*sz-1;
		fx = cur[0] + sin(alpha - fov * FOURTY_FIVE_DEGREES)*render_dist;
		fz = cur[2] + cos(alpha - fov * FOURTY_FIVE_DEGREES)*render_dist;
		fxi = ((cur[0] + sin(alpha + fov * FOURTY_FIVE_DEGREES)*render_dist) - fx) / xr;
		fzi = ((cur[2] + cos(alpha + fov * FOURTY_FIVE_DEGREES)*render_dist) - fz) / xr;
		fx += thread_index * fxi;
		fz += thread_index * fzi;

		for (i=thread_index;i<xr;i+=cpu.count) {
			c_column = fb + i;
			x = dbl2int16(cur[0]); z = dbl2int16(cur[2]);
			xi = dbl2int16((fx - cur[0]) / render_dist);
			zi = dbl2int16((fz - cur[2]) / render_dist);

			//wf = render_dist;
			opty = isfloor ? yr : 0;
			x+=xi;z+=zi;
			lastcol = 0xbabababa;
			for (lasty=999666111, wf=1; wf < render_dist; wf++, lasty = opty, x+=xi, z+=zi) {
				//if (((x>>16)&~sizemask)||((z>>16)&~sizemask)) continue;
				if (((unsigned)x>>16) > (unsigned) sz - 2 || ((unsigned)z>>16) > (unsigned) sz - 2) continue;
				if (!bilfilter) {
					prof_enter(PROF_ADDRESS_GENERATE);
					index = ((((unsigned)z>>16)&sizemask)<<sizeshift) + (((unsigned)x>>16)&sizemask);

					double CFp = wf*Thing;
					double FpFpp = wf*tgB;
					double FFpp = h[index] - cur.v[1];

					double FQMUL = (double) (( (int) (FpFpp*FFpp>0&&fabs(FFpp)>fabs(FpFpp)))*2-1);
					double FpF;
					if (FpFpp*FFpp>0.0f)
						FpF = fabs(fabs(FpFpp)-fabs(FFpp));
						else
						FpF = fabs(FpFpp)+fabs(FFpp);

					double FQ = fabs(FpF)*rcpThing;
					double FpQ= fabs(FpFpp*FpF*rcpwf[wf]*rcpThing);
					double CQ = fabs(CFp)+FQMUL*fabs(FpQ);
					double XF3p=fabs(FQ*dCX/CQ)*((double)(((int)(FFpp<FpFpp))*2-1));

					newy   = yr2 + (int)(yr2*XF3p);
					prof_leave(PROF_ADDRESS_GENERATE);
					if ((isfloor && newy >= opty)||((!isfloor) && newy <= opty)) continue;
					prof_enter(PROF_INTERPOL_INIT);
					color = tex[index];
					if (isfloor) { // newy must be no less than 1
						newy = newy>=1 ? newy : 1;
					} else { //newy must be no more than yr-2
						newy = newy<= yr - 2 ? newy : yr-2;
					}
					newymp = newy * xr + swOne*xr;
					opty += swOne;
					prof_leave(PROF_INTERPOL_INIT);
					prof_enter(PROF_INTERPOLATE);
					if (lasty!=999666111) {
						j = opty * xr;
						if (isfloor) {
							do {
								c_column[j] = color;
								j -= xr;
							} while (j>newymp);
						} else {
							do {
								c_column[j] = color;
								j += xr;
							} while (j<newymp);
						}
					}
					opty = newy;
					prof_leave(PROF_INTERPOLATE);

				} else {
					prof_enter(PROF_ADDRESS_GENERATE);
					tempx = (((unsigned) x >> 16)&sizemask);
					txadd = ((tempx+1)&sizemask) - tempx;
					index = ((((unsigned)z>>16)&sizemask)<<sizeshift) + tempx;
					index2 = (index + sz) & texandmask;
					double FFpp = filter4(x&0xffff, z&0xffff,
						h[ index		],
						h[ index + txadd	],
						h[ index2		],
						h[ index2 + txadd	]) - cur.v[1];
					double CFp = wf*Thing;
					double FpFpp = wf*tgB;

					double FQMUL = (double) (( (int) (FpFpp*FFpp>0&&fabs(FFpp)>fabs(FpFpp)))*2-1);
					double FpF;
					if (FpFpp*FFpp>0.0f)
						FpF = fabs(fabs(FpFpp)-fabs(FFpp));
						else
						FpF = fabs(FpFpp)+fabs(FFpp);

					double FQ = fabs(FpF)*rcpThing;
					double FpQ= fabs(FpFpp*FpF*rcpwf[wf]*rcpThing);
					double CQ = fabs(CFp)+FQMUL*fabs(FpQ);
					double XF3p=fabs(FQ*dCX/CQ)*((double)(((int)(FFpp<FpFpp))*2-1));
					newy   = yr2 + (int)(yr2*XF3p);
					prof_leave(PROF_ADDRESS_GENERATE);
					if ((isfloor && newy >= opty)||((!isfloor) && newy <= opty)) continue;
					prof_enter(PROF_INTERPOL_INIT);
					if (cpu.sse) {
						color = bilinea4(tex[index], tex[index+txadd], tex[index2], tex[index2+txadd],
							x & 0xffff, z & 0xffff);
					} else {
						color = bilinea_p5(tex[index], tex[index+txadd], tex[index2], tex[index2+txadd],
							x & 0xffff, z & 0xffff);
					}
					if (lastcol == 0xbabababa) lastcol = color;
					if (isfloor) { // newy must be no less than 1
						newy = newy>=1 ? newy : 1;
					} else { //newy must be no more than yr-2
						newy = newy<= yr - 2 ? newy : yr-2;
					}
					newymp = newy * xr + swOne*xr;
					opty += swOne;
					int oldc[3], newc[3];
					decompose(lastcol, oldc); decompose(color, newc);
					int steps = swOne*(newy-lasty);
					int stepsrcp = (int) (65536.0f/steps);
					int incc[3] = {
						(((newc[0]-oldc[0])>>8)*stepsrcp)/256,
						(((newc[1]-oldc[1])>>8)*stepsrcp)/256,
						(((newc[2]-oldc[2])>>8)*stepsrcp)/256
						};
					oldc[0] += swOne*(opty-lasty)*incc[0];
					oldc[1] += swOne*(opty-lasty)*incc[1];
					oldc[2] += swOne*(opty-lasty)*incc[2];
					prof_leave(PROF_INTERPOL_INIT);
					prof_enter(PROF_INTERPOLATE);
					if (lasty!=999666111) {
						j = opty * xr;
						if (isfloor) {
							do {
								c_column[j] =
										((oldc[0]&0xff0000) >> 16) |
										((oldc[1]&0xff0000) >> 8 ) |
										(oldc[2]&0xff0000);
								j -= xr;
								oldc[0] += incc[0];
								oldc[1] += incc[1];
								oldc[2] += incc[2];
							} while (j>newymp);
						} else {
							do {
								c_column[j] =
										((oldc[0]&0xff0000) >> 16) |
										((oldc[1]&0xff0000) >> 8 ) |
										(oldc[2]&0xff0000);
								j += xr;
								oldc[0] += incc[0];
								oldc[1] += incc[1];
								oldc[2] += incc[2];
								
							} while (j<newymp);
						}
					}
					opty = newy;
					lastcol = color;
					prof_leave(PROF_INTERPOLATE);
				}
			}
			fx += cpu.count*fxi; fz += cpu.count*fzi;
		}
	}
	/*if (!thread_index) {
		show_light(fb, xr, yr);
	}*/
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
			if (bilfilter) {
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
	int xr2 = xr/2;
	int yr2 = yr/2;
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
	voxel_light_recalc(thread_index);
	if (voxel_rendering_method) {
		voxel_single_frame_do2(fb, thread_index, tt, ti, tti);
	} else {
		voxel_single_frame_do1(fb, thread_index, tt, ti, tti);
	}
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
		lx = (int) (256 + 128 * sin(bTime()*0.1));
		lz = (int) (256 + 128 * cos(bTime()*0.1));
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
