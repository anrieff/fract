/***************************************************************************
 *   Copyright (C) 2006 by Veselin Georgiev                                *
 *   anrieff@mailg.com (convert to gmail)                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "MyTypes.h"
#include "sphere.h"

struct Tracer : public Sphere {
	Vector points[3];
	Vector pnormal;
	double pdist;
	float sinphase, phasespeed, thickness;
	Uint32 mycolor;

	void init(void);

	// from sphere
	double get_depth(const Vector &camera); // this is actually used to precalc stuff
	Uint32 shade(Vector& ray, const Vector& camera, const Vector& light, double rlsrcp,
		     float &opacity, void *IntersectContext, int iteration, FilteringInfo& finfo);


};


extern Tracer tracer[];
extern int tracers_count;
extern bool g_view_tracers;