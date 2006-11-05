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
#define MIN_SIZE 4

/* OUTER_MIPLEVEL
** determine the outer mip-level for voxel traversal algorithm
** 2^OUTER_MIPLEVEL is the size of the blocks which will be speedily skipped
** if there is opportunity for. It's good to make it half the log2 of the
** heightmap size.
*/
#define OUTER_MIPLEVEL 4
#define OUTER_SIZE (1 << OUTER_MIPLEVEL)


extern int vframe;
struct subdivision_t {
	float dist;
	int index;
};

#define InTexture(x,y) (x>=0.0f&&x<size-1&&y>0&&y<size-1)


/**
*** @class Hierarchy
*** @author Veselin Georgiev
*** @date 2005-08-12
*** @short Implements ray-heightfield intersections using Quadtree structure
***
**/
class Hierarchy {
	float *maps[12]; // support textures up to 4096x4096
	float *umaps[7];
	float *sdmap;
	int size;
	int slog, s; //log2(size)
	bool is_floor;
	int blog;

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
	
	bool is_ok(int i0, int j0, int i1, int j1, int blocksize, int dx) const;
	void build_umaps(float *);
	void free_umaps(void);
	void process(float &r, int i0, int j0, int i1, int j2, int ll, int dx);
	bool try_load_map_from_cache(float *map);
	void insert_map_in_cache(float *map);

public:
	Hierarchy(){
		memset(maps, 0, sizeof(maps));
		size = slog = 0;
		raycasts = recursions = 0;
		tot = eff = 0;
		sdmap = NULL;
	}
	~Hierarchy() {
		if (sdmap) {
			free(sdmap);
		}
		sdmap = NULL;
		for (int i=0;i<=slog;i++) {
			if (maps[i]) free(maps[i]);
			maps[i] = NULL;
		}
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
	bool build_hierarchy(int thesize, float *thebuff, bool is_floor);

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
	float ray_intersect(const Vector & orig, const Vector & proj, Vector & crossing);

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
