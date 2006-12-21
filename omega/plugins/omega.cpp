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

class Omega : public Plugin {
	int max_iters;
	Rgb coltable[1536];
	int coltable_size;
public:
	Omega() 
	{
		max_iters = 100;
		coltable_size = 1536;
		for (int i = 0; i < 256; i++) coltable[       i] = RGB(    0,     i,   255);
		for (int i = 0; i < 256; i++) coltable[ 256 + i] = RGB(    0,   255, 255-i);
		for (int i = 0; i < 256; i++) coltable[ 512 + i] = RGB(    i,   255,     0);
		for (int i = 0; i < 256; i++) coltable[ 768 + i] = RGB(  255, 255-i,     0);
		for (int i = 0; i < 256; i++) coltable[1024 + i] = RGB(  255,     0,     i);
		for (int i = 0; i < 256; i++) coltable[1280 + i] = RGB(255-i,     0,   255);

	}
	~Omega() {}
	PluginInfo get_info() const
	{
		PluginInfo info;
		strcpy(info.name, "Omega");
		strcpy(info.description,
			"The set, generated with W_n = e^(-W_{n-1})");
		info.priority = 5;
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
		DC lz = z;
		int iters = 0;
		while (real(z)*real(z) + imag(z)*imag(z) < 100 && iters < max_iters) {
			z = exp(-z);
			DC diff = lz-z;
			if (real(diff) * real(diff) + imag(diff) * imag(diff) < 1e-16) break;
			lz = z;
			++iters;
		}
		if (iters >= max_iters) return 0;
		iters *= 42;
		return coltable[iters % coltable_size];
	}
	bool iterate(IterationPoint *p)
	{
		DC z(p->x, p->y);
		z = exp(-z);
		p->x = real(z), p->y = imag(z);
		return (real(z)*real(z) + imag(z)*imag(z) < 22);
	}
};

EXPORT_CLASS(Omega)
