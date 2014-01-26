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

struct FractalLightning {
	float fy;

	// create a new fractal params, based on interpolation between a and b:
	// 
	// t must be in [0..1]. 0 is pure a, 1 is pure b
	void interpolate(const FractalLightning& a, const FractalLightning& b, double t);
	
	// create a new fractal params using random
	void new_random(double time, int num = 0);
	
	// evaluate the fractal
	double eval(double x) const;
	double eval(int depth, double mult, double y0, double y1, double x) const;
};

#define BANDS 20
struct TrigLightning {
	double amps[BANDS];
	double offsets[BANDS];
	
	void interpolate(const TrigLightning&a, const TrigLightning& b, double t);
	
	void new_random(double time, int num = 0);
	
	float eval(float x) const;
};

struct Thorus: public Sphere {

	double get_depth(const Vector &camera); // this is actually used to precalc stuff
	Uint32 shade(Vector& ray, const Vector& camera, const Vector& light, double rlsrcp,
		     float &opacity, void *IntersectContext, int iteration, FilteringInfo& finfo);
	
	void init(void);
	
	// public usable data:
	int rods;	// number of "rods"
	double phi;	// angular offset of the first "rod" (meaningful value: [0 - 2PI/rods) )
	double warpspeed; // speed of lightning `winding'
private:
	// useless data:
	Vector top;
	Vector verts[4];
	Vector rod_normal[4];
	Vector tux[4];
	Vector xrod[4];
	double rod_dist[4];
	double len;
	int last_time;
	FractalLightning prev[4], next[4], cur[4];
};

void thor_init(void);

extern Thorus thor[];
extern int thors_count;

