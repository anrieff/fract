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

//#define FAST_HIERARCHY

const char * hierarchy_cache = "data/hcache";


/** @class Hierarchy */

bool Hierarchy::try_load_map_from_cache(float *map)
{
	unsigned myhash[4];
	unsigned header[5];
	
	MD5Hasher hasher;
	hasher.add_data(map, size * size * sizeof(float));
	hasher.result(myhash);
	
	FILE *f = fopen(hierarchy_cache, "rb");
	if (!f) return false;
	
	while (5 == fread(header, sizeof(header[0]), 5, f)) {
		// see if it is our record:
		bool good = true;
		for (int i = 0; i < 4; i++) if (header[i] != myhash[i]) {
			good = false;
			break;
		}
		if (good) {
			int r = fread(sdmap, s * sizeof(float), size * size, f);
			fclose(f);
			return r == size * size;
		} else {
			// skip data
			fseek(f, header[4], SEEK_CUR);
		}
	}
	
	fclose(f);
	return false;
}

void Hierarchy::insert_map_in_cache(float *map)
{
	FILE *f = fopen(hierarchy_cache, "ab");
	unsigned header[5];
	
	MD5Hasher hasher;
	hasher.add_data(map, size * size * sizeof(float));
	hasher.result(header); // md5 fills the first four entries
	
	header[4] = size * size * sizeof(float) * s; // the data size
	
	fwrite(header, sizeof(header[0]), 5, f);
	fwrite(sdmap, sizeof(float) * s, size * size, f);
	
	fclose(f);
}


bool Hierarchy::is_ok(int i0, int j0, int i1, int j1, int blocksize, int dx) const
{
	for (int t = 0; t < 4; t++)
	{
		int i2 = i1 + (t & 1);
		int j2 = j1 + (t & 2) / 2;
		i2 *= blocksize;
		j2 *= blocksize;
		if (t & 1) i2--;
		if (t & 2) j2--;
		if (((i2-i0)*(i2-i0) + (j2-j0)*(j2-j0)) >= dx*dx) return false;
		if (blocksize == 1) break;
	}
	return true;
}

void Hierarchy::build_umaps(float *map)
{
	for (int k = 0; k <= blog; k++) {
		int bs = 1 << k;
		int n = size / bs;
		umaps[k] = (float*) malloc(n * n * sizeof(float));
		for (int i = 0; i < n; i++) {
			for (int j = 0; j < n; j++) {
				float & r = umaps[k][i * n + j];
				r = is_floor ? -1e9 : 1e9;
				for (int q = 0; q < bs; q++) {
					for (int w = 0; w < bs; w++) {
						float t = map[(i * bs + q) * size + j * bs + w];
						if ( is_floor && t > r) r = t;
						if (!is_floor && t < r) r = t;
					}
				}
			}
		}
	}
}

void Hierarchy::free_umaps(void)
{
	for (int i = 0; i <= blog; i++) {
		if (umaps[i]) {
			free(umaps[i]);
		}
		umaps[i] = NULL;
	}
}

void Hierarchy::process(float &r, int i0, int j0, int i1, int j1, int ll, int dx)
{
	if (ll < 0) return;
	int bs = 1 << ll;
	int n = size >> ll;
	
	double y = i1 * bs + bs * 0.5;
	double x = j1 * bs + bs * 0.5;
	
	if (sqrt((x-j0)*(x-j0) + (y-i0)*(y-i0)) > dx + bs * 0.5) return;
	
	if (is_ok(i0, j0, i1, j1, bs, dx)) {
		float t = umaps[ll][i1 * n + j1];
		if ( is_floor && t > r) r = t;
		if (!is_floor && t < r) r = t;
	} else {
		float t = umaps[ll][i1 * n + j1];
		if ( is_floor && t < r) return;
		if (!is_floor && t > r) return;
		for (int i2 = i1 * 2; i2 < i1 * 2 + 2; i2++) {
			for (int j2 = j1 * 2; j2 < j1 * 2 + 2; j2++) {
				process(r, i0, j0, i2, j2, ll - 1, dx);
			}
		}
	}
}

struct PVStruct {
	float f;
	int c;
	
	bool operator < (const PVStruct & r) const { return f < r.f; }
};


#ifndef FAST_HIERARCHY

static float fmax4(float a, float b, float c, float d)
{
	float ff = a;
	ff = b > ff ? b : ff;
	ff = c > ff ? c : ff;
	ff = d > ff ? d : ff;
	return ff;
}

static float fmin4(float a, float b, float c, float d)
{
	float ff = a;
	ff = b < ff ? b : ff;
	ff = c < ff ? c : ff;
	ff = d < ff ? d : ff;
	return ff;
}

bool Hierarchy::build_hierarchy(int thesize, float *thebuff, bool is_floor)
{
	float (*ffun) (float,float,float,float) = is_floor ? fmax4 : fmin4;
	this->is_floor = is_floor;
	size = thesize;
	slog = power_of_2(size);
	int allocsize = 16* size*size;
		//maps[slog ] = thebuff;

	int csize = size * 2;
	for (int i= slog; i>=0; i--) {
		allocsize /= 4;
		int oldsize = csize;
		csize /= 2;

		maps[i] = (float *) malloc(allocsize);
		if (maps[i] == NULL) {
			i++;
			while (i < slog) {
				free(maps[i]);
				i++;
			}
			return false;
		}

			// memory allocated; fill with usefull info
		if (i == slog) {
			memcpy(maps[i], thebuff, allocsize);
			continue;
		}
		int p = 0;
		for (int y = 0; y < csize; y++)
			for (int x = 0; x < csize; x++,p++) {
			maps[i][p] = ffun(
					maps[i+1][( y * 2      * oldsize) + x * 2    ],
			maps[i+1][( y * 2      * oldsize) + x * 2 + 1],
			maps[i+1][((y * 2 + 1) * oldsize) + x * 2    ],
			maps[i+1][((y * 2 + 1) * oldsize) + x * 2 + 1]
					 );
			}
	}
	return true;
}

static bool pred_less(float a, float b) { return a < b; }
static bool pred_more(float a, float b) { return a > b; }


// this ugly macro is used by the Ray-heightfield intersection routine, DON'T DELETE!
#define INTERNAL 			float maxd = fmin(tMaxXsm, tMaxZsm); \
					cdist += 0.0001;\
					X = (int) (Camera[0] + cdist * diff0); \
					Z = (int) (Camera[2] + cdist * diff2); \
					Y = Yi + cdist * diff1; \
					if (enter_d == fromX) { \
						tMaxX = cdist + tDeltaX; \
						tMaxZ = cdist + (((stepZ+1)>>1)+Z-(Camera[2] + cdist * diff2))*rcpdiff2; \
} \
					if (enter_d == fromZ) { \
						tMaxX = cdist + (((stepX+1)>>1)+X-(Camera[0] + cdist * diff0))*rcpdiff0; \
						tMaxZ = cdist + tDeltaZ; \
} \
					while (tMaxX < maxd || tMaxZ < maxd) { \
						if (pred(Y, (ilast = maps[slog][X + (Z<<slog)]))) { \
							crossing = Vector(X, ilast, Z); \
							return orig.distto(crossing); \
} \
						if (tMaxX < tMaxZ) { \
							Y = Yi + tMaxX * diff1; \
							if (pred(Y, ilast)) { \
								crossing =  Vector(X, ilast, Z); \
								return orig.distto(crossing); \
} \
							tMaxX += tDeltaX; \
							X += stepX; \
} else { \
							Y = Yi + tMaxZ * diff1; \
							if (pred(Y, ilast)) { \
								crossing = Vector(X, ilast, Z); \
								return orig.distto(crossing); \
} \
							tMaxZ += tDeltaZ; \
							Z += stepZ; \
} \
} \


float Hierarchy::ray_intersect(const Vector & orig, const Vector & proj, Vector & crossing)
{
	bool (*pred) (float, float) = is_floor ? pred_less : pred_more;
	++raycasts;
	bool coords_inited = false;
	int X, Z, stepX, stepZ, Xsm, Zsm; // all *sm variables are used in the "Large" walk
	float tMaxX, tMaxZ, tDeltaX, tDeltaZ, last, ilast;
	float tMaxXsm, tMaxZsm, tDeltaXsm, tDeltaZsm, cdist;
	EnterDirection enter_d = fromNowhere;
	Vector Camera; // camera is where the first point above the heightfield is

	/* Check for rays which go straight in the sky */
	if (is_floor) {
		if (proj[1] > orig[1] && orig[1] > maps[0][0]) {
			return 1e9;
		}
	} else {
		if (proj[1] < orig[1] && orig[1] < maps[0][0])
			return 1e9;
	}

	/* check whether we are "below" the terrain */
	if (InTexture(orig.v[0], orig.v[2])) {
		coords_inited = true;
		Camera = orig;
		X = (int) orig[0];
		Z = (int) orig[2];
		if (is_floor) {
			if (getheight_bilinear(orig.v[0], orig.v[2]) >= orig.v[1])
				return 1e9;
		} else {
			if (getheight_bilinear(orig.v[0], orig.v[2]) <= orig.v[1])
				return 1e9;
		}
	} else {
		X = Z = 0; // make gcc happy
	}
	/* traversal vector in diff */
	Vector diff;
	diff.make_vector(proj, orig);

		// precalculate stuff...
	float rcpdiff0 = 1.0f / diff[0], rcpdiff2 = 1.0f / diff[2];
	float rcp640 = rcpdiff0 * OUTER_SIZE, rcp642 = rcpdiff2 * OUTER_SIZE;

	/* If we're outside the grid, find where we enter it */
	if (!coords_inited) {
		float p[4];
		p[0] =       - orig[0]  * rcpdiff0;
		p[1] = (size - orig[0]) * rcpdiff0;
		p[2] =       - orig[2]  * rcpdiff2;
		p[3] = (size - orig[2]) * rcpdiff2;
		float best = 1e9;
		for (int i = 0; i < 4; i++)
			if (p[i] > 0 && p[i] < best) best = p[i];
		if (best > 1e6)
			return 1e9;
		Camera.macc(orig, diff, best);
		X = (int) Camera[0];
		Z = (int) Camera[2];
	}
// prepare our OUTER loop
	int out = slog - OUTER_MIPLEVEL, Gsize = ~(size / OUTER_SIZE - 1);
	Xsm = X / OUTER_SIZE;
	Zsm = Z / OUTER_SIZE;
	Vector cm64 = Camera;
	cm64.scale(1.0/OUTER_SIZE);
	stepX = diff[0]>0?+1:-1;
	stepZ = diff[2]>0?+1:-1;
	tMaxX = (((stepX+1)>>1)+X-Camera[0])*rcpdiff0;
	tMaxZ = (((stepZ+1)>>1)+Z-Camera[2])*rcpdiff2;
	tMaxXsm = (((stepX+1)>>1)+Xsm-cm64[0])*rcp640;
	tMaxZsm = (((stepZ+1)>>1)+Zsm-cm64[2])*rcp642;

	tDeltaX = fabsf(rcpdiff0);
	tDeltaZ = fabsf(rcpdiff2);
	tDeltaXsm = OUTER_SIZE * tDeltaX;
	tDeltaZsm = OUTER_SIZE * tDeltaZ;
	float Yi = Camera[1];
	float Y = Yi, Ysm = Yi, diff0 = diff[0], diff1 = diff[1], diff2 = diff[2];
	cdist = 0;
	while ((Xsm & Gsize) == 0 && (Zsm & Gsize) == 0) {
		if (pred(Ysm, (last = maps[out][Xsm + (Zsm << out)]))) {
				// possibility for intersection detected, examine closely...
			INTERNAL
		}
		if (tMaxXsm < tMaxZsm) {
			Ysm = Yi + tMaxXsm * diff1;
			if (pred(Ysm, last)) {
				// possibility for intersection detected, examine closely...
				INTERNAL
			}
			cdist = tMaxXsm;
			enter_d = fromX;

			tMaxXsm += tDeltaXsm;
			Xsm += stepX;
		} else {
			Ysm = Yi + tMaxZsm * diff1;
			if (pred(Ysm, last)) {
				// possibility for intersection detected, examine closely...
				INTERNAL
			}
			cdist = tMaxZsm;
			enter_d = fromZ;
			tMaxZsm += tDeltaZsm;
			Zsm += stepZ;
		}
	}
	return 1e9;
}

#else

static Task *buildtask;
static int task_num = 0;

bool Hierarchy::build_hierarchy(int thesize, float *thebuff, bool is_floor)
{
#ifdef DEBUG
	double startt = bTime();
#endif
	this->is_floor = is_floor;
	size = thesize;
	slog = power_of_2(size) - power_of_2(MIN_SIZE); // min size is 4
	s = slog + 2;
	
	
	sdmap = (float*) malloc(size * size * s * sizeof(float));
	
	for (int i = 0; i < size; i++)
		for (int j = 0; j < size; j++)
			sdmap[(i * size + j) * s] = thebuff[i * size + j];
	
	if (try_load_map_from_cache(thebuff)) {
		return true;
	}
	
	if (task_num == 0) {
		buildtask = new Task(HIERGEN, __FUNCTION__);
		buildtask->set_steps(NUM_VOXELS * size);
	}
	Task &task = *buildtask;
	
	blog = power_of_2(size) / 2;
	int bs = 1 << blog;
	int n = size / bs;
	
	build_umaps(thebuff);

	for (int i0 = 0; i0 < size; i0++) {
		for (int j0 = 0; j0 < size; j0++) {
			int dx = MIN_SIZE+1;
			for (int k = 1; k <= slog + 1; k++, dx = (dx-1)*2+1) {
				float & r = sdmap[(i0*size+j0) * s + k];
				r = is_floor ? -1e9 : + 1e9;
				int mini = i0 - dx;
				int maxi = i0 + dx;
				int minj = j0 - dx;
				int maxj = j0 + dx;
				
				mini >?= 0;
				maxi <?= size;
				minj >?= 0;
				maxj <?= size;
				
				mini = (mini / bs);
				minj = (minj / bs);
				maxi = (maxi / bs) + 1;
				maxj = (maxj / bs) + 1;
				
				mini >?= 0;
				maxi <?= n;
				minj >?= 0;
				maxj <?= n;
				
				for (int i1 = mini; i1 < maxi; i1++) {
					for (int j1 = minj; j1 < maxj; j1++) {
						process(r, i0, j0, i1, j1, blog, dx);
					}
				}
			}
		}
		task++;
	}
	free_umaps();
#ifdef DEBUG
	printf("Generation time : %.2lfs\n", bTime()- startt);
#endif
	insert_map_in_cache(thebuff);
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
	double xz_length = sqrt(v[0]*v[0] + v[2]*v[2]);
	
	if (xz_length < 1e-9) {
		crossing = orig;
		double down = sdmap[((((int) orig[2]) * size) + (int) orig[0])*s];
		crossing[1] = down;
		return fabs(orig[1] - down);
	}
	
	v.scale(1 / xz_length);
	Vector p = orig;
	
	while (INRANGE(p)) {
		int t = (((int) p[2]) * size) + (int) p[0];
		float *q = &sdmap[t * s];
		
		
		
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
					if (sdmap[t*s] > p[1]) {
						crossing = p;
						crossing[1] = sdmap[t*s];
						return crossing.distto(orig);
					}
				} else {
					if (sdmap[t*s] < p[1]) {
						crossing = p;
						crossing[1] = sdmap[t*s];
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

#endif // FAST_HIERARCHY
