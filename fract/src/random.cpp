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

#ifdef __GNUC__
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
