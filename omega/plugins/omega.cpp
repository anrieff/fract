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
public:
	Omega() 
	{
		max_iters = 100;
	}
	~Omega() {}
	PluginInfo get_info() const
	{
		PluginInfo info;
		strcpy(info.name, "Omega");
		strcpy(info.description,
			"The set, generated with W_n = e^(-W_{n-1})");
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
	int num_iters(double x, double y)
	{
		DC z(x, y);
		DC lz = z;
		int iters = 0;
		while (real(z)*real(z) + imag(z)*imag(z) < 22 && iters < max_iters) {
			z = exp(-z);
			DC diff = lz-z;
			if (real(diff) * real(diff) + imag(diff) * imag(diff) < 1e-16) break;
			lz = z;
			++iters;
		}
		if (iters >= max_iters) return INF;
		return (int) (iters * 12.5);
	}
};

EXPORT_CLASS(Omega)
