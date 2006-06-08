/***************************************************************************
 *   Copyright (C) 2004 by Veselin Georgiev                                *
 *   vesko@ViruS                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "shadows.h"
#include "MyGlobal.h"
#include "cross_vars.h"
#include "profile.h"
#include "gfx.h"
#include "vectormath.h"
#include "mesh.h"
#include <stdlib.h>
#include "array.h"
#include "vector2f.h"

#define shadows_related
#include "x86_asm.h"
#undef shadows_related

double light_radius = 7.5f;
ShadowCaster* occluders[2000];
const int max_neighs = 7;


static float rowstart[RES_MAXY][2], rowincrease[RES_MAXY][2];
static int ceilmax, floormin; // ending row for ceiling, starting row for floor in the image


static inline float face(const vec2f& a, const vec2f& b, const vec2f& c) 
{
	return +a[0] * b[1] + b[0] * c[1] + c[0] * a[1]
	       -c[0] * b[1] - b[0] * a[1] - a[0] * c[1];
}

static void shadows_precalc(int xr, int yr, Vector & mtt, Vector& mti, Vector& mtti)
{
	prof_enter(PROF_SHADOW_PRECALC);
	for (int j = 0; j < yr; j++) {
		double y = mtt[1] + j*mtti[1] - cur[1];
		if (y > 0.0 && fabs(y) > DST_THRESHOLD) ceilmax = j;
		if (floormin == -1 && y < 0.0) floormin = j;
		double planeDist = (y<0.0)?(cur[1] - daFloor):(daCeiling - cur[1]);
		double m = planeDist / fabs(y);
		for (int i=0;i<2;i++) {
			rowstart   [j][i] = cur[i*2] + (mtt[i*2]+mtti[i*2]*j-cur[i*2])*m;
			rowincrease[j][i] = mti[i*2] * m;
		}
	}
	prof_leave(PROF_SHADOW_PRECALC);
}


static void shadows_merge(int xr, int yr, Uint32 *target_framebuffer, Uint16 *sbuffer)
{
	prof_enter(PROF_MERGE);
	if (mmx2_enabled)
		shadows_merge_mmx2(target_framebuffer, sbuffer, xr*yr);
	else {
		xr*=yr;
		for (int i=0;i<xr;i++)
			target_framebuffer[i] =
					(((target_framebuffer[i] & 0xff    ) * (255-(255&sbuffer[i])) >> 8)           ) +
					(((target_framebuffer[i] & 0xff00  ) * (255-(255&sbuffer[i])) >> 8) & 0xff00  ) +
					(((target_framebuffer[i] & 0xff0000) * (255-(255&sbuffer[i])) >> 8) & 0xff0000);
	}
	prof_leave(PROF_MERGE);
}

class bary_t {
	float s[2], u[2], v[2], d;
public:
	bary_t(){}
	bary_t(float a[], float b[], float c[])
	{
		s[0] = a[0];
		s[1] = a[1];
		u[0] = b[0] - a[0];
		u[1] = b[1] - a[1];
		v[0] = c[0] - a[0];
		v[1] = c[1] - a[1];
		d = 1.0f / (u[0] * v[1] - u[1] * v[0]);
	}
	void eval(float x, float y, float res[])
	{
		x -= s[0];
		y -= s[1];
		res[0] = d * (x * v[1] - y * v[0]);
		res[1] = d * (y * u[0] - x * u[1]);
		res[2] = 1.0f - res[0] - res[1];
	}
};


#undef min
#undef max
#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))

#define CLAMP(x,a,b) (x=x>a?(x<b?x:b):a)

static int all_min_x, all_min_y, all_max_x, all_max_y;

struct WedgeDrawer : public AbstractDrawer {
	bary_t bary;
	Uint16 *sbuffer;
	int w1, w2, w3, xr;

	inline bool draw_line(int xs, int y, int size)
	{
		Uint16 * buff = &sbuffer[y*xr];
		float h[3];
		for (int i = xs; i < xs + size; i++) {
			bary.eval((float) i, (float) y, h);
			if (h[0] >= 0.0f && h[0] <= 1.0f && h[1] >= 0.0f && h[1] <= 1.0f && h[2] >= 0.0f && h[2] <= 1.0f) 
			{
				//
				int value = (int) (h[0] * w1 + h[1] * w2 + h[2] * w3);
				buff[i] = (buff[i] & 0xff) | (value << 8);
			}

		}
		return true;
	}
};

static void raster_wedge(float aa[], float bb[], float cc[], int w1, int w2, int w3, int xr, int yr, Uint16 *sbuffer)
{
	vec2f a(aa), b(bb), c(cc);
	WedgeDrawer wd;
	wd.bary = bary_t(aa, bb, cc);
	wd.xr = xr;
	wd.w1 = w1; wd.w2 = w2; wd.w3 = w3; wd.sbuffer = sbuffer;

	TriangleRasterizer rasterizer(xr, yr, a, b, c);
	rasterizer.draw(wd);

	rasterizer.update_limits(all_min_x, all_max_x, all_min_y, all_max_y);
}

static void intersect(float a[], float b[], float c[], float d[], float x[])
{
	float h[2] = { c[0] - a[0], c[1] - a[1] };
	float v[2] = { b[0] - a[0], b[1] - a[1] };
	float u[2] = { c[0] - d[0], c[1] - d[1] };
	//
	float down = (v[0] * u[1] - v[1] * u[0]);
	if (fabs(down) < 1e-10) {
		x[0] = a[0];
		x[1] = a[1];
		return;
	}
	float D = 1.0f / down;
	float p = D * (h[0] * u[1] - h[1] * u[0]);
	if (p < -1) p = -1;
	if (p > +2) p = 2;
	x[0] = a[0] + v[0] * p;
	x[1] = a[1] + v[1] * p;
}

struct SolidDrawer : public AbstractDrawer 
{
	Uint16 *buff, fill;
	int xr;
	SolidDrawer(Uint16 *sbuffer, Uint16 _fillcolor, int _xr) : buff(sbuffer), fill(_fillcolor), xr(_xr) {}
	inline bool draw_line(int x, int y, int size)
	{
		Uint16 *p = &buff[y * xr + x];
		for (int i = 0; i < size; i++)
			p[i] |= fill;
		return true;
	}
};

static void display_poly(
		Vector poly[],
		int n,
		const Vector &ml,
		int xr, int yr,
		Vector &cur,
		Vector &mtt,
		Vector &mti,
		Vector & mtti,
		Uint16 *sbuffer
		)
{
	static Vector casted[MAX_SIDES + 1];
	static float scr_coords[MAX_SIDES + 1][2];
	int c = 0;
	for (int j = 0; j < n + 1; j++) {
		bool useless; int res;
		res = ProjectPointShadow(poly[j], ml, scr_coords[j], xr, yr, cur, mtt, mti, mtti, useless, casted[j]);
		c += (useless == true);
		if (res >= 4) c += 1000;
	}
	if (c != 0 && c != n + 1) return;

	all_min_x = all_min_y = inf;
	all_max_x = all_max_y = -inf;
		
	/* Stage 1: Render hard shadows */
	static vec2f cs1[MAX_SIDES], cs2[MAX_SIDES];
	int cs1a, cs2a;
	cs1a = n; memcpy(cs1, scr_coords, n * sizeof(cs1[0]));
	while (cs1a > 2) {
		cs2a = 0;
		int i = 0;
		while (i < cs1a - 1) {
			if (face(cs1[i], cs1[i+1], cs1[(i+2)%cs1a]) < +1e-6) {
				//draw it
				SolidDrawer sd(sbuffer, shadow_intensity << 8, xr);
				TriangleRasterizer ras(xr, yr, cs1[i], cs1[i+1], cs1[(i+2)%cs1a]);
				ras.draw(sd);
				ras.update_limits(all_min_x, all_max_x, all_min_y, all_max_y);
				//discard the second point
				cs2[cs2a++] = cs1[i];
				i += 2;
			} else {
				cs2[cs2a++] = cs1[i];
				i++;
			}
		}
		if (i == cs1a - 1) cs2[cs2a++] = cs1[i];

		if (cs2a == cs1a) break;
		cs1a = cs2a;
		memcpy(cs1, cs2, cs1a * sizeof(cs1[0]));
	}

		
		
		
	//OutLineToScreen(sbuffer, casted, n, 0x8086, cur, w, xr, yr);
	//MapToScreen(sbuffer, casted, n, shadow_intensity << 8, cur, w, xr, yr);
	
	/* Recalc and splat wedges */

#ifndef DONT_DRAW_WEDGES
	static Vector wed[MAX_SIDES][2][2];
	static float fed[MAX_SIDES][2][2][2];
	static bool xvalid[MAX_SIDES], valid[MAX_SIDES];
	for (int j = 0; j < n; j++) {
		Vector a = ml - poly[j+1];
		Vector b = ml - poly[j];
		Vector n = a ^ b;
		n.make_length( light_radius );
		int res[2];
		bool u1, u2;
		res[0] = ProjectPointShadow(poly[j], ml + n, fed[j][0][0], xr, yr, cur, mtt, mti, mtti, u1, wed[j][0][0]);
		res[1] = ProjectPointShadow(poly[j], ml - n, fed[j][0][1], xr, yr, cur, mtt, mti, mtti, u2, wed[j][0][1]);
		xvalid[j] = (res[0] < 4 && res[1] < 4 /*&& u1 == u2*/);
		res[0] = ProjectPointShadow(poly[j+1], ml + n, fed[j][1][0], xr, yr, cur, mtt, mti, mtti, u1, wed[j][1][0]);
		res[1] = ProjectPointShadow(poly[j+1], ml - n, fed[j][1][1], xr, yr, cur, mtt, mti, mtti, u2, wed[j][1][1]);
		xvalid[j] &= (res[0] < 4 && res[1] < 4 /*&& u1 == u2*/);
	}
	for (int j = 0; j < n; j++) {
		valid[j] = xvalid[(j-1+n) % n] && xvalid[j] && xvalid[(j+1)%n];
		if (!valid[j]) continue;
		float f[2][2][2];
		for (int k = -1; k <= 1; k+=2)
			for (int l = 0; l < 2; l++) 
				intersect(fed[j][1][l], fed[j][0][l], fed[(j+k+n)%n][1][l], fed[(j+k+n)%n][0][l], f[(k+1)/2][l]);
		raster_wedge(f[0][1], f[0][0], f[1][0], shadow_intensity, shadow_intensity, 0, xr, yr, sbuffer);
		raster_wedge(f[1][1], f[1][0], f[0][1], shadow_intensity, 0,                0, xr, yr, sbuffer);
	}
#endif
	/* stage 3: reblend */
	
	CLAMP(all_min_x, 0, xr-1);	
	CLAMP(all_max_x, 0, xr-1);	
	CLAMP(all_min_y, 0, yr-1);	
	CLAMP(all_max_y, 0, yr-1);	
	for (int j = all_min_y; j <= all_max_y; j++) {
		Uint16 *buff = &sbuffer[j * xr];
		for (int i = all_min_x; i <= all_max_x; i++) {
			Uint16 t = buff[i];
			t = (t & 0xff) + (t >> 8);
			if (t > shadow_intensity) t = shadow_intensity;
			buff[i] = t; 
		}
	}
	
}

struct DensityDrawer : public AbstractDrawer
{
	Uint16 *buff, fill;
	sphere *S;
	Vector l;
	int xr;
	DensityDrawer(Uint16 *sbuffer, Uint16 _fillc, int _xr, sphere *ss, Vector ll) : 
			buff(sbuffer), fill(_fillc), S(ss), l(ll), xr(_xr) {}
	
	inline bool computeminmax(const Vector& in, float &f1, float &f2)
	{
		Vector f;
		f.make_vector(in, S->pos);
		Vector v = l - in;
		float ua, ub, uc, Dt;

		ua = v.lengthSqr();
		ub = f*v*2.0;
		uc = f.lengthSqr() - sqr(S->d);
		Dt = sqr(ub) - 4.0f*ua*uc;
		if (Dt < 0) return false;
		float div_er = 1.0f / (2.0f * ua);
		Dt = fast_sqrt(Dt);
		f1 = div_er * (-ub - Dt);
		f2 = div_er * (-ub + Dt);
		if (f1 > f2) {
			float t = f1;
			f1 = f2;
			f2 = t;
		}
		return true;
	}
	
	inline bool draw_line(int x, int y, int size)
	{
		Uint16 *p = &buff[y * xr + x];
		Vector v(
				rowstart[y][0] + x * rowincrease[y][0],
				y > floormin ? daFloor : daCeiling,
				rowstart[y][1] + x * rowincrease[y][1]);
		//for (int i = 0; i < size; i++)
		//	p[i] |= fill;
		for (int i = 0; i < size; i++) {
			float f1, f2;
			if (computeminmax(v, f1, f2)) {
				float f = (f2-f1)/(0.01f+f1);
				if (f > 1.0f) {
					f = 1.0f;
				}
				p[i] |= ((Uint16)(fill*f)) << 8;
			} 
			//
			v.v[0] += rowincrease[y][0];
			v.v[2] += rowincrease[y][1];
		}
		return true;
	}
};


static void display_poly_fake_sphere(
		Vector poly[],
		int n,
		const Vector &ml,
		int xr, int yr,
		Vector &cur,
		Vector &mtt,
		Vector &mti,
		Vector & mtti,
		Uint16 *sbuffer,
		sphere *S
			)
{
	static Vector casted[MAX_SIDES + 1];
	static float scr_coords[MAX_SIDES + 1][2];
	int c = 0;
	for (int j = 0; j < n + 1; j++) {
		bool useless; int res;
		res = ProjectPointShadow(poly[j], ml, scr_coords[j], xr, yr, cur, mtt, mti, mtti, useless, casted[j]);
		c += (useless == true);
		if (res >= 4) c += 1000;
	}
	if (c != 0 && c != n + 1) return;

	all_min_x = all_min_y = inf;
	all_max_x = all_max_y = -inf;
		
	/* Stage 1: Render hard shadows */
	static vec2f cs1[MAX_SIDES], cs2[MAX_SIDES];
	int cs1a, cs2a;
	cs1a = n; memcpy(cs1, scr_coords, n * sizeof(cs1[0]));
	while (cs1a > 2) {
		cs2a = 0;
		int i = 0;
		while (i < cs1a - 1) {
			if (face(cs1[i], cs1[i+1], cs1[(i+2)%cs1a]) < +1e-6) {
				//draw it
				DensityDrawer dd(sbuffer, shadow_intensity, xr, S, ml);
				TriangleRasterizer ras(xr, yr, cs1[i], cs1[i+1], cs1[(i+2)%cs1a]);
				ras.draw(dd);
				ras.update_limits(all_min_x, all_max_x, all_min_y, all_max_y);
				//discard the second point
				cs2[cs2a++] = cs1[i];
				i += 2;
			} else {
				cs2[cs2a++] = cs1[i];
				i++;
			}
		}
		if (i == cs1a - 1) cs2[cs2a++] = cs1[i];

		if (cs2a == cs1a) break;
		cs1a = cs2a;
		memcpy(cs1, cs2, cs1a * sizeof(cs1[0]));
	}
	//
	/* stage 3: reblend */
	
	CLAMP(all_min_x, 0, xr-1);	
	CLAMP(all_max_x, 0, xr-1);	
	CLAMP(all_min_y, 0, yr-1);	
	CLAMP(all_max_y, 0, yr-1);	
	
	if (all_min_x == all_max_x || all_min_y == all_max_y) return;
	
	for (int j = all_min_y; j <= all_max_y; j++) {
		Uint16 *buff = &sbuffer[j * xr];
		for (int i = all_min_x; i <= all_max_x; i++) {
			Uint16 t = buff[i];
			t = (t & 0xff) + (t >> 8);
			if (t > shadow_intensity) t = shadow_intensity;
			buff[i] = t; 
		}
	}
	
}


struct Vertex {
	int no;
	Vector v;
	Vertex() {}
	Vertex(int _no, const Vector & _v):no(_no), v(_v) {}
};

struct PolyInfo {
	int start, size;
};

int compar_vertex(const void *a, const void *b)
{
	return static_cast<const Vertex*>(a)->no - static_cast<const Vertex*>(b)->no;
}

static int recu_es;
static Mesh::EdgeInfo recu_edges[MAX_SIDES];
static Vector mesh_poly[MAX_SIDES];

static bool ps[MAX_SIDES];
static int maxdist, maxvert;

void dfs(int node, int g[][max_neighs + 1], int lev)
{
	if (lev > maxdist) {
		maxdist = lev;
		maxvert = node;
	}
	ps[node] = true;
	for (int i = 1; i <= g[node][0]; i++)
		if (!ps[g[node][i]]) 
			dfs(g[node][i], g, lev + 1);
}

int find_most_distant(int g[][max_neighs + 1], int root, int n)
{
	memset(ps, 0, sizeof(bool) * n);
	maxdist = -1;
	dfs(root, g, 0);
	return maxvert;
}

static int temp_path[MAX_SIDES];
static int final_path[MAX_SIDES];
static int path_length;

void record_path(int node, int dest, int g[][max_neighs + 1], bool ps[], int lev) 
{
	temp_path[lev] = node;
	ps[node] = true;
	if (node == dest) {
		memcpy(final_path, temp_path, (lev+1) * sizeof(int));
		path_length = lev+1;
		return;
	}
	
	for (int i = 1; i <= g[node][0]; i++) {
		if (!ps[g[node][i]]) {
			record_path(g[node][i], dest, g, ps, lev+1);
		}
	}
}

int connect_graph(Mesh::EdgeInfo e[], int m)
{
	static bool visited[MAX_SIDES];
	static Vertex verts[MAX_SIDES];
	static int fwdmap[MAX_SIDES];
	static int g[MAX_SIDES][max_neighs + 1];
	int n = 0;
	memset(visited, 0, sizeof(visited));
	
	for (int i = 0; i < m; i++) {
		if (!visited[e[i].ai]) {
			visited[e[i].ai] = true;
			verts[n++] = Vertex(e[i].ai, e[i].a);
		}
		if (!visited[e[i].bi]) {
			visited[e[i].bi] = true;
			verts[n++] = Vertex(e[i].bi, e[i].b);
		}
	}
	qsort(verts, n, sizeof(Vertex), compar_vertex);
	for (int i = 0; i < n; i++)
		fwdmap[verts[i].no] = i;
	memset(g, 0, sizeof(int) * (max_neighs + 1) * n);
	for (int i = 0; i < m; i++) {
		int x = fwdmap[e[i].ai];
		int y = fwdmap[e[i].bi];
		if (g[x][0] == max_neighs || g[y][0] == max_neighs) {
#ifdef DEBUG
			printf("Graph representation overflow\n");
#endif
			continue;
		}
		g[x][++g[x][0]] = y;
		g[y][++g[y][0]] = x;
	}
	memset(visited, 0, sizeof(bool) * n);
	Array< Array <int> > compos;
	for (int i = 0; i < n; i++) if (!visited[i]) {
		int x = find_most_distant(g, i, n);
		int y = find_most_distant(g, x, n);
		memset(ps, 0, sizeof(bool) * n);
		record_path(x, y, g, ps, 0);
		Array<int> compo;
		for (int j = 0; j < path_length; j++) {
			compo += final_path[j];
			visited[final_path[j]] = true;
		}
		compos += compo;
	}
	//
	/*
	printf("%d components: \n", compos.size());
	for (int i = 0; i < compos.size(); i++) {
		printf("{ ");
		for (int j = 0; j < compos[i].size(); j ++)
			printf("%d ", compos[i][j]);
		printf("}\n");
	}
	*/
	
	Array<int> all;
	all.append(compos[0]);
	bool *used = (bool*)alloca(sizeof(bool) * compos.size());
	memset(used, 0, sizeof(bool) * compos.size());
	used[0] = true;
	for (int k = 1; k < compos.size(); k++) {
		int bi = -1;
		bool reversed = false;
		double mdist = 1e99;
		Vector & end = verts[all[all.size()-1]].v;
		for (int i = 0; i < compos.size(); i++) if (!used[i]) {
			double t = end.distto( verts[compos[i][0]].v );
			if (t < mdist) {
				bi = i;
				mdist = t;
				reversed = false;
			}
			t = end.distto( verts[ compos[i][compos[i].size()-1] ].v );
			if (t < mdist) {
				bi = i;
				mdist = t;
				reversed = true;
			}
		}
		used[bi] = true;
		if (reversed) compos[bi].reverse();
		all.append(compos[bi]);
	}
	int r = 0;
	for (int i = 0; i < all.size(); i++) {
		if (r == 0 || mesh_poly[r-1].distto(verts[all[i]].v) > 1e-9)
			mesh_poly[r++] = verts[all[i]].v;
	}
	if (mesh_poly[r-1].distto(mesh_poly[0]) < 1e-9) r--;
	return r;
}

static inline Vector plane_cast(const Vector & cur, const Vector & v)
{
	double dy = v.v[1] - cur.v[1];
	double fy = daCeiling;
	if (dy < 0.0) {
		fy = daFloor;
		dy = -dy;
	}
	
	Vector dir;
	dir.make_vector(v, cur);
	dir.scale(fabs(fy - cur[1]) / dy);
	dir += cur;
	dir.v[1] = fy;
	return dir;
}

static inline double evaluate(const Vector & xc, const Vector & p, double addage)
{
	return addage + xc * p;
}

static void reversed_check(Vector verts[], const PolyInfo &pi, const Vector & l)
{
	static float casted[MAX_SIDES][2];
	
	int n = pi.size;
	for (int i = 0; i < n; i++) {
		Vector t = plane_cast(l, verts[pi.start + i]);
		casted[i][0] = (float) t.v[0];
		casted[i][1] = (float) t.v[2];
	}
#define fdist(a,b) (sqrt((a[0]-b[0])*(a[0]-b[0]) + (a[1]-b[1])*(a[1]-b[1])))
	float total_angle = 0;
	for (int i = 1; i <= n; i++) {
		float a = fdist(casted[i-1], casted[i%n]);
		float b = fdist(casted[(i+1)%n], casted[i%n]);
		float c = fdist(casted[i-1], casted[(i+1)%n]);
		total_angle += copysign(M_PI - acos((a*a+b*b-c*c)/(2*a*b)), face(casted[i-1], casted[i%n], casted[(i+1)%n]));
	}
	float mul = 1.0;
	if (verts[pi.start][1] < l[1]) mul = -1.0;
	if (mul*total_angle > -1e-6*mul) {
		int i = pi.start;
		int j = pi.start + pi.size - 1;
		while (i < j) {
			Vector t = verts[i];
			verts[i] = verts[j];
			verts[j] = t;
			i++; j--;
		}
	}
}

static void thresh_light(Vector verts[], int n, Vector l)
{
	for (int i = 0; i < n; i++) {
		if (fabs(verts[i][1] - l[1]) < DST_THRESHOLD) {
			if (verts[i][1] <= l[1])
				verts[i][1] = l[1] - DST_THRESHOLD;
			else
				verts[i][1] = l[1] + DST_THRESHOLD;
		}
	}
}

int rearrange_poly(Vector verts[], int n, Vector l, Vector cur, double alpha, double beta, PolyInfo o[])
{
	Array<Vector> up, down;
	//
	Vector pvec(sin(alpha) * cos(beta), sin(beta), cos(alpha) * cos(beta));
	pvec.norm();
	double D = - evaluate(pvec, cur, 0.0);
	//
	thresh_light(verts, n, l);
	for (int i = 0; i < n; i++) {
		int j = (i + 1) % n;
		Vector va = verts[i];
		Vector vb = verts[j];
		Vector a = plane_cast(l, va);
		Vector b = plane_cast(l, vb);
		bool afloor = a.v[1] == (double) daFloor;
		double ad = evaluate(pvec, a, D);
		double bd = evaluate(pvec, b, D);
		bool aok = ad > VALIDITY_THRESH;
		bool bok = bd > VALIDITY_THRESH;
		if (!(aok || bok)) continue;
		if (aok ^ bok) {
			// find valid, using binsearch
			double left = 0.0, right = 1.0, mid;
			Vector dv; dv.make_vector(vb, va);
			while (right - left > 1e-6) {
				mid = (left + right) *0.5;
				Vector temp;
				temp.macc(va, dv, mid);
				double td = evaluate(pvec, plane_cast(l, temp), D);
				if ((aok && td < VALIDITY_THRESH) || (!aok && td > VALIDITY_THRESH))
					right = mid;
				else
					left = mid;
			}
			Vector good;
			good.macc(va, dv, mid);
			if (aok) vb = good;
			else     va = good;
		}
		if ((va[1] - l[1])*(vb[1] - l[1]) < 0) {
			double t = (l[1] - va[1])/(vb[1] - va[1]);
			Vector dv; dv.make_vector(vb, va);
			Vector c;
			c.macc(va, dv, t);
			//c.v[1] -= DST_THRESHOLD;
			if (afloor) {
				down += va;
				c.v[1] -= DST_THRESHOLD;
				down += c;
				c.v[1] += 2.0* DST_THRESHOLD;
				up += c;
				up += vb;
			} else {
				up += va;
				c.v[1] += DST_THRESHOLD;
				up += c;
				c.v[1] -= 2.0* DST_THRESHOLD;
				down += c;
				down += vb;
			}
		} else {
			if (afloor) {
				down += va;
				down += vb;
			} else {
				up += va;
				up += vb;
			}
		}
	}
	
	//
	PolyInfo pi;
	pi.size = -2;
	
	int os = 0;
	if (up.size()) {
		pi.start = 0;
		pi.size = 0;
		for (int i = 0; i < up.size(); i++) {
			if (i == 0 || up[i] != verts[pi.size-1])
				verts[pi.size++] = up[i];
		}
		if (pi.size > 1 && verts[pi.size-1] == verts[0]) pi.size--;
		verts[pi.size] = verts[0];
		verts[pi.size + 1] = verts[1];
		o[os++] = pi;
	}
	if (down.size()) {
		pi.start = pi.size + 2;
		pi.size = 0;
		for (int i = 0; i < down.size(); i++) {
			if (i == 0 || down[i] != verts[pi.start + pi.size - 1])
				verts[pi.start + pi.size++] = down[i];
		}
		if (pi.size > 1 && verts[pi.start + pi.size - 1] == verts[pi.start]) pi.size--;
		verts[pi.start + pi.size] = verts[pi.start];
		verts[pi.start + pi.size + 1] = verts[pi.start + 1];
		o[os++] = pi;
	}
	for (int i = 0; i < os; i++) {
		reversed_check(verts, o[i], l);
	}
	return os;
}

static void expand_poly(Vector verts[], int n, Vector center, Vector l)
{
	thresh_light(verts, n, l);
	for (int i = 0; i < n; i++)
	{
		Vector p = plane_cast(l, verts[i]);
		double coeff = p.distto(verts[i]) / p.distto(l);
		Vector v = verts[i] - center;
		v.norm();
		v.scale(coeff * light_radius);
		verts[i] += v;
	}
	thresh_light(verts, n, l);
}

void render_shadows_real(Uint32 *target_framebuffer, Uint16 *sbuffer, int xr, int yr, Vector& mtt, Vector& mti, Vector& mtti)
{
	Vector ml(lx, ly, lz);
	ceilmax = floormin = -1;

	prof_enter(PROF_ZERO_SBUFFER); // reset the S-buffer;
	memset(sbuffer, 0, xr*yr*sizeof(Uint16));
	prof_leave(PROF_ZERO_SBUFFER);
	
	shadows_precalc(xr, yr, mtt, mti, mtti);
	
	for (int i = 0; i < spherecount; i++) if (sp[i].flags & CASTS_SHADOW) {
		Vector poly[SPHERE_SIDES*2+8];
		
		prof_enter(PROF_POLY_GEN);
		sp[i].CalculateFixedConvex(poly, ml, SPHERE_SIDES);
		prof_leave(PROF_POLY_GEN);
		PolyInfo pif[2];
		
		prof_enter(PROF_POLY_REARRANGE);
		int pir = rearrange_poly(poly, SPHERE_SIDES, ml, cur, alpha, beta, pif);
		prof_leave(PROF_POLY_REARRANGE);

		prof_enter(PROF_POLY_DISPLAY);
		for (int i = 0; i < pir; i++) {
			display_poly(poly + pif[i].start, pif[i].size, ml, xr, yr, cur, mtt, mti, mtti, sbuffer);
		}
		prof_leave(PROF_POLY_DISPLAY);
	}
	
	
	for (int i = 0; i < mesh_count; i++) if (mesh[i].GetFlags() & CASTS_SHADOW) {
		recu_es = 0;
		prof_enter(PROF_CONNECT_GRAPH);
		for (int j = 0; j < mesh[i].num_edges; j++)
		{
			Mesh::EdgeInfo &e = mesh[i].edges[j];
			Vector p = (e.a + e.b) * 0.5 - ml;
			if ((p * e.norm1) * (p * e.norm2) < 0.0)
				recu_edges[recu_es++] = e;
#ifdef DEBUG
			if (fabs(p * e.norm1) < 1e-8) {
				printf("Warning: zero at edge %d\n", j);
				printf("Normal is "); e.norm1.print();
			}
			if (fabs(p * e.norm2) < 1e-8) {
				printf("Warning: zero at edge %d\n", j);
				printf("Normal is "); e.norm2.print();
			}
#endif
		}
		int r = connect_graph(recu_edges, recu_es);
		prof_leave(PROF_CONNECT_GRAPH);
	
		PolyInfo pif[2];
		prof_enter(PROF_POLY_REARRANGE);
		int pir = rearrange_poly(mesh_poly, r, ml, cur, alpha, beta, pif);
		prof_leave(PROF_POLY_REARRANGE);
		prof_enter(PROF_POLY_DISPLAY);
		for (int i = 0; i < pir; i++) {
			display_poly(mesh_poly + pif[i].start, pif[i].size, ml, xr, yr, cur, mtt, mti, mtti, sbuffer);
		}
		prof_leave(PROF_POLY_DISPLAY);
	}
	shadows_merge(xr, yr, target_framebuffer, sbuffer);
}

void render_shadows_fake(Uint32 *target_framebuffer, Uint16 *sbuffer, int xr, int yr, Vector& mtt, Vector& mti, Vector& mtti)
{
	Vector ml(lx, ly, lz);
	ceilmax = floormin = -1;

	prof_enter(PROF_ZERO_SBUFFER); // reset the S-buffer;
	memset(sbuffer, 0, xr*yr*sizeof(Uint16));
	prof_leave(PROF_ZERO_SBUFFER);
	
	shadows_precalc(xr, yr, mtt, mti, mtti);
	
	for (int i = 0; i < spherecount; i++) if (sp[i].flags & CASTS_SHADOW) {
		Vector poly[SPHERE_SIDES*2+8];
		
		prof_enter(PROF_POLY_GEN);
		sp[i].CalculateFixedConvex(poly, ml, SPHERE_SIDES);
		prof_leave(PROF_POLY_GEN);
		PolyInfo pif[2];
		
		expand_poly(poly, SPHERE_SIDES, sp[i].pos, ml);
		
		prof_enter(PROF_POLY_REARRANGE);
		int pir = rearrange_poly(poly, SPHERE_SIDES, ml, cur, alpha, beta, pif);
		prof_leave(PROF_POLY_REARRANGE);

		prof_enter(PROF_POLY_DISPLAY);
		for (int i = 0; i < pir; i++) {
			//display_poly(poly + pif[i].start, pif[i].size, ml, xr, yr, cur, mtt, mti, mtti, sbuffer);
			display_poly_fake_sphere(poly + pif[i].start, pif[i].size,
					ml, xr, yr, cur, mtt, mti, mtti, sbuffer, &sp[i]);
		}
		prof_leave(PROF_POLY_DISPLAY);

	}
	shadows_merge(xr, yr, target_framebuffer, sbuffer);
}

void render_shadows(Uint32 *target_framebuffer, Uint16 *sbuffer, int xr, int yr, Vector& mtt, Vector& mti, Vector& mtti)
{
	render_shadows_fake(target_framebuffer, sbuffer, xr, yr, mtt, mti, mtti);
}

static int occlusions(const sphere & a, const sphere & b, const Vector & light)
{
	char temp[128];
	int c = 0;
	
	Vector st = a.pos - Vector(a.d, a.d, a.d);
	for (int i = 0; i < 8; i++) {
		Vector v = st;
		if (i & 1) v += Vector(a.d, 0.0, 0.0) * 2.0;
		if (i & 2) v += Vector(0.0, a.d, 0.0) * 2.0;
		if (i & 4) v += Vector(0.0, 0.0, a.d) * 2.0;
		Vector ray;
		ray.make_vector(light, v);
		if (b.FastIntersect( ray, v, ray.lengthSqr(), temp)) c++;
	}
	return c;
}

static int occlusions(const sphere &a, Mesh & m, const Vector & light, bool full_intersections)
{	
	int c = 0;

	Vector st = a.pos - Vector(a.d, a.d, a.d);
	for (int i = 0; i < 8; i++) {
		Vector v = st;
		if (i & 1) v += Vector(a.d, 0.0, 0.0) * 2.0;
		if (i & 2) v += Vector(0.0, a.d, 0.0) * 2.0;
		if (i & 4) v += Vector(0.0, 0.0, a.d) * 2.0;
		Vector ray;
		ray.make_vector(light, v);
		ray.norm();
		if (full_intersections) {
			if (m.FullIntersect(v, ray)) c++;
		} else {
			if (m.TestIntersect(v, ray)) c++;
		}
	}
	return c;
}


void check_for_shadowed_spheres(void)
{
	Vector ml(lx, ly, lz);
	Array<ShadowCaster*> all;
	
	for (int i = 0; i < spherecount; i++) {
		sphere & s = sp[i];
		s.flags &= ~SHADOWED;
		s.flags &= ~PART_SHADOWED;
		
		for (int j = 0; j < spherecount; j++) if (i != j && (sp[j].flags & CASTS_SHADOW)) {
			if (8 == occlusions(s, sp[j], ml))
			{
				s.flags |= SHADOWED;
				break;
			}
		}
		if (s.flags & SHADOWED) continue;
		for (int j = 0; j < mesh_count; j++) 
			if ((mesh[j].GetFlags() & CASTS_SHADOW) && 8 == occlusions(s, mesh[j], ml, true)) {
				s.flags |= SHADOWED;
				break;
			}
		if (s.flags & SHADOWED) continue;
		
		int csize = 0;
		s.occluders_ind = all.size();
		double LCd = s.pos.distto(ml);
		Vector LC; LC.make_vector(s.pos, ml); LC.norm();
		double LCa = atan(s.d / LCd);
		for (int j = 0; j < spherecount; j++) if (i != j && (sp[j].flags & CASTS_SHADOW)) {

			if (occlusions(s, sp[j], ml)) {
				s.flags |= PART_SHADOWED;
				all += &sp[j];
				csize ++;
			} else {
				Vector LX; LX.make_vector(sp[j].pos, ml);
				double LXd = LX.length();
				if (LXd < LCd) {
					LX.norm();
					double ang = acos(LX*LC);
					if (ang < LCa + atan(sp[j].d / LXd)) {
						s.flags |= PART_SHADOWED;
						all += &sp[j];
						csize++;
					}
				}
			}
		}
		
		for (int j = 0; j < mesh_count; j++) 
			if ((mesh[j].GetFlags() & CASTS_SHADOW) && occlusions(s, mesh[j], ml, false)) {
				s.flags |= PART_SHADOWED;
				all += &mesh[j];
				csize++;
			}
		s.occluders_size = csize;
	}
	if (all.size() > 2000) printf("Too much occluders!!!\n");
	for (int i = 0; i < all.size(); i++)
		occluders[i] = all[i];
}

void shadows_close(void)
{
}
