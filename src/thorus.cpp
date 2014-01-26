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
 *   thorus.cpp - defines the "Thorus" object (a fictionary ball, that     *
 *   contains quite a bit static electricity :) )                          *
 *                                                                         *
 ***************************************************************************/

#include "thorus.h"
#include "vectormath.h"
#include "random.h"

Thorus thor[2];
int thors_count;

const double topy = -0.9;
const double boty = +0.66;

static float sinprec[1024];
static float precmul;


void thor_init(void)
{
	for (int i = 0; i < 1024; i++)
		sinprec[i] = sinf(i / 1024.0 * 2.0f * M_PI);
	precmul = 1024 / (2.0f * M_PI);
}

/** 
 * @class Thorus
 */ 

void Thorus::init(void)
{
	last_time = -1;
	if (rods < 0 || rods > 5) return;
	for (int i = 0; i < rods; i++) {
		next[i].new_random(0);
	}
	if (warpspeed <= 0.0) warpspeed = 1.0;
}

double Thorus::get_depth(const Vector & camera)
{
	top = pos; top[1] += d * topy;
	double xzmult = sqrt(1 - boty * boty) * d;
	for (int i = 0; i < rods; i++) {
		verts[i] = Vector(
				pos[0] + cos(i * 2.0 * M_PI / rods + phi) * xzmult,
				pos[1] + d * boty,
				pos[2] + sin(i * 2.0 * M_PI / rods + phi) * xzmult
				 );
		Vector t = verts[i] - top;
		Vector u = verts[i] - camera;
		tux[i] = u ^ t;
		rod_normal[i] = t ^ tux[i];
		rod_normal[i].norm();
		tux[i].norm();
		rod_dist[i] = rod_normal[i] * top;
		xrod[i] = t; xrod[i].norm();
	}
	len = top.distto(verts[0]);
	
	int xtime = (int) (bTime() * warpspeed);
	double ftime = bTime()*warpspeed - xtime;
	if (xtime != last_time) {
		for (int i = 0; i < rods; i++) {
			prev[i] = next[i];
			next[i].new_random(bTime()*warpspeed, i);
		}
	}
	last_time = xtime;
	for (int i = 0; i < rods; i++)
		cur[i].interpolate(prev[i], next[i], ftime);
		
	// do stupid things :) in fact, what we are required to do :)
	return pos.distto(camera);
}

Uint32 Thorus::shade(Vector& v, const Vector& c, const Vector& light, double rlsrcp,
	     float &opacity, void *IntersectContext, int iteration, FilteringInfo& finfo)
{
	opacity = 0.0f;
	Uint32 sr = 0, sg = 0, sb = 0;
	
	for (int i = 0; i < rods; i++) {
		float X = c * rod_normal[i] - rod_dist[i];
		float dX = v * rod_normal[i];
		float root = -X/dX;
		if (root < 0) continue;
		Vector t;
		t.macc(c, v, root);
		t -= top;
		float proj_x = t * xrod[i];
		if (proj_x < 0 || proj_x > len) continue;
		float proj_y = t * tux[i];
		
		if (fabs(proj_y) > 3.5f) continue;
		
		float fun_y = cur[i].eval(proj_x / len);
		
		float delta = sqr(fun_y - proj_y);
		
		if (delta < 1e-6) delta = 1e-6;
		float power = 0.51/delta;
		opacity += power;
		sr += (int) (r * power);
		sg += (int) (g * power);
		sb += (int) (b * power);
		
	}
	
	if (opacity > 1.0f) opacity = 1.0f;
	if (sr > 255) sr = 255;
	if (sg > 255) sg = 255;
	if (sb > 255) sb = 255;
	return (sr << 16) | (sg << 8) | sb;
}


/** 
 * @class FractalLightning
 */ 

void FractalLightning::interpolate(const FractalLightning& a, const FractalLightning& b, double t)
{
	fy = hermite(a.fy, b.fy, t);
}

void FractalLightning::new_random(double time, int num)
{
	fy = time + 20 * num;
}

double FractalLightning::eval(double x) const
{
	return 2.4*perlin(x*6.0f, fy) * cos((x-0.5)*M_PI);
}

/** 
 * @class TrigLightning
 */ 
void TrigLightning::interpolate(const TrigLightning& a, const TrigLightning& b, double t)
{
	for (int i = 0; i < BANDS; i++)
	{
		amps[i] = hermite(a.amps[i], b.amps[i], t);
		offsets[i] = hermite(a.offsets[i], b.offsets[i], t);
	}
}

void TrigLightning::new_random(double time, int num)
{
	for (int i = 0; i < BANDS; i++) {
		amps[i] = 2.0 * drandom() - 1.0;
		offsets[i] = 2 * M_PI * drandom();
	}
}


float TrigLightning::eval(float x) const
{
	float y = 0;
	for (int i = 0; i < BANDS; i++) {
		y += amps[i] * sinprec[((int) (((10 + 3 * i) * x + offsets[i])*precmul)) % 1024];
	}
	return y * cosf((x-0.5f)*3.1415f);
}
