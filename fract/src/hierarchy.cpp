/***************************************************************************
 *   Copyright (C) 2006 by Veselin Georgiev                                *
 *   anrieff@mailg.com (convert to gmail)                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <assert.h>
#include "fract.h"
#include "vector3.h"
#include "hierarchy.h"
#include "md5.h"
#include "progress.h"
#include "common.h"
#include "voxel.h"


static float fmax2(float a, float b)
{
	return a > b ? a : b;
}

static float fmin2(float a, float b)
{
	return a < b ? a : b;
}

static Task *buildtask;
static int task_num = 0;

bool Hierarchy::build_hierarchy(int thesize, float *thebuff, bool is_floor)
{
	float (*ffun) (float,float) = is_floor ? fmax2 : fmin2;
	float *maps[12]; //support textures up to 2048x2048
	this->is_floor = is_floor;
	size = thesize;
	slog = power_of_2(size);
	s = slog - power_of_2(MIN_SIZE) + 2;
	
	if (task_num == 0) {
		buildtask = new Task(HIERGEN, __FUNCTION__);
		buildtask->set_steps(NUM_VOXELS * 4 * s);
	}
	Task &task = *buildtask;
	
	highestpeak = is_floor ? -1e9 : +1e9;

	for (int i = 0; i < size*size; i++)
		highestpeak = ffun(highestpeak, thebuff[i]);

	int allocsize = size * size * 4;
	
	for (int i = 0; i <= slog; i++) {
		maps[i] = (float* ) malloc(allocsize);
	}
	field = (float*) malloc(allocsize);
	memcpy(field, thebuff, allocsize);
	memcpy(maps[0], thebuff, allocsize);
	
	const int quaddirs[4][2] = {
		{+1, +1}, // +z, +x
		{-1, +1}, // -z, +x
		{+1, -1}, // +z, -x
		{-1, -1}  // -z, -x
	};
	
	for (int q = 0; q < 4; q++) {
		
		quadrant[q] = (float*) malloc(4 * size * size * s);
		for (int i = 0; i < size; i++) {
			for (int j = 0; j < size; j++) {
				quadrant[q][(i*size+j)*s] = thebuff[i * size + j];
			}
		}
		
		int ss = 2, klev = 0, mlev;
		for (mlev = 1; mlev <= slog; mlev++, ss *= 2) {
			if (ss >= MIN_SIZE) klev++;
			if (klev >= s) break;
			
			for (int i = 0; i < size; i++) {
				for (int j = 0; j < size; j++) {
					int t = i * size + j;
					maps[mlev][t] = maps[mlev-1][t];
					for (int ii = 0; ii < 2; ii++) {
						for (int jj = 0; jj < 2; jj++) if (ii||jj) {
							int y = i + (ii * ss/2) * quaddirs[q][0];
							int x = j + (jj * ss/2) * quaddirs[q][1];
							if (y >= 0 && y < size && x >= 0 && x < size) {
								maps[mlev][t] = ffun(maps[mlev][t],
										maps[mlev-1][y*size+x]);
							}
						}
					}
					if (ss >= MIN_SIZE) quadrant[q][t*s+klev] = maps[mlev][t];
				}
			}
			task++;
		}
		
	}
	
	for (int i = 0; i <= slog; i++) {
		free(maps[i]); maps[i] = NULL;
	}
	if (++task_num == NUM_VOXELS) {
		delete buildtask;
	}
	return true;
}

float Hierarchy::ray_intersect(const Vector & orig, const Vector & proj, Vector & crossing)
{
#define INRANGE(v) (v[0]>=0 && v[0] < size-1 && v[2] >= 0 && v[2] < size - 1)

	if (!INRANGE(orig)) return 1e9;
	
	Vector v;
	v.make_vector(proj, orig);
	
	if ( is_floor && v[1] > 0 && orig[1] >= highestpeak) return 1e9;
	if (!is_floor && v[1] < 0 && orig[1] <= highestpeak) return 1e9;
	
	double xz_length = sqrt(v[0]*v[0] + v[2]*v[2]);
	
	if (xz_length < 1e-9) {
		crossing = orig;
		double down = field[((((int) orig[2]) * size) + (int) orig[0])*s];
		crossing[1] = down;
		return fabs(orig[1] - down);
	}
	
	int quadr = 0;
	if (v[0] < 0) quadr += 2;
	if (v[2] < 0) quadr++;
	float *myquadrant = quadrant[quadr];
	
	v.scale(1 / xz_length);
	Vector p = orig;
		
	while (INRANGE(p)) {
		int t = (((int) p[2]) * size) + (int) p[0];
		float *q = &myquadrant[t * s];
		
		
		
		int k = 0;double ss = MIN_SIZE;
		if (is_floor) {
			while (k <= slog && p[1] + v[1] * ss > q[k+1]) {
				++k;
				ss *= 2.0;
			}
		} else {
			while (k <= slog && p[1] + v[1] * ss < q[k+1]) {
				++k;
				ss *= 2.0;
			}
		}
		
		if (k == 0) {
			for (int i = 0; i < 8; i++) {
				if (!(INRANGE(p))) return 1e9;
				t = (((int) p[2]) * size) + (int) p[0];
				if (is_floor) {
					if (field[t] > p[1]) {
						crossing = p;
						crossing[1] = field[t];
						return crossing.distto(orig);
					}
				} else {
					if (field[t] < p[1]) {
						crossing = p;
						crossing[1] = field[t];
						return crossing.distto(orig);
					}
				}
				p += v;
			}
		} else {
			p += v * (ss * 0.5);
		}
	}
	return 1e9;
}


