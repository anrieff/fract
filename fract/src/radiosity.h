/***************************************************************************
 *   Copyright (C) 2006 by Veselin Georgiev                                *
 *   anrieff@mailg.com (convert to gmail)                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef __RADIOSITY_H__
#define __RADIOSITY_H__

#include "threads.h"

extern double rad_amplification;
extern int    rad_spv;
extern double rad_stone_lightout, rad_stone_specular;
extern double rad_water_lightout, rad_water_specular;
extern int    rad_light_samples;
extern double rad_light_radius;

void radiosity_calculate(ThreadPool &);

#endif // __RADIOSITY_H__
