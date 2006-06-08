/***************************************************************************
 *   Copyright (C) 2004 by Veselin Georgiev                                *
 *   vesko@ViruS                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   --------------------------------------------------------------------  *
 *       Abstract class for raytraceable objects. Any primitive            *
 *       (sphere, triangle, lightsource, etc) should inherit it            *
 *                                                                         *
 ***************************************************************************/

#ifndef __OBJECT_H__
#define __OBJECT_H__

#include "MyGlobal.h"
#include "vector3.h"

struct FilteringInfo;

#define MAX_OBJECTS (MAX_TRIANGLES + MAX_SPHERES)

enum OBTYPE {
	OB_ALL,
	OB_SPHERE = 0x100,
	OB_TRIANGLE = 0x200,
	OB_UNDEFINED = 0xffffffff
};

/**
 * @class Object
 * @brief abstract 3d primitive.
 *
 * "object" defines which methods need to be implemented by the derivate
 *          classes in order to be renderable
 *
 * @date 2005-12-11
 * @author Veselin Georgiev
 */
struct Object {
public:

	/// this data is used if you want to mark some of the objects
	/// so you can easily recognize them later
	unsigned tag, id;

	/// Get the distance from the camera (used later in Z-sorting)
	virtual double get_depth(const Vector &camera) = 0;

	/// If "w" are the three endpoints of the vector projection grid,
	/// determine whether the object is visible on the screen
	virtual bool is_visible(const Vector & camera, Vector w[3]) = 0;

	/// Create a convex polygon of 3D points in the `pt' array.
	/// The polygon must be created in such a way, so that if the
	/// polygon is rendered in place of the real object, they should
	/// allocate similar position and sizes on the 2D screen.
	///
	/// @returns how many points the resulting polygon has; must be
	/// &lt; MAX_SPHERE_SIDES
	virtual int calculate_convex(Vector pt[], Vector camera) = 0;

	/** Rasterize an object onto the screen. Use the polygon provided
	*** by calculate_convex().
	***
	*** @param framebuffer - a virtual framebuffer, possibly larger than
	***                     the physically visible; obtain dimensions with
	***                     xsize_render(xres()) and ysize_render(yres())
	*** @param color       - unique 16bit nonzero ID for the object; the "color"
	***                      to "paint" on the framebuffer with
	*** @param sides       - number of sides on the convex polygon
	*** @param pt          - the polygon itself
	*** @param camera      - teh camera
	*** @param w           - the three endpoints of the vector projection grid
	*** @param min_y       - the min. row on the screen, which contains the obj
	*** @param max_y       - the max. row on the screen, which contains the obj
	**/
	virtual void map2screen(
				Uint32 *framebuffer,
				int color,
				int sides,
				Vector pt[],
				Vector camera,
				Vector w[3],
				int & min_y,
				int & max_y) = 0;

	/** Performs "fast " intersection test:
	*** @param ray - Camera look direction (not necessarily unitary)
	*** @param camera - the position of the camera
	*** @param rls = ray.lengthSqr()
	*** @param IntersectContext -
	***        put any data, which may be used in further shading & tracing
	***        here (this is to avoid redundant calculations). You can
	***        safely use at most 128 bytes of the provided memory.
	*** @return whether intersection occured
	**/
	virtual bool fastintersect(
				const Vector& ray,
				const Vector& camera,
				const double& rls,
				void *IntersectContext) const = 0;

	/// The same as fastintersect, except it doesn't use `rls' and assumes
	/// that the `ray' vector is unitary
	virtual bool intersect(
				const Vector& ray,
				const Vector &camera,
				void *IntersectContext) = 0;

	/// Returns the distance to the intersection, using the stored calculations
	/// from Intersect or FastIntersect
	virtual double intersection_dist(void *IntersectContext) const = 0;

	/**
	*** Performs light, texturemapping, shading and reflections/refractions
	*** calculations
	***
	*** @param ray          - camera look direction (not necessarily unitary)
	*** @param camera       - the camera itself
	*** @param light        - light source
	*** @param rlsrcp       = 1 / ray.lengthSqr()
	*** @param[out] opacity - if the object is not fully opaque, this will
	***                       be in the range (0.0; 1.0)
	*** @param IntersectContext - the intersection context as previously
	***                           prepared by Intersect() or FastIntersect()
	*** @param iteration - the number of reflections/refractions counter,
	***                    used to prevent infinite raytracing loop
	*** @param finfo     - This is used to perform mipmapping on the
	***                    reflections/refractions
	**/
	virtual Uint32 shade(
				Vector& ray,
				const Vector& camera,
				const Vector& light,
				double rlsrcp,
				float *opacity,
				void *IntersectContext,
				int iteration,
				FilteringInfo& finfo
				) = 0;
	/**
	*** Called when a secondary reflected or refracted ray has hit the floor/ceiling
	*** and Raytrace() wants to know which mipmap level of the floor texture to use.
	***
	*** Return a number of the range [0..LOG2_MAX_TEXTURE_SIZE], where 0 is the most
	*** detailed level.
	***
	*** Parameters:
	*** @param x0 - the X coordinate where the secondary ray hit the floor
	*** @param z0 - the Z coordinate where the secondary ray hit the floor
	*** @param fi.camera  - (vector) the camera
	*** @param fi.xinc    - (vector) the "x" direction of the vector grid
	*** @param fi.yinc    - (vector) the "y" direction of the vector grid
	*** @param fi.through - (vector) the point from the vector grid for the originating ray
	*** @param fi.lectionType is REFLECTION or REFRACTION
	***
	*** If you want to handle the miplevel issue, trace two or more additional rays through
	*** the "neighborhood" of `through' and see where they hit. Use the area of the formed
	*** triangle to determine the miplevel
	***
	*** If you can't or don't want to handle it, return TEX_S or, indeed, any other level.
	**/
	virtual int get_best_miplevel(double x0, double z0, FilteringInfo & fi) = 0;

	/**
	*** Returns the type of the primitive, the id is like OB_SPHERE,
	*** OB_TRIANGLE
	**/
	virtual OBTYPE get_type(bool generic = true) const = 0;
};

/**
 * @class ShadowCaster
 * @brief Defines an interface for object that may cast shadow on other objects
 * 
 * This class has a single method "SIntersect" that tests if a ray intersects
 * the object
 * 
 * @date 2006-05-03
 * @author Veselin Georgiev
*/ 
class ShadowCaster {
public:
	/**
	 * @param start - the start of the ray
	 * @param dir   - the direction of the ray (this vector must be unitary)
	 * @param opt   - optional optimization parameter
	*/ 
	virtual bool sintersect(const Vector & start, const Vector & dir, int opt = 0) = 0;
};

struct CompareStruct {
	double depth;
	Object* obj;
};


class ObjectArray {
	int count;
	Vector cam;
	CompareStruct *t;
public:
	CompareStruct a[MAX_OBJECTS];
	ObjectArray ()
	{
		t = new CompareStruct[MAX_OBJECTS];
	}

	~ObjectArray()
	{
		delete [] t;
	}

	int size() const { return count; }
	void clear(const Vector & camera)
	{
		count = 0;
		cam = camera;
	}
	void operator += (Object *o)
	{
		a[count].obj = o;
		a[count].depth = o->get_depth(cam);
		++count;
	}

	Object * operator [] (const int idx) const
	{
		return a[idx].obj;
	}

	/******************************************************
	 * Sort the objects by depthance, in descending order *
	 * using merge sort, which has a guaranteed n*log n   *
	 * runtime.                                           *
	 *                                                    *
	 * Anything simpler could have been used, as it would *
	 * not lead to any performance impact, but will do    *
	 * something more noticeable at larger object counts  *
	 ******************************************************/

	void sort(int l, int r)
	{
		int i, j, m, tp=0;
		/* stage 1 */
		if (l==r) return;
		if (r-l==1) {
			if (a[l].depth < a[r].depth) {
				// swap the objects a[l] & a[r]
				t[0] = a[l];
				a[l] = a[r];
				a[r] = t[0];
			}
			return;
		}
		/* stage 2 */
		m = (l+r)/2;
		sort(l, m); sort(m+1, r);
		/* stage 3 */
		i = l; j = m+1;
		while (i<=m || j<=r) {
			if (i>m) t[tp++]=a[j++];
				else if (j>r) t[tp++]=a[i++];
					else if (a[i].depth > a[j].depth)
						t[tp++]=a[i++];
						else t[tp++]=a[j++];
		}
		for (i=l;i<=r;i++)
			a[i] = t[i-l];
	}

	void sort(void)
	{
		if (count > 1)
			sort(0, count - 1);
	}
};


#endif // __OBJECT_H__
