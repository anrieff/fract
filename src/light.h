/***************************************************************************
 *   Copyright (C) 2006 by Veselin Georgiev                                *
 *   anrieff@mailg.com (convert to gmail)                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef __LIGHT_H__
#define __LIGHT_H__

#include "vector3.h"

/**
 * @File	light.h
 * @Author	Veselin Georgiev
 * @Date	2006-10-19
 * @Brief	Describes fract's spherical light and shadowing algorithms
 */ 


/// Shadowing modes, the default being RASTER
enum ShadowMode {

	/**
	 * The RASTER mode is a complex, specialized method, that works for the 
	 * infiniteplane background only. The rastering method is very fast
	 * for spheres and low-poly (< 1000-2000 triangles) meshes.
	 * However, this method is `fragile', esp. on complex meshes
	 *
	 * DYNAMIC LIGHTS: yes
	 * MULTI-THREADED: yes (partially)
	 * ARTIFACTS: none
	 * SPEED: very fast for small meshes (O(N^2), where N is mesh poly count)
	 * OBJECTS RENDERED CORRECTLY: 2-manifolds only.
	 */
	RASTER,
	
	/**
	 * The RAYTRACE mode is general-purpose method, based on true 
	 * raytracing.
	 *
	 * DYNAMIC LIGHTS: yes
	 * MULTI-THREADED: yes
	 * ARTIFACTS: banding artifacts
	 * SPEED: very slow (O(N^2 * X * Y) -- N - poly count, X, Y - resolution)
	 * OBJECTS RENDERED CORRECTLY: any
	 */
	RAYTRACE,
	
	/**
	 * The LIGHTMAP mode is based on (costly) precalculation phase,
	 * which allows for fast in_shadow() lookups thereafter.
	 * 
	 * DYNAMIC LIGHTS: no
	 * MULTI-THREADED: yes (generation isn't, though)
	 * ARTIFACTS: banding artifacts, stepping artifacts
	 * SPEED: fast, doesn't depend on mesh sizes -- O(X*Y)
	 * OBJECTS RENDERED CORRECTLY: any
	 */
	LIGHTMAP,
};


/**
 * Fract's light is spherical, but for the RAYTRACE and LIGHTMAP methods it
 * needs to be approximated by a fixed count of point lights.
 * 
 * A pointlight needs to only know to determine, whether some point in the scene
 * is in shadow for that particular pointlight - thus returning just true/false
 */ 
class PointLight {
	float* build_lightmap_side(const Vector & dv);
public:
	Vector p; // the actual light point
	float *maps[6]; // cube map of depths
	// order is +X, -X, +Y, -Y, +Z, -Z
	int size; // map size
	
	PointLight();
	~PointLight();
	
	/**
	 * @brief checks if a point is in shadow
	 * @param p - the point to check for shadowing
	 * @returns \
	 *   0 = no shadow
	 *   1 = shadowed
	 *  -1 = entire side of the cubemap is not rendered. This means, the
	 *       point is not shadowed, and moreover, the intent is that no points
	 *       nearby could be shadowed. This is intended to specify optimization
	 *       hints. If you don't need to optimize, just equate -1 with 0.
	 */
	int in_shadow(const Vector& p);
	void rebuild_lightmap(void);
	
	/**
	 * Returns the distance to the closest intersected sphere or mesh.
	 * @param origin the ray start
	 * @param dir    ray direction (normalized)
	 * @returns the distance to the closest intersecting object
	 *          0, if nothing gets intersected.
	 */
	float closest_intersection(const Vector & origin, const Vector & dir);
};

/**
 * The Light class represents fully the spherical light. The shadow_density()
 * uses calls to PointLight::in_shadow() to determine amount of shadow for
 * the given point
 */ 
class Light {
public:
	Vector p;
	float radius;
	ShadowMode mode;
	PointLight points[16];
	
	Light();
	
	/// call this when you change p - this will reposition points[]
	/// and rebuild lightmaps if necessary.
	void reposition(void);
	
	/// sets the light sphere's radius, rebuilding the lightmaps if necessary.
	void setradius(float new_radius);
	
	/**
	 * @brief determines the amount of shadow for a particular point
	 * @param point the point to check for shadow
	 * @returns amount of shadow (0 = no shadow, 1 = full shadow)
	 */
	float shadow_density(const Vector & point);
	
	/// rebuilds the lightmaps (CVars::lmsize *MUST* be set to some size)
	void rebuild_lightmaps(void);
	
	/**
	 * Must be called after each scene load, just after the scene is built
	 * and set-up.
	*/ 
	void postload_init(void);
};

extern Light light;

static inline int lx() { return (int) light.p[0]; }
static inline int lz() { return (int) light.p[2]; }

#endif // __LIGHT_H__
