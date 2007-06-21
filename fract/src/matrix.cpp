/***************************************************************************
 *   Copyright (C) 2006 by Veselin Georgiev                                *
 *   anrieff@mailg.com (convert to gmail)                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <math.h>
#include "matrix.h"

Matrix rotation(int axis, double angle)
{
	double sina = sin(angle);
	double cosa = cos(angle);
	Matrix m(1);
	switch (axis) {
		case AXIS_X:
			m.m[1][1] = cosa;
			m.m[1][2] = sina;
			m.m[2][1] = -sina;
			m.m[2][2] = cosa;
			break;
		case AXIS_Y:
			m.m[0][0] = cosa;
			m.m[0][2] = -sina;
			m.m[2][0] = sina;
			m.m[2][2] = cosa;
			break;
		case AXIS_Z:
			m.m[0][0] = cosa;
			m.m[0][1] = sina;
			m.m[1][0] = -sina;
			m.m[1][1] = cosa;
			break;
		default:
			/* nothing */
			break;
	}
	return m;
}

