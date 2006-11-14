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
#include "cvars.h"
#include "vectormath.h"

extern vox_t vox[NUM_VOXELS];
extern Vector cur;

// "global" variables
double rad_amplification = 2.5;
int    rad_spv = 50;
double rad_stone_lightout = 0.3, rad_stone_specular = 8.0;
double rad_water_lightout = 0.78, rad_water_specular = 60.0;
double rad_light_radius = 8.0;
int    rad_light_samples = 100;
double rad_indirect_coeff = 0.2;

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
	
	inline void operator ^= (const HDRColor & rhs)
	{
		b *= rhs.b;
		g *= rhs.g;
		r *= rhs.r;
	}
};


template <class T>
struct JournalRecord {
	T *dest;
	T add;
	JournalRecord(){}
	JournalRecord(T *dst, const T& addage) : dest(dst), add(addage) {}
};

template <class T>
struct Journal {
	JournalRecord<T>* data;
	int m, maxcap;
	
	Journal()
	{
		data = NULL;
		m = 0;
	}
	
	void alloc(size_t num)
	{
		maxcap = num;
		data = new JournalRecord<T>[num];
		memset(data, 0, sizeof(T) * num);
	}
	
	~Journal()
	{
		if (data) delete [] data;
		data = NULL;
	}
	
	inline void add(T *dst, const T& addage) 
	{ 
		data[m++] = JournalRecord<T>(dst, addage); 
		assert(m < maxcap);
	}
	void reset(void) { m = 0;}
	void write(void)
	{
		for (int i = 0; i < m; i++) {
			*(data[i].dest) += data[i].add;
		}
	}
};

typedef Journal<HDRColor> ColorJournal;
typedef Journal<Vector> VectorJournal;

const int passes = 4;

class RadiosityCalculation : public Parallel {
	Mutex dispmutex;
	double waterlevel;
	int linesdone;
	Mutex writelock;
	InterlockedInt ui, di;
	HDRColor *up, *down;
	HDRColor *rmaps[2][2];
	HDRColor **drmap;
	Vector *vmaps[2][2];
	Vector **dvmap;
	Vector *normals[2];
	int n, size_per_thread;
	
	Journal<HDRColor> * col_journals;
	Journal<Vector> * vec_journals;
	Barrier *barriers;
	
	InterlockedInt pixels_traced;
	int all_pixels;
	
public:
	RadiosityCalculation(int allcpus)
	{
		all_pixels = 2000000;
		pixels_traced = 0;
		linesdone = 0;
		n = vox[0].size;
		down = new HDRColor[n * n];
		up   = new HDRColor[n * n];
		for (int i = 0; i < 2; i++) {
			for (int j = 0; j < 2; j++) {
				rmaps[i][j] = new HDRColor[n*n];
				vmaps[i][j] = new Vector[n*n];
				memset(vmaps[i][j], 0, n*n*sizeof(Vector));
			}
		}
		ui = 0; di = 0;
		size_per_thread = (2 + rad_spv * 2) * n;
		//records = new RadiosityRecord[size_per_thread*allcpus];
		col_journals = new Journal<HDRColor> [allcpus];
		for (int i = 0; i < allcpus; i++) 
			col_journals[i].alloc(size_per_thread);
		vec_journals = new Journal<Vector>   [allcpus];
		for (int i = 0; i < allcpus; i++) {
			vec_journals[i].alloc((rad_spv+2)*n);
		}
		waterlevel = voxel_get_waterlevel();
		
		normals[0] = new Vector[n*n];
		normals[1] = new Vector[n*n];
		for (int k = 0; k < 2; k++) {
			for (int j = 0; j < n; j++) {
				for (int i = 0; i < n; i++) {
					int ii = i; if (ii == n-1) ii--;
					int jj = j; if (jj == n-1) jj--;
					Vector nor0 = calc_normal(ii, jj, k, 0);
					Vector nor1 = calc_normal(ii, jj, k, 2);
					Vector normal = nor0 + nor1; normal.norm();
					if (k == 1) normal.scale(-1.0);
					if (k == 0 && vox[0].heightmap[i*n+j] < waterlevel)
						normal = Vector(0,1,0);
					normals[k][j*n+i] = normal;
				}
			}
		}
		barriers = new Barrier[passes*2];
		for (int i = 0; i < passes * 2; i++)
			barriers[i].set_threads(allcpus);
		drmap = rmaps[0];
		dvmap = vmaps[0];
	}
	
	Vector calc_normal(int i, int j, int k, int izb)
	{
		Vector t[3]; int q = 0;
		for (int l = 0; l < 4; l++) if (l != izb) {
			int ii = i, jj = j;
			if (l == 1 || l == 2) ii++;
			if (l == 2 || l == 3) jj++;
			Vector x(jj, vox[k].heightmap[ii * n + jj], ii);
			t[q++] = x;
		}
		Vector a, b;
		a.make_vector(t[1], t[0]);
		b.make_vector(t[2], t[0]);
		Vector n = a ^ b;
		n.norm();
		return n;
	}
	
	void process_voxel(int i, int j, int k, ColorJournal &rec, VectorJournal &vec)
	{
		float material_lightout = rad_stone_lightout, material_specular = rad_stone_specular;
		float accepted_thresh = 1.0f / 512.f / rad_spv;
		
		Vector nor0 = calc_normal(i, j, k, 0);
		Vector nor1 = calc_normal(i, j, k, 2);
		Vector normal = nor0 + nor1; normal.norm();
		if (k == 1) normal.scale(-1.0);
		HDRColor mycolor = vox[k].input_texture[i * n + j];
		
#ifdef DEBUG
		if (mycolor.b > 2 * mycolor.r) {
			printf("Boo!\n");
		}
#endif
		
		Vector pos = Vector(j + 0.5, vox[k].heightmap[i *n + j], i + 0.5);
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
			if (fabs(realdist - vox[k].hierarchy.ray_intersect_exact(lp, pos, dummy)) < 1.5 &&
			    vox[1-k].hierarchy.ray_intersect_exact(lp, pos, dummy) > realdist) {
				Vector t = lp - pos;
				if (t * normal > 0.0) {
					light_coverage++;
					t.norm();
					normq += sqrt(t * normal);
				}
			}
		}
		
		Vector reflectance_normal = pos - light.p;
		reflectance_normal.norm();
		reflectance_normal -= normal * ((normal * reflectance_normal) * 2.0);
		
		if (light_coverage) {
			float light_coeff = light_coverage / (float) rad_light_samples;
			float illum = light_coeff * rad_amplification * 
				(1.0 - rad_indirect_coeff) / sqrt(light.p.distto(pos)) * normq / light_coverage;
			float dist_light_to_p = light.p.distto(pos);
			float rcp_dist_light_to_point = 1.0f / dist_light_to_p;
		
			HDRColor* mycolorp = k ? &up[i*n + j] : &down[i*n + j];
			rec.add(mycolorp, HDRColor(illum, illum, illum, 0.0));
			
			for (int l = 0; l < rad_spv; l++) {
				Vector v;
				do {
					for (int r = 0; r < 3; r++)
						v[r] = drandom() * 2.0 - 1.0;
				} while (v * reflectance_normal <= 0);
				v.norm();
				float mult =  v * reflectance_normal;
				mult = powf(mult, material_specular) * material_lightout * rad_amplification * rad_indirect_coeff / rad_spv;
				if (mult < accepted_thresh) continue;
				Vector p = pos + v * 10.0;
				float mind = 1e6;
				int bk=-1; Vector bvec;
				for (int w = 0; w < 2; w++) {
					if (w == k) continue;
					float dd; Vector vec;
					dd = vox[w].hierarchy.ray_intersect_exact(p, p+v, vec);
					if (dd < mind) {
						mind = dd;
						bvec = vec;
						bk = w;
					}
				}
				if (bk != -1) {
					if (mind < 1.0f) mind = 1.0f;
					int jj = (int) bvec[0];
					int ii = (int) bvec[2];
					if (ii >= 0 && ii < n && jj >= 0 && jj < n) {
						HDRColor *dest = bk ? &up[ii * n + jj] : &down[ii * n + jj];
						float lambda = mind * rcp_dist_light_to_point;
						HDRColor wrcolor = mycolor * (mult / sqrt(1 + lambda));
						wrcolor ^= vox[k].input_texture[ii*n+jj];
						rec.add(dest, wrcolor);
						rec.add(&drmap[bk][ii*n+jj], wrcolor);
						Vector destnorm = normals[bk][ii*n+jj];
						Vector dv = v + (destnorm * ((v * destnorm) *-2.0));
						vec.add(&dvmap[bk][ii*n+jj], dv);
					}
				}
			}
			
		}
	}
	
	void process_secondary_reflection(int i, int j, int k, HDRColor mycolor, Vector reflectance_normal, ColorJournal &rec, VectorJournal &vec)
	{
		float material_lightout = rad_stone_lightout, material_specular = rad_stone_specular;
		float accepted_thresh = 1.0f / 2048.f / rad_spv;
		reflectance_normal.norm();
		
		Vector pos = Vector(j + 0.5, vox[k].heightmap[i *n + j], i + 0.5);
		if (pos[1] < waterlevel) 
		{
			pos[1] = waterlevel;
			material_lightout = rad_water_lightout;
			material_specular = rad_water_specular;
			reflectance_normal = Vector(0.0, 1.0, 0.0);
		}
		for (int l = 0; l < rad_spv; l++) {
			Vector v;
			do {
				for (int r = 0; r < 3; r++)
					v[r] = drandom() * 2.0 - 1.0;
			} while (v * reflectance_normal <= 0);
			v.norm();
			float mult =  v * reflectance_normal;
			mult = powf(mult, material_specular) * material_lightout * rad_amplification * rad_indirect_coeff / rad_spv;
			if (mult < accepted_thresh) continue;
			Vector p = pos + v * 10.0;
			float mind = 1e6;
			int bk=-1; Vector bvec;
			for (int w = 0; w < 2; w++) {
				if (w == k) continue;
				float dd; Vector vec;
				dd = vox[w].hierarchy.ray_intersect_exact(p, p+v, vec);
				if (dd < mind) {
					mind = dd;
					bvec = vec;
					bk = w;
				}
			}
			if (bk != -1) {
				if (mind < 1.0f) mind = 1.0f;
				int jj = (int) bvec[0];
				int ii = (int) bvec[2];
				if (ii >= 0 && ii < n && jj >= 0 && jj < n) {
					HDRColor *dest = bk ? &up[ii * n + jj] : &down[ii * n + jj];
					float lambda = mind * 0.002f;
					HDRColor wrcolor = mycolor * (mult / sqrt(1 + lambda));
					wrcolor ^= vox[k].input_texture[ii*n+jj];
					rec.add(dest, wrcolor);
					rec.add(&drmap[bk][ii*n+jj], wrcolor);
					Vector destnorm = normals[bk][ii*n+jj];
					Vector dv = v + (destnorm * ((v * destnorm) *-2.0));
					vec.add(&dvmap[bk][ii*n+jj], dv);
				}
			}
		}
	}
	
	void entry(int tidx, int ttotal)
	{
		int j;
		ColorJournal &col_j = col_journals[tidx];
		VectorJournal &vec_j = vec_journals[tidx];
		while ((j = di++) < n - 1) {
			col_j.reset(); vec_j.reset();
			for (int i = 0; i < n - 1; i++) {
				process_voxel(i, j, 0, col_j, vec_j);
			}
		
			writelock.enter(); col_j.write(); vec_j.write(); writelock.leave();

			dispmutex.enter();
			++linesdone;
			printf("\rCalculating Radiosity: [%6.2lf%%]", linesdone * 100.0 / (2*n-2));
			fflush(stdout);
			dispmutex.leave();
		}
		while ((j = ui++) < n - 1) {
			col_j.reset(); vec_j.reset();
			for (int i = 0; i < n - 1; i++) {
				process_voxel(i, j, 1, col_j, vec_j);
			}
			
			writelock.enter(); col_j.write(); vec_j.write(); writelock.leave();
			
			dispmutex.enter();
			++linesdone;
			printf("\rCalculating Radiosity: [%6.2lf%%]", linesdone * 100.0 / (2*n-2));
			fflush(stdout);
			dispmutex.leave();
		}
		for (int pass = 0; pass < passes; pass++) {
			barriers[pass*2].checkout();
			if (tidx == 0) {
				dispmutex.enter();
				printf("\n"); fflush(stdout);
				dispmutex.leave();
			}
			drmap = rmaps[(1+pass)%2];
			dvmap = vmaps[(1+pass)%2];
			HDRColor **crmap = rmaps[pass%2];
			Vector **cvmap = vmaps[pass%2];
			for (int i = 0; i < 2; i++) {
				multithreaded_memset(crmap[i], 0, n*n, tidx, ttotal);
				multithreaded_memset(cvmap[i], 0, n*n, tidx, ttotal);
			}
			barriers[pass*2+1].checkout();
			
			for (int k = 0; k < 2; k++) {
				for (int j = 0; j < n - 1; j++) if (j % ttotal == tidx) {
					col_j.reset();
					vec_j.reset();
					for (int i = 0; i < n - 1; i++) {
						process_secondary_reflection(i, j, k, crmap[k][j*n+i], cvmap[k][j*n+i], col_j, vec_j);
					}
					
					writelock.enter(); col_j.write(); vec_j.write(); writelock.leave();

					dispmutex.enter();
					++linesdone;
					printf("\rCalculating secondary rays, pass %d: [%6.2lf%%]",
					       pass+1, (linesdone - (pass+1)*(2*n-2))*100.0 / (2*n-2));
					fflush(stdout);
					dispmutex.leave();
				}
			}
			
		}
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
	
	void output_pov(const char * fn)
	{
		FILE *f = fopen(fn, "wt");
		if (!f) return;
		
		fprintf(f, "global_settings { max_trace_level 10 ambient_light 0.05 /*radiosity { brightness 1.0 count 160 }*/ }\n\n");
		
		Vector la = cur;
		la[0] += sin(CVars::alpha) * cos(CVars::beta);
		la[1] += sin(CVars::beta);
		la[2] += cos(CVars::alpha) * cos(CVars::beta);
		fprintf(f, "camera { location <%.3lf, %.3lf, %.3lf> look_at <%.3lf, %.3lf, %.3lf> }\n\n",
			cur[0], cur[1], cur[2], la[0], la[1], la[2]);
		
		fprintf(f, "light_source { <%.3lf, %.3lf, %.3lf>, <1,1,1> area_light <%.3lf,0,0>, <0,0,%.3lf>, 6, 6}\n\n",
			light.p[0],light.p[1], light.p[2], rad_light_radius, rad_light_radius);
			
		Vector *norms[2];
		for (int k = 0; k < 2; k++) {
			norms[k] = new Vector[n*n];
			for (int i = 0; i < n; i++)
				for (int j = 0; j < n; j++)
					norms[k][i*n+j].zero();
			 
			for (int i = 0; i < n - 1; i++) {
				for (int j = 0; j < n - 1; j++) {
					Vector nor0 = calc_normal(i, j, k, 0);
					Vector nor1 = calc_normal(i, j, k, 2);
					if (k == 1) {
						nor0.scale(-1.0);
						nor1.scale(-1.0);
					}
					for (int l = 1; l < 4; l++) {
						int ii = i, jj = j;
						if (l == 1 || l == 2) ii++;
						if (l == 2 || l == 3) jj++;
						norms[k][ii*n+jj] += nor0;
					}
					for (int l = 0; l < 4; l++) if (l != 2) {
						int ii = i, jj = j;
						if (l == 1 || l == 2) ii++;
						if (l == 2 || l == 3) jj++;
						norms[k][ii*n+jj] += nor1;
					}
				}
			}
			for (int i = 0; i < n; i++) {
				for (int j = 0; j < n; j++) {
					norms[k][i*n+j].norm();
				}
			}
			int all = n*n;
			fprintf(f, "#declare Mesh_%c=\nmesh2{ \n", 'A' + k);
			fprintf(f, "vertex_vectors {\n%d\n", all);
			for (int i = 0; i < n; i++) {
				for (int j = 0; j < n; j++) {
					fprintf(f, "\t<%.3lf, %.3lf, %.3lf>,\n", (double)j, vox[k].heightmap[i*n+j], (double) i);
				}
			}
			fprintf(f, "}\nnormal_vectors {\n%d\n", all);
			for (int i = 0; i < n; i++) {
				for (int j = 0; j < n; j++) {
					const Vector &v = norms[k][i*n+j];
					fprintf(f, "\t<%.3lf, %.3lf, %.3lf>,\n", v[0], v[1], v[2]);
				}
			}
			fprintf(f, "}\nuv_vectors {\n%d\n", all);
			for (int i = 0; i < n; i++) {
				for (int j = 0; j < n; j++) {
					fprintf(f, "\t<%.5lf, %.5lf>,\n", j / (double) n, i / (double) n);
				}
			}
			all = (n-1) * (n-1) * 2;
			char *names[] = { "face_indices", "normal_indices", "uv_indices" };
			for (int type = 0; type < 3; type++) {
				fprintf(f, "}\n%s {\n%d\n", names[type], all);
				for (int i = 0; i < n - 1; i++) {
					for (int j = 0; j < n - 1; j++) {
						for (int skip = 0; skip < 4; skip += 2) {
							int w[3], q = 0;
							for (int l = 0; l < 4; l++) if (l != skip) {
								int ii = i;
								int jj = j;
								if (l == 1 || l == 2) ii++;
								if (l == 2 || l == 3) jj++;
								w[q++] = ii*n + jj;
							}
							fprintf(f, "\t<%d,%d,%d>,\n", w[0], w[1], w[2]);
						}
					}
				}
			}
			fprintf(f, "}}\n");
			
			fprintf(f, "\n\n object { Mesh_%c texture { pigment { image_map { png \"whitefloor.png\" interpolate 4 } } finish { ambient 0.1 diffuse 1.0 specular 0.3 }}}\n", 'A'+k);
			
		}
		
		
		for (int i = 0; i < 2; i++) {
			delete [] norms[i];
		}
		
		fclose(f);
	}
	
	void finalize(void)
	{
		printf("\n"); fflush(stdout);
		blur_array_2d(down, n, 3);
		blur_array_2d(up  , n, 3);
		
		write_map(vox[0].input_texture, down, "data/rad_down.bmp");
		write_map(vox[1].input_texture, up,   "data/rad_up.bmp"  );
		
		//output_pov("/tmp/cave.pov");
	}
	
	~RadiosityCalculation()
	{
		delete [] col_journals;
		delete [] vec_journals;
		delete [] down;
		delete [] up;
		delete [] normals[0];
		delete [] normals[1];
		delete [] barriers;
		for (int i = 0; i < 2; i++)
			for (int j = 0; j < 2; j++) {
				delete [] rmaps[i][j];
				delete [] vmaps[i][j];
			}
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
