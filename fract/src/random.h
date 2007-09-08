/***************************************************************************
 *   Copyright (C) 2006 by Veselin Georgiev                                *
 *   vesko@ViruS                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef __RANDOM_H__
#define __RANDOM_H__

// returns random number in the interval [0; 1)
double drandom(void);

struct QMCPerm;
class QMCSampleGen;

// QMC iterator, needed by users of QMC numbers
class QMCIterator {
	QMCSampleGen *gen;
	int xidx, yidx;
public:
	/**
	 * Generate a new quasi-random point in the specified dimension
	 * (currently only two dimensions are supported)
	 * @param dimension - 0 or 1 - the dimension to generate point for
	 * @retval double in [0..1] - the requested quasi-random point.
	 */
	double next(int dimension);
	friend class QMCSampleGen;
};

// Quasi-monte carlo sample generator for 2D points
class QMCSampleGen {
	void setup(void);
public:
	int l;
	double *xx, *yy;
	QMCSampleGen();
	~QMCSampleGen();
	/**
	 * init()
	 * creates a new QMC iterator, which is to be used for
	 * one QMC integration. Call iterator's next() function
	 * to actually obtain the quasi-random points.
	 * @param num_samples - The number of samples you are likely going to
	 * need. If you use more than this, some sampling bias may 
	 * be introduced.
	 */
	QMCIterator init(int num_samples);
};

extern QMCSampleGen qmc;

#endif
