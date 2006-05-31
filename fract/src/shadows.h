/***************************************************************************
 *   Copyright (C) 2004 by Veselin Georgiev                                *
 *   vesko@ViruS                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *-------------------------------------------------------------------------*
 * The shadow casting algorithm used in fract.                             *
 * It works by projecting polygons from the light source and for each      *
 * pixel of the screen it checks whether the point lies inside some        *
 * polygon and also how far from the edge of the polygon it lies           *
 ***************************************************************************/

#include "MyTypes.h"
#include "vector3.h"
#include "object.h"
// the light is spherical, here is its radius
extern double light_radius;

// contains a dynamically allocated list of occluders - the elements of
// this list are indices in the sp[] and mesh[] arrays
extern ShadowCaster* occluders[];

const int SPHERE_SIDES = 16;
const int MAX_SIDES = 4096;
const int shadow_intensity = 0xB2;

#define VALIDITY_THRESH 10.0
#define ST_FIXED 1
#define ST_MONTE_CARLO 2

// this controls how shadow sampling will be done in the raytracing engine:
// * ST_FIXED - use constant number of fixed points around the light source
// * ST_MONTE_CARLO 
//    - use a constant number of random points around the light source
#define SHADOW_TYPE ST_FIXED

#if SHADOW_TYPE == ST_MONTE_CARLO
#define SHADOW_SAMPLES 16
#endif

/**
 * render_shadows - cast shadows from (lx, ly, lz) to the floor and ceil planes
 * @param target_framebuffer - 
 *           the color buffer that will be updated with the shadows
 * @param sbuffer - shadow buffer (will hold intensity data)
 * @param xr - X resolution
 * @param yr - Y resolution
 * @param mtt - Upper-Left end of the vector projection grid
 * @param mti - column increase
 * @param mtti - row increase
 */ 
void render_shadows(Uint32 *target_framebuffer, Uint16 *sbuffer, int xr, int yr, Vector& mtt, Vector& mti, Vector& mtti);

/**
 * Checks every sphere against every sphere or mesh object - if the sphere
 * is entirely in shadow, the corresponding SHADOWED flag is set
 */ 
void check_for_shadowed_spheres(void);

void shadows_close(void);

