/***************************************************************************
 *   Copyright (C) 2006 by Veselin Georgiev                                *
 *   anrieff@mailg.com (convert to gmail)                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef __MATRIX_H__
#define __MATRIX_H__

#include <string.h>
#include "vector3.h"

enum {
	AXIS_X,
	AXIS_Y,
	AXIS_Z
};

struct Matrix {
	double m[3][3];

	Matrix() {}
	// init the matrix with `x' on the main diagonal, zero on the others
	Matrix(double x) {
		memset(m, 0, sizeof(m));
		m[2][2] = m[1][1] = m[0][0] = x;
	}
	
	Matrix(double m00, double m01, double m02, double m10, double m11, double m12,
	       double m20, double m21, double m22) {
		m[0][0] = m00;
		m[0][1] = m01;
		m[0][2] = m02;
		m[1][0] = m10;
		m[1][1] = m11;
		m[1][2] = m12;
		m[2][0] = m20;
		m[2][1] = m21;
		m[2][2] = m22;
	}
	
	Vector operator * (const Vector &v) const
	{
		return Vector(
			m[0][0] * v.v[0] + m[0][1] * v.v[1] + m[0][2] * v.v[2],
			m[1][0] * v.v[0] + m[1][1] * v.v[1] + m[1][2] * v.v[2],
			m[2][0] * v.v[0] + m[2][1] * v.v[1] + m[2][2] * v.v[2]);
	}
	
	Matrix operator * (const Matrix &m) const {
		Matrix c(0);
		for (int i = 0; i < 3; i++)
			for (int j = 0; j < 3; j++)
				for (int k = 0; k < 3; k++)
					c.m[i][j] += this->m[i][k] * m.m[k][j];
		return c; 
	}
};

Matrix rotation(int axis, double angle);

#endif
