/***************************************************************************
 *   Copyright (C) 2006 by Veselin Georgiev                                *
 *   vesko@ViruS                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *-------------------------------------------------------------------------*
 * Implements Mersenne Twister's random number generation                  *
 ***************************************************************************/

#include <stdlib.h>
#include "random.h"

#ifndef _WIN32
double drandom(void)
{
	return drand48();
}
#else
const int emod = 31337;
const double r_emod_sq = 1.0183220899390033112871047534665e-9;

double drandom(void)
{
	double t = rand() % emod;
	t = t * emod + rand() % emod;
	return t * r_emod_sq;
}
#endif

QMCSampleGen::QMCSampleGen()
{
	xx = yy = NULL;
	l = 0;
}

QMCSampleGen::~QMCSampleGen()
{
	if (xx) delete [] xx;
	if (yy) delete [] yy;
	xx = yy = NULL;
	l = 0;
}

static int gcd(int a, int b)
{
	while (a && b) {
		b %= a;
		if (b) a %= b;
	}
	return a + b;
}

void QMCSampleGen::init(int num_samples)
{
	if (num_samples <= l) return;
	if (xx) delete [] xx;
	if (yy) delete [] yy;
	l = num_samples;
	xx = new double [l];
	yy = new double [l];
	for (int dim = 0; dim < 2; dim++) {
		int z = dim + 2;
		int x = 1, y = z;
		int i = 0;
		double *r = dim ? yy : xx;
		while (i < l) {
			r[i++] = (double) x / y;
			x++;
			while (x < y && gcd(x, y) != 1) x++;
			if (x >= y) {
				y *= z;
				x = 1;
			}
		}
	}
}

double QMCSampleGen::get(int dim, int index)
{
	return dim ? yy[index] : xx[index];
}

QMCSampleGen qmc;
