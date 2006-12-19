/***************************************************************************
 *   Copyright (C) 2006 by Veselin Georgiev                                *
 *   anrieff@mgail.com (convert to gmail)                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
***************************************************************************/
#include <string.h>
#include <cmath>
#include "pluginmanager.h"
#include <complex>

using namespace std;

typedef complex<double> DC;


static inline Rgb mul(Rgb c, float m)
{
	int b = c & 0xff; c >>= 8; b = (int) (b * m);
	int g = c & 0xff; c >>= 8; g = (int) (g * m);
	int                        r = (int) (c * m);
	return RGB(r,g,b);
}

class Newton : public Plugin {
	int max_iters;
	Rgb root_shades[5];
public:
	Newton() 
	{
		max_iters = 100;
		root_shades[0] = RGB(245, 140, 120);
		root_shades[1] = RGB(55, 240, 76);
		root_shades[2] = RGB(200, 15, 220);
		root_shades[3] = RGB(180, 200, 35);
		root_shades[4] = RGB(160, 160, 160);
	}
	~Newton() {}
	PluginInfo get_info() const
	{
		PluginInfo info;
		strcpy(info.name, "Newton");
		strcpy(info.description,
			"The set of convergence for solving (x^5 - 1) in C");
		info.priority = 0;
		return info;
	}
	View get_default_view() const
	{
		View v = { 0, 0, 3 };
		return v;
	}
	int get_max_iters() const { return max_iters; }
	void set_max_iters(int x) { max_iters = x; }
	Rgb shade(double x, double y)
	{
		DC z(x, y);
		DC zlast = z;
		int iters = 0;
		while (iters < max_iters) {
			DC z4 = z*z;
			z4 *= z4;
			z = z - (z*z4-DC(1,0))/(DC(5.0,0)*z4);
			DC diff = z - zlast;
			if (real(diff) * real(diff) + imag(diff) * imag(diff) < 1e-12)
				break;
			zlast = z;
			++iters;
		}
		if (iters >= max_iters) return 0;
		x = real(z), y = imag(z);
		Rgb color;
		if (fabs(y) < 0.1) {
			color = root_shades[4];
		} else {
			color = root_shades[(x < 0) * 2 + (y < 0)];
		}
		float m = 1.0f - (float) iters / max_iters;
		
		return mul(color, m*m);
	}
	bool iterate(IterationPoint *p)
	{
		DC z(p->x, p->y);
		DC z4 = z*z;
		z4 *= z4;
		z = z - (z*z4-DC(1,0))/(DC(5.0,0)*z4);
		p->x = real(z), p->y = imag(z);
		return true;
	}
};

EXPORT_CLASS(Newton)
