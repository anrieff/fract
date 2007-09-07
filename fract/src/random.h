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

// Quasi-monte carlo sample generator for 2D points
class QMCSampleGen {
	double *xx, *yy;
	int l;
public:
	QMCSampleGen();
	~QMCSampleGen();
	/**
	 * init()
	 * request the number of samples to be precalculated and stored
	 * must be called before any call to the get() method
	 * @param num_samples - the number of samples to generate
	 */
	void init(int num_samples);
	
	/**
	 * get()
	 * gets a quasi-random point.
	 * @param dim - dimension to use - 0 or 1
	 * @param index - the index of the point in the series. Must not
	 *                exceed the number, passed to ::init()
	 * @retval the quasi-random value for the given dimension
	*/
	double get(int dim, int index);
};

extern QMCSampleGen qmc;

#endif
