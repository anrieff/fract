/***************************************************************************
 *   Copyright (C) 2006 by Veselin Georgiev                                *
 *   anrieff@mailg.com (convert to gmail)                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *  ---------------------------------------------------------------------  *
 *                                                                         *
 *    Defines a bounding box class                                         *
 *                                                                         *
***************************************************************************/

#ifndef __BBOX_H__
#define __BBOX_H__

#include "vector3.h"
#include "triangle.h"

#ifndef fmax
#	define fmax(a,b) ((a)>(b)?(a):(b))
#endif

/**
 * @class BBox
 * @brief Simple bounding box class; used to perform faster intersection tests
 * @author Veselin Georgiev
 * @date 2005-12-14
 *
 * Usage: Derive a class from BBox. When updating, moving or scaling the
 *        geometry in your derived class, call BBox::recalc, then ::add()
 *        all the geometry's vertices.
 *        After done this, you can use the TestIntersect method
 */
struct BBox {
	Vector vmin, vmax;
	
	BBox()
	{
		recalc();
	}
	void recalc(void)
	{
		vmin = Vector(+1e9, +1e9, +1e9);
		vmax = Vector(-1e9, -1e9, -1e9);
	}
	void add(const Vector & v)
	{
		for (int i = 0; i < 3; i++) {
			vmin.v[i] = vmin.v[i] > v[i] ? v[i] : vmin.v[i];
			vmax.v[i] = vmax.v[i] < v[i] ? v[i] : vmax.v[i];
		}
	}
	void add(const Triangle & t)
	{
		for (int i = 0; i < 3; i++)
			add(t.vertex[i]);
	}
	
	void scale(double scalar, const Vector& center)
	{
		vmin = center + (vmin - center) * scalar;
		vmax = center + (vmax - center) * scalar;
	}
	
	void translate(const Vector & trans)
	{
		vmin += trans;
		vmax += trans;
	}
	
	inline bool inside(const Vector & v) const
	{
		return
				v.v[0] >= vmin.v[0] && v.v[0] <= vmax.v[0] &&
				v.v[1] >= vmin.v[1] && v.v[1] <= vmax.v[1] &&
				v.v[2] >= vmin.v[2] && v.v[2] <= vmax.v[2] ;
	}

	// does a ray, starting at `start' and heading to `dir' intersect the BBox?
	inline bool testintersect(const Vector & start, const Vector & dir) const
	{
		if (inside(start)) {
			return true;
		}
		for (int i = 0; i < 3; i++) {
			if (fabs(dir.v[i]) < 1e-12) continue;
			double rcpdir = 1.0 / dir.v[i];
			double md = fmax((vmax.v[i] - start.v[i]) * rcpdir, (vmin.v[i] - start.v[i]) * rcpdir);
			if (md < 0) continue;
			bool ok = true;
			for (int j = 0; j < 3; j++) if (i != j) {
				double c = start.v[j] + dir.v[j] * md;
				if (c < vmin.v[j] || c > vmax.v[j]) {
					ok = false;
					break;
				}
			}
			if (ok) {
				return true;
			}
		}
		return false;
	}

	inline double intersection_dist(const Vector & start, const Vector & dir)
	{
		if (inside(start)) {
			return 0.0;
		}
		double mindist = 1e99;
		for (int i = 0; i < 3; i++) {
			if (fabs(dir.v[i]) < 1e-12) continue;
			double rcpdir = 1.0 / dir.v[i];
			double md = fmax((vmax.v[i] - start.v[i]) * rcpdir, (vmin.v[i] - start.v[i]) * rcpdir);
			if (md < 0) continue;
			bool ok = true;
			for (int j = 0; j < 3; j++) if (i != j) {
				double c = start.v[j] + dir.v[j] * md;
				if (c < vmin.v[j] || c > vmax.v[j]) {
					ok = false;
					break;
				}
			}
			if (ok) {
				if (md < mindist) mindist = md;
			}
		}
		return mindist;
	}
	
	/// Does a segment intersect the BBox?
	/// @param a - one endpoint of the segment
	/// @param b - second endpoint of the segment
	bool segment_intersect(const Vector &a, const Vector &b)
	{
		return testintersect(b, a - b) && testintersect(a, b - a);
	}
	
	bool triangle_intersect(const Triangle& t)
	{
		return (
				inside(t.vertex[0]) ||
				inside(t.vertex[1]) ||
				inside(t.vertex[2]) ||
				segment_intersect(t.vertex[0], t.vertex[1]) ||
				segment_intersect(t.vertex[0], t.vertex[2]) ||
				segment_intersect(t.vertex[1], t.vertex[2]));
	}
};


#endif // __BBOX_H__
