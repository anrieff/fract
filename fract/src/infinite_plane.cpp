/***************************************************************************
 *   Copyright (C) 2004 by Veselin Georgiev                                *
 *   vesko@ViruS                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *-------------------------------------------------------------------------*
 * Contains the code that renders the (up to 1.05b default) floor and      *
 * ceiling. Pure mathematical infinite (X,Z) plane with tiled texture      *
 ***************************************************************************/

#include <math.h>
#include <string.h>
#include <assert.h>

#include "infinite_plane.h"
#include "bitmap.h"
#include "common.h"
#include "cpu.h"
#include "cvars.h"
#include "fract.h"
#include "gfx.h"
#include "profile.h"
#include "render.h"
#include "sphere.h"
#include "vectormath.h"

#include "cross_vars.h"


#define fract_asm
#include "x86_asm.h"
#undef fract_asm


const double Area_const_isotrophic = 1.61;
const double Area_const_anisotrophic = 4.12;
double Area_const;
int do_mipmap  = 1;
FilteringInfo def_finfo;


/* integrated lighting and Multiplication routine.
 * this routine replaces the following code:
 *
 *	screencolor = multiplycolor(texturecolor, lform(Lx, Ly, lx, ly))
 * being used somewhere in DrawIt.
 *
 * uses an integrated assembly code in x86_asm.h (using SSE) if the required instruction set is available
 */
Uint32 integrated( Uint32 texturecolor, int x, int y, int lx, int ly, int ysqrd_raytrace)
{
 if ( cpu.sse )  return igrated(texturecolor, x, y, lx, ly, ysqrd_raytrace);
 		else return multiplycolor(texturecolor, lform(x, y, lx, ly, ysqrd_raytrace));
}

/* This is called by the raytracer - gets a texture color at coords (x, 0, y) */

Uint32 texture_handle_nearest(int x, int y, int level, int ysqrd_raytrace)
{
 int *adata= (int*) T[level].get_data();
 return integrated(adata[((x & 255)>>level) +
 /*y*/(((y & 255)>>level)<<(LOG2_MAX_TEXTURE_SIZE-level))],x, y, lx, lz, ysqrd_raytrace);
}

Uint32 texture_handle_bilinear(double x, double y, int level, int ysqrd_raytrace)
{
 Uint32 x1y1, x2y1, x1y2, x2y2;
 unsigned *adata = (unsigned*) T[level].get_data();
 unsigned off, mask;
 unsigned xi = (unsigned)((int)floor(x));
 unsigned yi = (unsigned)((int)floor(y));
 unsigned atexandmask = MAX_TEXTURE_SIZE - 1;
 unsigned tm = (MAX_TEXTURE_SIZE >> level) - 1;
 unsigned Xi = (xi & atexandmask) >> level;
 unsigned Yi = (yi & atexandmask) >> level;
        /*x*/               /*y*/
 off = Xi + (Yi<<(LOG2_MAX_TEXTURE_SIZE-level));
 mask = (1 << (2*(LOG2_MAX_TEXTURE_SIZE-level))) - 1;
 x1y1 = adata[off];
 x2y1 = adata[(off+1)&mask];
 off += tm + 1;
 x1y2 = adata[off & mask];
 x2y2 = adata[(off+1)&mask];
 x=ldexp(x, -level); y=ldexp(y, -level);
 x-=floor(x); y-=floor(y);
 if (cpu.sse) {
 	return igrated(bilinea4(x1y1, x2y1, x1y2, x2y2, (int) (65535.0*x), (int) (65535.0*y)), xi, yi, lx, lz, ysqrd_raytrace);
	} else {
	return  multiplycolor(
		multiplycolorf(	x1y1, (1-x)*(1-y))+
		multiplycolorf(	x2y1,    x *(1-y))+
		multiplycolorf(	x1y2, (1-x)*   y )+
		multiplycolorf(	x2y2,    x *   y )
			, lform(xi, yi, lx, lz, ysqrd_raytrace));
	}
}

/*
function bilinea_p5:

does bilinear filtering on the four points (x0y0, x1y0, x0y1 and x1y1) using Fx and Fy as .16 fixedpoint represenation
of the coordinates. Basically, the routine does what the equivallent code from render.cpp did in versions prior to 1.03c:

		Tc = multiplycolor(T[xtex].data[boffset				], i2im(65536-Fx,65536-Fy))+
		multiplycolor(T[xtex].data[(boffset+1) &texandmask		], i2im(Fx  ,65536-Fy))+
		multiplycolor(T[xtex].data[(boffset+xandmask+1)&texandmask	], i2im(65536-Fx,  Fy))+
		multiplycolor(T[xtex].data[(boffset+xandmask+2)&texandmask	], i2im(Fx  ,  Fy));

	the speedup comes from the fact, that instead of 4 unpacks and 4 repacks(as in the above version),
	only one repack is done. Also, it attempts to exploit parallelism on the CPU's that have deep pipelines,
	but the only pre-SSE CPUs which could have pipelines that deep are maybe just the AMD K7s (T-Birds and Spitfires)
*/
Uint32 bilinea_p5(Uint32 x0y0, Uint32 x1y0, Uint32 x0y1, Uint32 x1y1, unsigned Fx, unsigned Fy)
{
	unsigned m0, m1, m2, m3;
	int r0, g0, b0, r1, g1, b1;
	Uint32 rez=0;

	if (cpu.mmx) {
	/* do we have MMX? If we do, we may use the crappy MMX bilinear filter, which, due to lack of
	the pshufw instruction is kinda slow...
	*/
		Fx>>=8; Fy>>=8;
#define bilinea_p5_asm
#include "x86_asm.h"
#undef bilinea_p5_asm
		return rez;
	}
// non-mmx version
	m1 = m0 = 65536 - Fy;
	m2 = 65536 - (m3=Fx);
	m0 *= m2;
	m1 *= Fx;
	m2 *= Fy;
	m3 *= Fy;
	m0 >>= 16; m1>>=16; m2 >>= 16; m3>>=16;

	b0  = m0 * (x0y0&0xff); x0y0 >>= 8;
	g0  = m0 * (x0y0&0xff); x0y0 >>= 8;
	r0  = m0 * (x0y0&0xff);
	b1  = m1 * (x1y0&0xff); x1y0 >>= 8;
	g1  = m1 * (x1y0&0xff); x1y0 >>= 8;
	r1  = m1 * (x1y0&0xff);

	b0 += m2 * (x0y1&0xff); x0y1 >>= 8;
	g0 += m2 * (x0y1&0xff); x0y1 >>= 8;
	r0 += m2 * (x0y1&0xff);
	b1 += m3 * (x1y1&0xff); x1y1 >>= 8;
	g1 += m3 * (x1y1&0xff); x1y1 >>= 8;
	r1 += m3 * (x1y1&0xff);


	b0 += b1; g0 += g1; r0 += r1;
	b0 >>= 16; g0>>=16; r0>>=16;
	return (b0 + (g0 << 8) + (r0 << 16));
}



#ifdef _MSC_VER
// returns the 2 base logarithm of x
double log2(double x)
{
	return log(x) * 1.4426950408889634073599246810019;
}
#endif
/*
 *	Chooses which texture instance to use based on the distance of the row from the camera and the angle
 *	the ray from the camera encloses with that row.
 */
void choose_texture(const Vector& cur, int xr, int yr, double cx, double cxi, double cy, double cyi, double hx, double hy,
			double dp, int *xtex, int *xandmask, int *xshl, int *tam, int *notmask
			)
{
	double parea; // parea is the area (in square texels) which single ray covers
	int i;
	double aa = sqrt(cxi * cxi + cyi * cyi), bb = sqrt(sqr(cx - hx) + sqr(cy - hy));
	
	if (!CVars::anisotrophic) { // isotrophic/anisotrophic selection
		parea = aa * bb; // old/ugly - measure rectangle size
	} else {
		if (aa > bb) parea = aa * aa; // new - measure square size of the larger side
			else parea = bb * bb;
	}
	*xtex = 0; *xandmask = MAX_TEXTURE_SIZE; *xshl = (int) log2(*xandmask);
	/*
		The idea is simple. Since we want to bilinear filter (or do nearest neighbour) all the time
		we do want a single ray to cover an area of approx 1 sq texel, we get the area a ray will have
		on the 256x256 texture. If the area is above `Area_const' (an meaningful value [1..4]) we want
		to get a smaller texture - a 128x128 version mipmap will have 4 times less texels, resulting in
		four times less area. So we get lower texture sizes and divide the area by 4 each time, until
		the area is acceptable or we reached using 1x1 texture.
	*/
	if (do_mipmap)
	while (parea>Area_const && *xtex<end_tex) {
		(*xtex)++; (*xandmask)/=2; (*xshl)--;
		parea /= 4.0;
		}
		else
	for (i=0;i<2;i++) {
		(*xtex)++; (*xandmask)/=2; (*xshl)--;
		parea /= 4.0;
	}
	(*xandmask)--;
	*notmask = (0xffffffff >> (LOG2_TEXTURE_AREA+*xtex)) << (LOG2_TEXTURE_AREA+*xtex);
	*tam = (1 << (2*(*xshl))) - 1;
}

/*
	render_infinite_plane_sse:

	This is a part of the SSE version of the renderer. This creates an image of the floor (and the ceiling)
	only. Shadow-casting is not performed here. Uses assembly-optimized code, with loop unrolling and not
	a single IF in the whole routine :)))

 */

#ifdef USE_ASSEMBLY
SSE_ALIGN(static const float c64k[4]) =
	{BRIGHTNESS_C, BRIGHTNESS_C, BRIGHTNESS_C, BRIGHTNESS_C};
SSE_ALIGN(static const unsigned short my_ffs[8]) =
	{0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0x0, 0xffff, 0x0};

#endif

void render_infinite_plane_sse(Uint32 *fb, int xr, int yr, const Vector& in_tt, const Vector& ti, const Vector& in_tti, int thread_idx, InterlockedInt& lock)
{
	Vector tt;
	Uint32 *dptr;
	int i, j;
	float fx[4], fy[4];
	float fxi[4];
	float fyi[4];
	double ydist, dp, scalefactor, cx, cy, cxi, cyi, hx, hy;
	Vector ut;
	int xtex, xandmask, xshl, texandmask, notmask;
	int xxandmask[2];
	Uint32 *ptex;

	// these alignments are needed to use movaps intead of movups.
	SSE_ALIGN(float fxyi[8]);
	SSE_ALIGN(float lightxy1[16]);

#ifdef USE_ASSEMBLY
	SSE_ALIGN(int save[4]);
#endif
	SSE_ALIGN(int gx[4]);
	SSE_ALIGN(int gy[4]);
	SSE_ALIGN(int gxx[8]);
	//printf("thread #%d ran render_floor(); addr. of lightxy1 = 0x%X\n", start_line, (unsigned) lightxy1);	fflush(stdout);
	

	while ((j = lock++) < yr) {
	//for (j = thread_idx; j < yr; j += cpu.count) {
		tt = in_tt + in_tti * j;
		dptr = &fb[j*xr];
		Vector g1 = tt;
		ut.make_vector(g1, in_tti);
		if ((ydist=vfabs(cur[1]-tt[1]))>DST_THRESHOLD) {
				prof_enter(PROF_INIT_PER_ROW);
			dp = (cur[1]>tt[1]?(cur[1]-daFloor):(daCeiling-cur[1]));

			// load y distance, squared
			lightxy1[15] = lightxy1[14] = lightxy1[13] = lightxy1[12] = (cur[1]>tt[1]?ysqrd_floor:ysqrd_ceil);

			scalefactor = dp/ydist;
			cx = cur[0] + (tt[0]-cur[0])*scalefactor;
			cy = cur[2] + (tt[2]-cur[2])*scalefactor;
			cxi = (cur[0]+(tt[0]+ti[0]*xr-cur[0])*scalefactor - cx)/((double) xr);
			cyi = (cur[2]+(tt[2]+ti[2]*yr-cur[2])*scalefactor - cy)/((double) yr);
			// calculate second "helping" point from the previos image row
			scalefactor = (cur[1]>ut[1]?(cur[1]-daFloor):(daCeiling-cur[1])) / vfabs(cur[1]-ut[1]);
			hx = cur[0] + (ut[0]-cur[0])*scalefactor;
			hy = cur[2] + (ut[2]-cur[2])*scalefactor;

			for (i=0;i<4;i++) {
				gxx[i] = (int) (cxi*262144.0);
				gxx[i+4] = (int) (cyi*262144.0);
				gx[i] = (int) (cx*65536.0) + (int) (i*cxi*65536.0);
				gy[i] = (int) (cy*65536.0) + (int) (i*cyi*65536.0);
				fx[i] = cx + i*cxi;
				fy[i] = cy + i*cyi;
				fxi[i] = cxi*4;
				fyi[i] = cyi*4;
				lightxy1[i] = lx;
				lightxy1[i+4] = lz;
				lightxy1[i+8] = 1.0;
				fxyi[  i] = fxi[i];
				fxyi[4+i] = fyi[i];
			}
			// choose apropriate texture from the T array:
			choose_texture(cur, xr, yr, cx, cxi, cy, cyi, hx, hy, dp,
				&xtex, &xandmask, &xshl, &texandmask, &notmask);
			xxandmask[0] = xxandmask[1] = xandmask;
			// actual rendering follows:
			ptex = (Uint32 *) T[xtex].get_data();
			i = xr/4;
				prof_leave(PROF_INIT_PER_ROW);
				prof_enter(PROF_WORK_PER_ROW);
			// load some useful values:
#define render1
#include "x86_asm.h"
#undef render1
			if (CVars::bilinear) {
				while (i--) {
#define render2
#include "x86_asm.h"
#undef render2
					dptr+=4;
				}
			} else {
			// NEAREST NEIGHBOUR FILTER VERSION:
				while (i--) {
#define render3
#include "x86_asm.h"
#undef render3
					dptr += 4;
				}
			}
			prof_leave(PROF_WORK_PER_ROW);
		} else { // fill the row with zeros:
			memset(dptr, 0, 4*xr);
			dptr+=xr;
		}
#define emit_emms
#include "x86_asm.h"
#undef emit_emms
	}
}

/*
 *  This is the Pre-SSE version of the floor renderer. Contains C code only :)
 */
void render_infinite_plane_p5(Uint32 *fb, int xr, int yr, const Vector& in_tt, const Vector& ti, const Vector& in_tti, int thread_idx, InterlockedInt &lock)
{
	Vector tt = in_tt, tti = in_tti;
	Uint32 *dptr;
	int i, j;
	double ydist, dp, scalefactor, cx, cy, cxi, cyi, hx, hy;
	Vector ut;
	int xtex, xandmask, xshl, texandmask, notmask;
	Uint32 *ptex;
	int gX, gY, Bx, By, Fx, Fy, boffset, Tc;
	int gxx[2];
	int SavergX, SavergY, Lr=0, Lx=0, Ly=0;

	Vector tt_start = tt;
	
	while ((j = lock++) < yr) {
		tt = tt_start + tti * j;
		dptr = &fb[j*xr];
		Vector g1 = tt;
		ut.make_vector(g1, tti);
		if ((ydist=vfabs(cur[1]-tt[1]))>DST_THRESHOLD) {
			prof_enter(PROF_INIT_PER_ROW);
			dp = (cur[1]>tt[1]?(cur[1]-daFloor):(daCeiling-cur[1]));

			// load y distance, squared
			ysqrd = (cur[1]>tt[1]?ysqrd_floor:ysqrd_ceil);

			scalefactor = dp/ydist;
			cx = cur[0] + (tt[0]-cur[0])*scalefactor;
			cy = cur[2] + (tt[2]-cur[2])*scalefactor;
			cxi = (cur[0]+(tt[0]+ti[0]*xr-cur[0])*scalefactor - cx)/((double) xr);
			cyi = (cur[2]+(tt[2]+ti[2]*yr-cur[2])*scalefactor - cy)/((double) yr);
			// calculate second "helping" point from the previos image row
			scalefactor = (cur[1]>ut[1]?(cur[1]-daFloor):(daCeiling-cur[1])) / vfabs(cur[1]-ut[1]);
			hx = cur[0] + (ut[0]-cur[0])*scalefactor;
			hy = cur[2] + (ut[2]-cur[2])*scalefactor;

			gxx[0] = (int) (cxi*65536.0);
			gxx[1] = (int) (cyi*65536.0);
			gX = (int) (cx*65536.0);
			gY = (int) (cy*65536.0);
			// choose apropriate texture from the T array:
			choose_texture(cur, xr, yr, cx, cxi, cy, cyi, hx, hy, dp,
				&xtex, &xandmask, &xshl, &texandmask, &notmask);
			// actual rendering follows:
			ptex = (Uint32 *) T[xtex].get_data();
			i = xr;
			prof_leave(PROF_INIT_PER_ROW);
			prof_enter(PROF_WORK_PER_ROW);
			if (CVars::bilinear) {
				do {
					Bx = (gX<0?(notmask|(gX>>(16+xtex))):(gX>>(16+xtex)));
					By = (gY<0?(notmask|(gY>>(16+xtex))):(gY>>(16+xtex)));
					Fx = ((gX>>xtex)&0xffff);
					Fy = ((gY>>xtex)&0xffff);
					gX += gxx[0];
					gY += gxx[1];
					boffset = ((Bx & xandmask)) + (((By & xandmask))<<xshl);
					int _1 = ((Bx & xandmask) == xandmask) ? -xandmask : 1;
					Tc = bilinea_p5(	ptex[boffset				],
								ptex[(boffset+_1) &texandmask		],
								ptex[(boffset+xandmask+1)&texandmask	],
								ptex[(boffset+xandmask+1+_1)&texandmask	],
								Fx, Fy);
					*(dptr++) = multiplycolor(Tc, lform(r(gX), r(gY), lx, lz, ysqrd));
				} while (--i);
			} else {
			// NEAREST NEIGHBOUR FILTER VERSION:
				do {
					SavergX = r(gX); SavergY = r(gY);
					gX += gxx[0];
					gY += gxx[1];
					if (SavergX==Lx && SavergY==Ly)
						*(dptr++) = Lr;
						else{
						Lx=SavergX; Ly=SavergY;
						*(dptr++) = Lr = multiplycolor(
								ptex[
									(((unsigned) (Lx)>>xtex) & xandmask) +
									(((((unsigned)(Ly)>>xtex) & xandmask))<<xshl)
								],
							lform(Lx, Ly, lx, lz, ysqrd));
					}
				} while (--i);
			}
			prof_leave(PROF_WORK_PER_ROW);
		} else { // fill the row with zeros:
			memset(dptr, 0, 4*xr);
		}
	}
}

void infinite_plane_perframe_init(void)
{
	if (CVars::anisotrophic) {
		Area_const = Area_const_anisotrophic;
	} else {
		Area_const = Area_const_isotrophic;
	}
}

void render_infinite_plane(Uint32 *frame_buffer, int xr, int yr, Vector& tt, Vector& ti, Vector& tti, 
			   int start_line, InterlockedInt &lock)
{
	if (cpu.sse) {
		render_infinite_plane_sse(frame_buffer, xr, yr, tt, ti, tti, start_line, lock);
	} else {
		render_infinite_plane_p5(frame_buffer, xr, yr, tt, ti, tti, start_line, lock);
	}
}

