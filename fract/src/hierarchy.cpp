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

static Task *buildtask;
static int task_num = 0;

/*
bool Hierarchy::build_hierarchy(int thesize, float *thebuff, bool is_floor)
{
	this->is_floor = is_floor;
	size = thesize;
	slog = power_of_2(size) - power_of_2(MIN_SIZE); // min size is 4
	s = slog + 2;
	
	
	sdmap = (float*) malloc(size * size * s * sizeof(float));
	
	for (int i = 0; i < size; i++)
		for (int j = 0; j < size; j++)
			sdmap[(i * size + j) * s] = thebuff[i * size + j];
		
	if (task_num == 0) {
		buildtask = new Task(HIERGEN, __FUNCTION__);
		buildtask->set_steps(((1 << (slog+2))-2) * NUM_VOXELS * size);
	}
	Task &task = *buildtask;
	
 	PVStruct all[768];
	int as = 0;
	for (int i = 0; i < size; i++) {
		for (int j = 0; j < size; j++) {
			int k = 0;
			float t = thebuff[i * size + j];
			while (k < as && all[k].f != t) k++;
			if (k == as) {
				all[as].f = t;
				all[as++].c = 1;
			} else {
				all[k].c++;
			}
			assert(as < 768);
		}
	}
	
	sort(all, as);
	
	int * imap = (int*) malloc(size * size * sizeof(int));
	for (int i = 0; i < size * size; i++) {
		float f = thebuff[i];
		imap[i] = 0;
		while (all[imap[i]].f != f) imap[i]++;
	}
	
	int dx = MIN_SIZE + 1;
	
	for (int k = 1; k <= slog + 1; k++, dx = (dx-1)*2 + 1) {
		int offsets[2048];
		for (int i = 0; i <= dx; i++) {
			int base = i*i;
			int j = 0;
			while (base + j*j < dx*dx) j++;
			offsets[i] = j-1;
		}
		int counts[768] = {0};
		int i0 = 0, j0 = 0;
		for (int i = 0; i < size; i++) {
			for (int j = 0; j < size; j++) {
				if (is_ok(i0, j0, i, j, 1, dx)) {
					counts[imap[i*size+j]]++;
				}
			}
		}
		for (; i0 < size; i0++) {
			
			bool up = i0 % 2 == 0;
			j0 = up ? 0 : size - 1;
			int jend = up ? size : -1;
			int jinc = up ? +1 : -1;
			for (; j0 != jend; j0 += jinc) {
				if (j0 != (up ? 0 : size-1)) {
					for (int id = -dx; id <= dx; id++) if (i0 + id >= 0 && i0 + id < size) {
						int m = offsets[id<0?-id:id];
						int x = j0 - jinc - jinc * m;
						int y = i0 + id;
						if (x >= 0 && x < size) {
							counts[imap[y * size + x]]--;
						}
						x = j0 + jinc * m;
						if (x >= 0 && x < size) {
							counts[imap[y * size + x]]++;
						}
					}
				}
				int t;
				if (is_floor) {
					t = as-1;
					while (counts[t] <= 0) t--;
				} else {
					t = 0;
					while (counts[t] <= 0) t++;
				}
				sdmap[(i0*size+j0)*s+k] = all[t].f;
			}
			j0 = up ? size - 1 : 0;
			for (int jd = -dx; jd <= dx; jd++) if (j0 + jd >= 0 && j0 + jd < size) {
				int m = offsets[jd < 0 ? -jd : jd];
				int y = i0 - 1 - m;
				int x = j0 + jd;
				if (y >= 0 && y < size) {
					counts[imap[y * size + x]]--;
				}
				y = i0 + m;
				if (y >= 0 && y < size) {
					counts[imap[y * size + x]]++;
				}
			}
			task += (1 << k);
		}
		
	}
	
	free(imap);
	
	if (++task_num == NUM_VOXELS) {
		delete buildtask;
	}
	return true;
}
*/


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

