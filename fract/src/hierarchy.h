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
	float *field;
	float *quadrant[4];
	float highestpeak;
	int size;
	int slog, s; //log2(size)
	bool is_floor;

public:
	Hierarchy(){
		size = slog = 0;
		memset(quadrant, 0, sizeof(quadrant));
		field = NULL;
	}
	~Hierarchy() {
		if (field) {
			free(field);
			field = NULL;
		}
		for (int i = 0; i < 4; i++) {
			if (quadrant[i]) free(quadrant[i]);
			quadrant[i] = NULL;
		}
	}

	/// builds the quadtree
	/// @param thesize size of the input heightmap
	/// @param thebuff the heightmap itself
	/// @param is_floor whether to build a floor or a ceiling quadtree
	///                 the difference is in whether the highest or the
	///                 lowest point is taken
	/// @returns TRUE on success, FALSE on failure
	bool build_hierarchy(int thesize, float *thebuff, bool is_floor);

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

	float ray_intersect(const Vector & orig, const Vector & proj, Vector & crossing);

};

#endif //__HIERARCHY_H__
