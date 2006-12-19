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
	Rgb coltable[1536];
	int coltable_size;
public:
	Mandelbrot()
	{
		max_iters = 100;
		coltable_size = 1536;
		for (int i = 0; i < 256; i++) coltable[       i] = RGB(  255,     i,     0);
		for (int i = 0; i < 256; i++) coltable[ 256 + i] = RGB(255-i,   255,     0);
		for (int i = 0; i < 256; i++) coltable[ 512 + i] = RGB(    0,   255,     i);
		for (int i = 0; i < 256; i++) coltable[ 768 + i] = RGB(    0, 255-i,   255);
		for (int i = 0; i < 256; i++) coltable[1024 + i] = RGB(    i,     0,   255);
		for (int i = 0; i < 256; i++) coltable[1280 + i] = RGB(  255,     0, 255-i);
	}
	~Mandelbrot() {	}
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
	Rgb shade(double x, double y)
	{
		DC c(x, y), z(0, 0);
		int iters = 0;
		while (real(z)*real(z) + imag(z)*imag(z) < 4 && iters < max_iters) {
			z = z*z + c;
			++iters;
		}
		if (iters >= max_iters) return 0;
		iters *= 12;
		return coltable[iters % coltable_size];
	}
	void init_point(IterationPoint *p)
	{
		double *d = reinterpret_cast<double*>(p->auxdata);
		d[0] = p->x;
		d[1] = p->y;
	}
	
	bool iterate(IterationPoint *p)
	{
		double *d = reinterpret_cast<double*>(p->auxdata);
		DC c(d[0], d[1]);
		DC z(p->x, p->y);
		z = z*z + c;
		p->x = real(z), p->y = imag(z);
		return (real(z) * real(z) + imag(z) * imag(z) < 4);
	}
};

EXPORT_CLASS(Mandelbrot)
