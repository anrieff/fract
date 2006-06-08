/***************************************************************************
 *   Copyright (C) 2005 by Veselin Georgiev                                *
 *   vesko@workhorse                                                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

 
#ifndef __HIERARCHY_H__
#define __HIERARCHY_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"

#define CEIL_MAX 9999

/* OUTER_MIPLEVEL
** determine the outer mip-level for voxel traversal algorithm
** 2^OUTER_MIPLEVEL is the size of the blocks which will be speedily skipped
** if there is opportunity for. It's good to make it half the log2 of the
** heightmap size.
*/
#define OUTER_MIPLEVEL 4
#define OUTER_SIZE (1 << OUTER_MIPLEVEL)


extern int bilfilter, vframe;
struct subdivision_t {
	float dist;
	int index;
};

static float fmax4(float a, float b, float c, float d)
{
	float ff = a;
	ff = b > ff ? b : ff;
	ff = c > ff ? c : ff;
	ff = d > ff ? d : ff;
	return ff;
}

static float fmin4(float a, float b, float c, float d)
{
	float ff = a;
	ff = b < ff ? b : ff;
	ff = c < ff ? c : ff;
	ff = d < ff ? d : ff;
	return ff;
}

#define InTexture(x,y) (x>=0.0f&&x<size-1&&y>0&&y<size-1)

// this ugly macro is used by the Ray-heightfield intersection routine, DON'T DELETE!
#define INTERNAL 			float maxd = fmin(tMaxXsm, tMaxZsm); \
					cdist += 0.0001;\
					X = (int) (Camera[0] + cdist * diff0); \
					Z = (int) (Camera[2] + cdist * diff2); \
					Y = Yi + cdist * diff1; \
					if (enter_d == fromX) { \
						tMaxX = cdist + tDeltaX; \
						tMaxZ = cdist + (((stepZ+1)>>1)+Z-(Camera[2] + cdist * diff2))*rcpdiff2; \
					} \
					if (enter_d == fromZ) { \
						tMaxX = cdist + (((stepX+1)>>1)+X-(Camera[0] + cdist * diff0))*rcpdiff0; \
						tMaxZ = cdist + tDeltaZ; \
					} \
					while (tMaxX < maxd || tMaxZ < maxd) { \
						if ((ilast = maps[slog][X + (Z<<slog)])>Y) { \
							crossing = Vector(X, ilast, Z); \
							return orig.distto(crossing); \
						} \
						if (tMaxX < tMaxZ) { \
							Y = Yi + tMaxX * diff1; \
							if (Y < ilast) { \
								crossing =  Vector(X, ilast, Z); \
								return orig.distto(crossing); \
							} \
							tMaxX += tDeltaX; \
							X += stepX; \
						} else { \
							Y = Yi + tMaxZ * diff1; \
							if (Y < ilast) { \
								crossing = Vector(X, ilast, Z); \
								return orig.distto(crossing); \
							} \
							tMaxZ += tDeltaZ; \
							Z += stepZ; \
						} \
					} \

/**
*** @class Hierarchy
*** @author Veselin Georgiev
*** @date 2005-08-12
*** @short Implements ray-heightfield intersections using Quadtree structure
***
**/
class Hierarchy {
	float *maps[12]; // support textures up to 4096x4096
	int size;
	int slog; //log2(size)
	bool floor;

	int raycasts, recursions, tot, eff;


	float getheight_bilinear(float x, float y)
	{
		float fx = modff(x, &x);
		float fy = modff(y, &y);
		int base = (int) y * size + (int) x;
		return
			maps[slog][base		] * (1.0f - fx) * (1.0f - fy) +
			maps[slog][base	+ 1	] * (       fx) * (1.0f - fy) +
			maps[slog][base	+ size	] * (1.0f - fx) * (       fy) +
			maps[slog][base	+ size+1] * (       fx) * (       fy);
	}

	float getheight_nearest(float x, float y, float BAD_VAL)
	{
		if (x < 0.0f || x > size - 1 || y < 0.0f || y > size - 1)
			return BAD_VAL;
		return maps[slog][(int) y * size + (int) x];
	}

public:
	Hierarchy(){
		memset(maps, 0, sizeof(maps));
		size = slog = 0;
		raycasts = recursions = 0;
		tot = eff = 0;
	}
	~Hierarchy() {
		for (int i=0;i<=slog;i++)
			free(maps[i]);
#ifdef DEBUG
		if (raycasts) {
			printf("%d raycasts, %d recursions, average %.1Lf recursions per raycast\n",
				raycasts, recursions, (long double) recursions / raycasts);
			printf("%d raycasts per frame; average %.3lf%% of pixels are ray-traced\n", raycasts / vframe,
				raycasts/vframe / (640.0*4.80));
		}
#endif
	}

	/// builds the quadtree
	/// @param thesize size of the input heightmap
	/// @param thebuff the heightmap itself
	/// @param is_floor whether to build a floor or a ceiling quadtree
	///                 the difference is in whether the highest or the
	///                 lowest point is taken
	/// @returns TRUE on success, FALSE on failure
	bool build_hierarchy(int thesize, float *thebuff, bool is_floor)
	{
		float (*ffun) (float,float,float,float) = is_floor ? fmax4 : fmin4;
		floor = is_floor;
		size = thesize;
		slog = power_of_2(size);
		int allocsize = 16* size*size;
		//maps[slog ] = thebuff;

		int csize = size * 2;
		for (int i= slog; i>=0; i--) {
			allocsize /= 4;
			int oldsize = csize;
			csize /= 2;

			maps[i] = (float *) malloc(allocsize);
			if (maps[i] == NULL) {
				i++;
				while (i < slog) {
					free(maps[i]);
					i++;
				}
				return false;
			}

			// memory allocated; fill with usefull info
			if (i == slog) {
				memcpy(maps[i], thebuff, allocsize);
				continue;
			}
			int p = 0;
			for (int y = 0; y < csize; y++)
				for (int x = 0; x < csize; x++,p++) {
					maps[i][p] = ffun(
							maps[i+1][( y * 2      * oldsize) + x * 2    ],
							maps[i+1][( y * 2      * oldsize) + x * 2 + 1],
							maps[i+1][((y * 2 + 1) * oldsize) + x * 2    ],
							maps[i+1][((y * 2 + 1) * oldsize) + x * 2 + 1]
							);
				}
		}
		return true;
	}

	enum EnterDirection {
		fromX, fromZ, fromNowhere
	};

	/** RayIntersect
	*** Traces an intersection of the ray originating from `orig', passing through proj
	*** and (eventually) hitting the heightmap. If the intersection exists,
	*** the distance from it to the `orig' is returned and `crossing' is set to the point
	*** of the intersection. Returns very large depth (more than 1e6) otherwise
	*** ***
	*** @param orig the origin point (you may want this set to the camera coordinates)
	*** @param proj the point from the vector projection grid
	*** @param crossing the result will be written here if intersection occurs
	*** ***
	*** @returns 1e9 if no intersection, orig.distto(crossing) otherwise
	**/

	/*
		The algorithm heavily relies on the ideas from "A Fast Voxel Traversal Algorithm (for Raytracing)"
		The paper:  http://www.cs.yorku.ca/~amana/research/grid.pdf

		It is modified to take mip-mapping into account. The whole 512x512 heightmap divided to
		32x32 field of 16x16 blocks; i.e. the 4th mipmap level is taken (based on some benchmarks).
		The Voxel Traversal Algorithm is applied on the 32x32 field and if possibility for intersection
		exists, then the same algorithm is applied inside the 16x16 block. If the intersection is
		not found there, the "Large" walk continues.
		We achieve about 1.1 M Intersections tests per second for adjascent texels and 800k for random ones.
		The tests are on a 2.43 Ghz Athlon 64
	*/
	// all comments in this routine assume we're raycasting a floor heightmap. For the ceiling equivallent,
	// just "reverse" all `below' statements

	float ray_intersect(const Vector & orig, const Vector & proj, Vector & crossing)
	{
		++raycasts;
		bool coords_inited = false;
		int X, Z, stepX, stepZ, Xsm, Zsm; // all *sm variables are used in the "Large" walk
		float tMaxX, tMaxZ, tDeltaX, tDeltaZ, last, ilast;
		float tMaxXsm, tMaxZsm, tDeltaXsm, tDeltaZsm, cdist;
		EnterDirection enter_d = fromNowhere;
		Vector Camera; // camera is where the first point above the heightfield is

		/* Check for rays which go straight in the sky */
		if (floor) {
			if (proj[1] > orig[1] && orig[1] > maps[0][0]) {
				return 1e9;
			}
		} else {
			if (proj[1] < orig[1] && orig[1] < maps[0][0])
				return 1e9;
		}

		/* check whether we are "below" the terrain */
		if (InTexture(orig.v[0], orig.v[2])) {
			coords_inited = true;
			Camera = orig;
			X = (int) orig[0];
			Z = (int) orig[2];
			if (floor) {
				if (getheight_bilinear(orig.v[0], orig.v[2]) >= orig.v[1])
					return 1e9;
			} else {
				if (getheight_bilinear(orig.v[0], orig.v[2]) <= orig.v[1])
					return 1e9;
			}
		} else {
			X = Z = 0; // make gcc happy
		}
		/* traversal vector in diff */
		Vector diff;
		diff.make_vector(proj, orig);

		// precalculate stuff...
		float rcpdiff0 = 1.0f / diff[0], rcpdiff2 = 1.0f / diff[2];
		float rcp640 = rcpdiff0 * OUTER_SIZE, rcp642 = rcpdiff2 * OUTER_SIZE;

		/* If we're outside the grid, find where we enter it */
		if (!coords_inited) {
			float p[4];
			p[0] =       - orig[0]  * rcpdiff0;
			p[1] = (size - orig[0]) * rcpdiff0;
			p[2] =       - orig[2]  * rcpdiff2;
			p[3] = (size - orig[2]) * rcpdiff2;
			float best = 1e9;
			for (int i = 0; i < 4; i++)
				if (p[i] > 0 && p[i] < best) best = p[i];
			if (best > 1e6)
				return 1e9;
			Camera.macc(orig, diff, best);
			X = (int) Camera[0];
			Z = (int) Camera[2];
		}
// prepare our OUTER loop
		int out = slog - OUTER_MIPLEVEL, Gsize = ~(size / OUTER_SIZE - 1);
		Xsm = X / OUTER_SIZE;
		Zsm = Z / OUTER_SIZE;
		Vector cm64 = Camera;
		cm64.scale(1.0/OUTER_SIZE);
		stepX = diff[0]>0?+1:-1;
		stepZ = diff[2]>0?+1:-1;
		tMaxX = (((stepX+1)>>1)+X-Camera[0])*rcpdiff0;
		tMaxZ = (((stepZ+1)>>1)+Z-Camera[2])*rcpdiff2;
		tMaxXsm = (((stepX+1)>>1)+Xsm-cm64[0])*rcp640;
		tMaxZsm = (((stepZ+1)>>1)+Zsm-cm64[2])*rcp642;

		tDeltaX = fabsf(rcpdiff0);
		tDeltaZ = fabsf(rcpdiff2);
		tDeltaXsm = OUTER_SIZE * tDeltaX;
		tDeltaZsm = OUTER_SIZE * tDeltaZ;
		float Yi = Camera[1];
		float Y = Yi, Ysm = Yi, diff0 = diff[0], diff1 = diff[1], diff2 = diff[2];
		cdist = 0;
		while ((Xsm & Gsize) == 0 && (Zsm & Gsize) == 0) {
			if ((last = maps[out][Xsm + (Zsm << out)]) > Ysm) {
				// possibility for intersection detected, examine closely...
				INTERNAL
			}
			if (tMaxXsm < tMaxZsm) {
				Ysm = Yi + tMaxXsm * diff1;
				if (Ysm < last) {
				// possibility for intersection detected, examine closely...
					INTERNAL
				}
				cdist = tMaxXsm;
				enter_d = fromX;

				tMaxXsm += tDeltaXsm;
				Xsm += stepX;
			} else {
				Ysm = Yi + tMaxZsm * diff1;
				if (Ysm < last) {
				// possibility for intersection detected, examine closely...
					INTERNAL
				}
				cdist = tMaxZsm;
				enter_d = fromZ;
				tMaxZsm += tDeltaZsm;
				Zsm += stepZ;
			}
		}
		return 1e9;
	}
};

#endif //__HIERARCHY_H__


		// this abandoned version is the single-level one. It achieves about 500k on adjascent texels
		/*while ((X & ~(size-1)) == 0 && (Z & ~(size-1)) == 0) {
			if ((last=maps[slog][X + (Z << slog)]) > Y) {
			// found solution
				crossing = Vector(X, last, Z);
				return orig.distto(crossing);
			}
			if (tMaxX < tMaxZ) {
				Y = Yi + tMaxX * diff1;
				if (Y < last) {
					crossing = Vector(X, last, Z);
					return orig.distto(crossing);
				}
				tMaxX += tDeltaX;
				X += stepX;
			} else {
				Y = Yi + tMaxZ * diff1;
				if (Y < last) {
					crossing = Vector(X, last, Z);
					return orig.distto(crossing);
				}
				tMaxZ += tDeltaZ;
				Z += stepZ;
			}
		}*/
