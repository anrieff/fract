/***************************************************************************
 *   Copyright (C) 2006 by Veselin Georgiev                                *
 *   anrieff@mailg.com (convert to gmail)                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *-------------------------------------------------------------------------*
 *                                                                         *
 *   tracer.cpp - defines the "Tracer" object (dashed circle in 3D)        *
 *                                                                         *
 ***************************************************************************/

#include "tracer.h"
#include "vectormath.h"
#include "random.h"


Tracer tracer[8];
int tracers_count;
bool g_view_tracers = false;

/** 
 * @class Tracer
 */ 

void Tracer::init(void)
{
	Vector a = points[1] - points[0];
	Vector b = points[2] - points[0];
	Vector vp = a ^ b;
	pnormal = vp;
	pnormal.norm();
	pdist = - (points[0] * pnormal);
	Vector u1 = a ^ vp;
	Vector v1 = b ^ vp;
	Vector u = u1, v = v1;

	Vector center1 = (points[0] + points[1]) * 0.5;
	Vector center2 = (points[0] + points[2]) * 0.5;


	//
	// Solve: center1 + p * u = center2 + q * v
	//
	center2 -= center1;
	v.scale(-1);

	center2 = Vector(center2[0], center2[1] + center2[1], 0);
	u = Vector(u[0], u[1] + u[1], 0);
	v = Vector(v[0], v[1] + v[1], 0);

	double det = u[0] * v[1] - u[1] * v[0];
	if (fabs(det) < 1e-9) return;
	double p = (center2[0] * v[1] - center2[1] * v[0]) / det; 

	pos = (points[0] + points[1])* 0.5 + u1 * p;
	d = pos.distto(points[0]);
	if (thickness == 0) thickness = 1.0f;
	if (phasespeed == 0) phasespeed = 6.0f;
}

double Tracer::get_depth(const Vector &camera)
{
	sinphase = bTime() * phasespeed;
	mycolor = (this->r << 16 ) + (this->g << 8) + this->b;
	return camera.distto(pos);
}

Uint32 Tracer::shade(Vector &ray, const Vector &camera, const Vector& light, double rlsrcp, float &opacity, void*, int, FilteringInfo&)
{
	opacity = 0;
	double ev = pnormal * camera + pdist;
	double dv = pnormal * ray;
	// ev + p * dv = 0
	double p = -ev / dv;
	if (p < 0) return mycolor;
	Vector v;
	v.macc(camera, ray, p);
	double r = v.distto(pos) - d;

	if (fabs(r) < thickness)
	{
		opacity = (thickness - fabs(r)) / thickness;
		double mul = atan2(v[0] - pos[0], v[1] + v[2] - pos[1] - pos[2]);
		double sn = sin(6 * mul + sinphase);
		if (sn < 0) sn = -sqrt(-sn); else sn = sqrt(sn);
		opacity *= opacity * (1.0 + sn) * 0.5;;
	}
	return mycolor;
}
