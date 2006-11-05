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
#include "vector3.h"
#include "radiosity.h"
#include "voxel.h"
#include "hierarchy.h"
#include "threads.h"
#include "cpu.h"
#include "light.h"
#include "random.h"
#include "shaders.h"
#include "bitmap.h"

extern vox_t vox[NUM_VOXELS];

// "global" variables
double rad_amplification = 2.5;
int    rad_spv = 50;
double rad_stone_lightout = 0.3, rad_stone_specular = 8.0;
double rad_water_lightout = 0.78, rad_water_specular = 60.0;
double rad_light_radius = 8.0;
int    rad_light_samples = 100;

struct HDRColor {
	float b, g, r, a;
	HDRColor(unsigned RGBColor = 0)
	{
		b = (RGBColor & 0xff) / 255.0f; RGBColor >>= 8;
		g = (RGBColor & 0xff) / 255.0f; RGBColor >>= 8;
		r = (RGBColor & 0xff) / 255.0f;
		a = 0.0f;
	}
	
	HDRColor(float _b, float _g, float _r, float _a) {
		b = _b;
		g = _g;
		r = _r;
		a = _a;
	}
	
	static inline float fclamp(float x, float min, float max)
	{
		if (x < min) x = min; if (x > max) x = max;
		return x;
	}
	
	unsigned toRGB32(void) const
	{
		unsigned res = 0;

		res |= (unsigned) (255.0f * fclamp(r, 0.0f, 1.0f)); res <<= 8;
		res |= (unsigned) (255.0f * fclamp(g, 0.0f, 1.0f)); res <<= 8;
		res |= (unsigned) (255.0f * fclamp(b, 0.0f, 1.0f));
		return res;
	}
	
	inline HDRColor operator * (float x) {
		return HDRColor(b * x, g * x, r * x, a * x);
	}
	
	inline HDRColor operator + (const HDRColor& rhs)
	{
		return HDRColor(b + rhs.b, g + rhs.g, r + rhs.r, a + rhs.a);
	}
	
	inline HDRColor& operator += (const HDRColor & rhs)
	{
		b += rhs.b;
		g += rhs.g;
		r += rhs.r;
		a += rhs.a;
		return *this;
	}
};



struct RadiosityRecord {
	HDRColor *dest;
	HDRColor add;
	
	RadiosityRecord(){}
	RadiosityRecord(HDRColor *col, const HDRColor& addage) : dest(col), add(addage) {}
};

class RadiosityCalculation : public Parallel {
	Mutex dispmutex;
	double waterlevel;
	int linesdone;
	Mutex writelock;
	InterlockedInt ui, di;
	HDRColor *up, *down;
	int n, size_per_thread;
	
	RadiosityRecord * records;
	
public:
	RadiosityCalculation(int allcpus)
	{
		linesdone = 0;
		n = vox[0].size;
		down = new HDRColor[n * n];
		up   = new HDRColor[n * n];
		ui = 0; di = 0;
		size_per_thread = 15 + rad_spv;
		records = new RadiosityRecord[size_per_thread*allcpus];
		waterlevel = voxel_get_waterlevel();
	}
	
	Vector calc_normal(int i, int j, int k, int izb)
	{
		Vector t[3]; int q = 0;
		for (int l = 0; l < 4; l++) if (l != izb) {
			Vector x(i, vox[k].heightmap[i + j * n], j);
			if (l == 1 || l == 2) x[0] += 1.0;
			if (l == 2 || l == 3) x[2] += 1.0;
			t[q++] = x;
		}
		Vector a, b;
		a.make_vector(t[1], t[0]);
		b.make_vector(t[2], t[0]);
		Vector n = b ^ a;
		n.norm();
		return n;
	}
	
	void process_voxel(int i, int j, int k, RadiosityRecord *rec)
	{
		int m = 0;
		float material_lightout = rad_stone_lightout, material_specular = rad_stone_specular;
		
		Vector nor0 = calc_normal(i, j, k, 0);
		Vector nor1 = calc_normal(i, j, k, 3);
		Vector normal = nor0 + nor1; normal.norm();
		HDRColor mycolor = vox[k].input_texture[i + j *n];
		
		Vector pos = Vector(i + 0.5, vox[k].heightmap[i + j * n], j + 0.5);
		if (pos[1] < waterlevel) 
		{
			pos[1] = waterlevel;
			material_lightout = rad_water_lightout;
			material_specular = rad_water_specular;
			normal = Vector(0.0, 1.0, 0.0);
		}
		int light_coverage = 0;
		float normq = 0.0f;
		
		for (int e = 0; e < rad_light_samples; e++) {
			Vector lp;
			do {
				for (int r = 0; r < 3; r++) {
					lp[r] = light.p[r] + ( (drandom()*2.0-1.0)*rad_light_radius );
				}
			} while (lp.distto(light.p) > rad_light_radius);
			double realdist = lp.distto(pos);
			Vector dummy;
			if (fabs(realdist - vox[k].hierarchy.ray_intersect(lp, pos, dummy)) < 1.5) {
				light_coverage++;
				Vector t = lp - pos;
				t.norm();
				normq += fabs(t * normal);
			}
		}
		
		
		if (light_coverage) {
			float light_coeff = light_coverage / (float) rad_light_samples;
			float illum = light_coeff * 18.0 / sqrt(light.p.distto(pos)) * normq / light_coverage;
		
			HDRColor* mycolorp = k ? &up[i + j*n] : &down[i + j * n];
			rec[m++] = RadiosityRecord(mycolorp, HDRColor(illum, illum, illum, 0.0));
			
			for (int l = 0; l < rad_spv; l++) {
				Vector v;
				do {
					for (int r = 0; r < 3; r++)
						v[r] = drandom() * 2.0 - 1.0;
				} while (v * normal <= 0);
				v.norm();
				float mult =  v * normal;
				mult = powf(mult, material_specular) * material_lightout * rad_amplification;
				Vector p = pos + v * 10.0;
				float mind = 1e6;
				int bk=-1; Vector bvec;
				for (int w = 0; w < 2; w++) {
					float dd; Vector vec;
					dd = vox[w].hierarchy.ray_intersect(p, v, vec);
					if (dd < mind) {
						mind = dd;
						bvec = vec;
						bk = w;
					}
				}
				if (bk != -1) {
					if (mind < 1.0f) mind = 1.0f;
					int ii = (int) bvec[0];
					int jj = (int) bvec[2];
					if (ii >= 0 && ii < n && jj >= 0 && jj < n) {
						HDRColor *dest = bk ? &up[ii + jj * n] : &down[ii + jj * n];
						rec[m++] = RadiosityRecord(dest, mycolor * (mult / sqrt(mind)));
						assert(m < size_per_thread);
					}
				}
			}
			
			writelock.enter();
			for (int i = 0; i < m; i++)
				*(rec[i].dest) += rec[i].add;
			writelock.leave();
		}
	}
	
	void entry(int tidx, int ttotal)
	{
		int j;
		while ((j = di++) < n - 1) {
			for (int i = 0; i < n - 1; i++) {
				process_voxel(i, j, 0, records + (tidx * size_per_thread));
			}
			dispmutex.enter();
			++linesdone;
			printf("\rCalculating Radiosity: [%6.2lf%%]", linesdone * 100.0 / (2*n-2));
			fflush(stdout);
			dispmutex.leave();
		}
		while ((j = ui++) < n - 1) {
			for (int i = 0; i < n - 1; i++) {
				process_voxel(i, j, 1, records + (tidx * size_per_thread));
			}
			dispmutex.enter();
			++linesdone;
			printf("\rCalculating Radiosity: [%6.2lf%%]", linesdone * 100.0 / (2*n-2));
			fflush(stdout);
			dispmutex.leave();
		}
	}
	
	void blur_map(HDRColor * map, const ConvolveMatrix& m)
	{
		HDRColor *result = new HDRColor [n*n];
		int sum = 0;
		for (int i = 0; i < m.n; i++)
			for (int j = 0; j < m.n; j++)
				sum += m.coeff[i][j];
		float multi = 1.0f / sum;
		for (int i = 0; i < n; i++) {
			for (int j = 0; j < n; j++) {
				HDRColor res;
				for (int q = 0; q < m.n; q++) {
					for (int w = 0; w < m.n; w++) {
						int ii = i + q - m.n/2;
						int jj = j + w - m.n/2;
						if (ii >= 0 && ii < n && jj >= 0 && jj < n) {
							HDRColor pick = map[ii *n + jj];
							res += pick * (float) m.coeff[q][w];
						}
					}
				}
				result[i*n+j] = res * multi;
			}
		}
		for (int i = 0; i < n; i++)
			for (int j = 0; j < n; j++)
				map[i*n+j] = result[i*n+j];
		delete [] result;
	}
	
	void write_map(Uint32 *tex, HDRColor *map, const char * fn)
	{
		RawImg a(n, n);
		Uint32 *d = (Uint32*) a.get_data();
		for (int i = 0; i < n; i++) {
			for (int j = 0; j < n; j++) {
				int t = i * n + j;
				HDRColor col1 = tex[t];
				HDRColor col2 = map[t];
				col1.r *= col2.r;
				col1.g *= col2.g;
				col1.b *= col2.b;
				d[t] = col1.toRGB32();
			}
		}
		a.save_bmp(fn);
	}
	
	void finalize(void)
	{
		printf("\n"); fflush(stdout);
		blur_map(down, blur_ma3x_3);
		blur_map(up, blur_ma3x_3);
		
		write_map(vox[0].input_texture, down, "data/rad_down.bmp");
		write_map(vox[1].input_texture, up,   "data/rad_up.bmp"  );
	}
	
	~RadiosityCalculation()
	{
		delete [] records;
		delete [] down;
		delete [] up;
	}
	
};


void radiosity_calculate(ThreadPool &threadman)
{
	static bool calculated = false;
	if (calculated) return;
	calculated = true;
	RadiosityCalculation radiosity_calculation(cpu.count);
	threadman.run(&radiosity_calculation, cpu.count);
	radiosity_calculation.finalize();
}
