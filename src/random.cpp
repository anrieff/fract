/***************************************************************************
 *   Copyright (C) 2006 by Veselin Georgiev                                *
 *   vesko@ViruS                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include "random.h"

#define BASE0 5
#define BASE1 7

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

static double radical_inverse(int base, int x)
{
	int t[20], c = 0;
	while (x) {
		t[c++] = x % base;
		x /= base;
	}
	double r = 0, z = 1.0 / base;
	for (int i = 0; i < c; i++) {
		r += z * t[i];
		z /= base;
	}
	return r;
}

QMCSampleGen::QMCSampleGen()
{
	setup();
}

void QMCSampleGen::setup(void)
{
	xx = yy = NULL;
	l = 0;
	//
	l = 10000;
	xx = new double[l];
	yy = new double[l];
	for (int i = 0; i < l; i++)
	{
		xx[i] = radical_inverse(BASE0, i+1);
		yy[i] = radical_inverse(BASE1, i+1);
	}
}

QMCSampleGen::~QMCSampleGen()
{
	if (xx) delete [] xx;
	if (yy) delete [] yy;
	xx = yy = NULL;
	l = 0;
}

QMCIterator QMCSampleGen::init(int num_samples)
{
	QMCIterator it;
	it.xidx = it.yidx = rand() % (qmc.l);
	it.gen = &qmc;
	return it;
}

double QMCIterator::next(int dim)
{
	int &i = dim ? yidx : xidx;
	if (i >= gen->l) {
		i = 0;
	}
	if (dim == 1) {
		return gen->yy[i++];
	} else {
		return gen->xx[i++];
	}
}

QMCSampleGen qmc;
