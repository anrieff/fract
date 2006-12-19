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
#include <math.h>
#include "pluginmanager.h"
#include <complex>

typedef std::complex<double> DC;

class Mandelbrot : public Plugin {
	int max_iters;
public:
	Mandelbrot() 
	{
		max_iters = 100;
	}
	~Mandelbrot() {}
	PluginInfo get_info() const
	{
		PluginInfo info;
		strcpy(info.name, "Mandelbrot");
		strcpy(info.description,
			"The standard mandelbrot set");
		info.priority = 10;
		return info;
	}
	View get_default_view() const
	{
		View v = { 0, 0, 2 };
		return v;
	}
	int get_max_iters() const { return max_iters; }
	void set_max_iters(int x) { max_iters = x; }
	int num_iters(double x, double y)
	{
		DC c(x, y), z(0, 0);
		int iters = 0;
		while (real(z)*real(z) + imag(z)*imag(z) < 4 && iters < max_iters) {
			z = z*z + c;
			++iters;
		}
		if (iters >= max_iters) return INF;
		return (int) (iters * 12.5);
	}
};

EXPORT_CLASS(Mandelbrot)
