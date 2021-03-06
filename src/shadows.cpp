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
#include "cpu.h"
#include "cvars.h"
#include "common.h"
#include "profile.h"
#include "gfx.h"
#include "vectormath.h"
#include "mesh.h"
#include "light.h"
#include <stdlib.h>
#include "array.h"
#include "vector2f.h"
#ifdef _WIN32
#include <malloc.h> // for alloca
#endif


#define shadows_related
#include "x86_asm.h"
#undef shadows_related

double light_radius = 7.5f;
int g_biasmethod = 1;
ShadowCaster* occluders[10000];
const int max_neighs = 7;

const double YOFFSET = 0.125;


static float rowstart[RES_MAXY][2], rowincrease[RES_MAXY][2];
static int ceilmax, floormin; // ending row for ceiling, starting row for floor in the image
static InterlockedInt rowint;

static void shadows_precalc(int xr, int yr, const Vector & mtt, const Vector& mti, const Vector& mtti)
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


void shadows_merge(int xr, int yr, Uint32 *target_framebuffer, Uint16 *sbuffer)
{
	prof_enter(PROF_MERGE);
	if (cpu.mmx2)
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

static bool in_shadow(Mesh::EdgeInfo & e, Mesh & m, const Vector &light)
{
	int base = m.get_triangle_base();
	char ic[128];
	for (int i = 0; i < m.triangle_count; i++) {
		Triangle &t = trio[i + base];
		
		bool same_tri = false;
		for (int j = 0; j < 3; j++)
			if (t.vertex[j] == e.a || t.vertex[j] == e.b) {
				same_tri = true;
				break;
			}
		if (same_tri) continue;
		
		Vector ray;
		ray.make_vector(e.a, light);
		if (t.intersect(ray, light, ic)) return true;
		ray.make_vector(e.b, light);
		if (t.intersect(ray, light, ic)) return true;
		
	}
	return false;
}

class bary_t {
	float s[2], u[2], v[2], d;
	float inc[3];
public:
	float state[3];
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
		inc[1] = d * (+1.0 * v[1]);
		inc[2] = d * (-1.0 * u[1]);
		inc[0] = -inc[1] - inc[2];
	}
	void eval(float x, float y, float res[])
	{
		x -= s[0];
		y -= s[1];
		res[1] = d * (x * v[1] - y * v[0]);
		res[2] = d * (y * u[0] - x * u[1]);
		res[0] = 1.0f - res[1] - res[2];
	}
	void eval_first(float x, float y)
	{
		eval(x, y, state);
	}
	inline void eval_next(void)
	{
		state[0] += inc[0];
		state[1] += inc[1];
		state[2] += inc[2];
	}
};


#undef min
#undef max
#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))

#define CLAMP(x,a,b) (x=x>a?(x<b?x:b):a)

struct Box2D {
	int min_x, min_y, max_x, max_y;
	
	Box2D() {
		min_x = min_y = inf;
		max_x = max_y = -inf;
	}
};

struct WedgeDrawer : public AbstractDrawer {
	bary_t bary;
	Uint16 *sbuffer;
	int w1, w2, w3, xr;

	inline bool draw_line(int xs, int y, int size)
	{
		Uint16 * buff = &sbuffer[y*xr];
		bary.eval_first((float) xs, (float) y);
		for (int i = xs; i < xs + size; i++) {
			float *h = bary.state;
			if (h[0] >= 0.0f && h[0] <= 1.0f && h[1] >= 0.0f && h[1] <= 1.0f && h[2] >= 0.0f && h[2] <= 1.0f) 
			{
				//
				int value = (int) (h[0] * w1 + h[1] * w2 + h[2] * w3);
				buff[i] = (buff[i] & 0xff) | (value << 8);
			}
			bary.eval_next();

		}
		return true;
	}
};

struct Simplex {
	vec2f v[3];
	float coeff[3];

	Simplex() {}
	Simplex(const vec2f& a, float ac, const vec2f& b, float bc, const vec2f& c, float cc)
	{
		v[0] = a; coeff[0] = ac;
		v[1] = b; coeff[1] = bc;
		v[2] = c; coeff[2] = cc;
	}
	void print(void) const
	{
		for (int i = 0; i < 3; i++)
			printf("\tv%d = (%.3f, %.3f), c = %.3f\n", i, v[i][0], v[i][1], coeff[i]);
		printf("\n");
		
	}
};

struct Triangularized {
	Array<Simplex> tris;
	bool isfloor, solid;
	void _kopy(const Triangularized &r)
	{
		tris = r.tris;
		isfloor = r.isfloor;
	}

	Triangularized() {}
	Triangularized(const Triangularized& r)
	{
		_kopy(r);
	}
	Triangularized & operator = (const Triangularized & r)
	{
		if (this != &r) {
			_kopy(r);
		}
		return *this;
	}
	
};

static void raster_wedge(Box2D& all, float aa[], float bb[], float cc[], int w1, int w2, int w3, 
			int xr, int yr, Uint16 *sbuffer, int thread_idx, int thread_count)
{
	vec2f a(aa), b(bb), c(cc);
	WedgeDrawer wd;
	wd.bary = bary_t(aa, bb, cc);
	wd.xr = xr;
	wd.w1 = w1; wd.w2 = w2; wd.w3 = w3; wd.sbuffer = sbuffer;

	TriangleRasterizer rasterizer(xr, yr, a, b, c, thread_idx, thread_count);
	rasterizer.draw(wd);

	rasterizer.update_limits(all.min_x, all.max_x, all.min_y, all.max_y);
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
	if (p < -1 || p > +2) {
		if (p < -1) p = -1;
		if (p > +2) p = 2;
		x[0] = a[0] + v[0] * p;
		x[1] = a[1] + v[1] * p;
		return;
	}
	float q = -D * (v[0] * h[1] - v[1] * h[0]);
	if (q < -1) q = -1;
	if (q > +2) q = 2;
	x[0] = c[0] + u[0] * q;
	x[1] = c[1] + u[1] * q;
}

struct SolidDrawer : public AbstractDrawer 
{
	Uint16 *buff, fill;
	int xr;
	SolidDrawer(Uint16 *sbuffer, Uint16 _fillcolor, int _xr) : buff(sbuffer), fill(_fillcolor), xr(_xr) {}
	inline bool draw_line(int x, int y, int size)
	{
		Uint16 *p = &buff[y * xr + x];
		if (size >= 16 && cpu.mmx) {
			fast_line_fill(p, size, fill);
		} else {
			for (int i = 0; i < size; i++)
				p[i] |= fill;
		}
		return true;
	}
};


struct Vertex {
	int no;
#ifdef DEBUG
	int color;
#endif
	Vector v;
	Vertex() {}
	Vertex(int _no, const Vector & _v):no(_no), v(_v) {}
	
	bool operator < (const Vertex &r) const
	{
		return no < r.no;
	}
};

struct PolyInfo {
	int start, size;
};


struct PolyContext {
	Mesh::EdgeInfo recu_edges[MAX_SIDES];
	Vector mesh_poly[MAX_SIDES];
	Vertex verts[MAX_SIDES];
#ifdef DEBUG
	int mesh_color[MAX_SIDES];
#endif
	vec2f cs1[MAX_SIDES], cs2[MAX_SIDES];
	
	bool ps[MAX_SIDES];
	int maxdist, maxvert;
	int temp_path[MAX_SIDES];
	int final_path[MAX_SIDES];
	int path_length;
	bool visited[MAX_SIDES];
	int fwdmap[MAX_SIDES];
	int g[MAX_SIDES][max_neighs + 1];
	float casted[MAX_SIDES][2];
	
	int recu_es;

	void * operator new (size_t size) { return sse_malloc(size); };
	void operator delete(void * what) { sse_free(what); }
};



void dfs(int node, int g[][max_neighs + 1], int lev, PolyContext &po)
{
	if (lev > po.maxdist) {
		po.maxdist = lev;
		po.maxvert = node;
	}
	po.ps[node] = true;
	for (int i = 1; i <= g[node][0]; i++)
		if (!po.ps[g[node][i]]) 
			dfs(g[node][i], g, lev + 1, po);
}

int find_most_distant(int g[][max_neighs + 1], int root, int n, PolyContext &po)
{
	memset(po.ps, 0, sizeof(bool) * n);
	po.maxdist = -1;
	dfs(root, g, 0, po);
	return po.maxvert;
}

static void record_path(int node, int dest, int g[][max_neighs + 1], bool ps[], int lev, PolyContext &po) 
{
	po.temp_path[lev] = node;
	ps[node] = true;
	if (node == dest) {
		memcpy(po.final_path, po.temp_path, (lev+1) * sizeof(int));
		po.path_length = lev+1;
		return;
	}
	
	for (int i = 1; i <= g[node][0]; i++) {
		if (!ps[g[node][i]]) {
			record_path(g[node][i], dest, g, ps, lev+1, po);
		}
	}
}

#ifdef DEBUG
static int make_color(int mod4)
{
	switch (mod4) {
		case 0: return 0xcccccc;
		case 1: return 0x00ffff;
		case 2: return 0xff00ff;
		case 3: return 0xffff00;
	}
	return 0;
}
#endif

static int connect_graph(Mesh::EdgeInfo e[], int m, PolyContext &po)
{
	int n = 0;
	memset(po.visited, 0, sizeof(po.visited));
	
	for (int i = 0; i < m; i++) {
		if (!po.visited[e[i].ai]) {
			po.visited[e[i].ai] = true;
			po.verts[n++] = Vertex(e[i].ai, e[i].a);
		}
		if (!po.visited[e[i].bi]) {
			po.visited[e[i].bi] = true;
			po.verts[n++] = Vertex(e[i].bi, e[i].b);
		}
	}
	sort_inplace(po.verts, n);
	for (int i = 0; i < n; i++)
		po.fwdmap[po.verts[i].no] = i;
	memset(po.g, 0, sizeof(int) * (max_neighs + 1) * n);
	for (int i = 0; i < m; i++) {
		int x = po.fwdmap[e[i].ai];
		int y = po.fwdmap[e[i].bi];
		if (po.g[x][0] == max_neighs || po.g[y][0] == max_neighs) {
#ifdef DEBUG
			printf("Graph representation overflow\n");
#endif
			continue;
		}
		po.g[x][++po.g[x][0]] = y;
		po.g[y][++po.g[y][0]] = x;
	}
	memset(po.visited, 0, sizeof(bool) * n);
	Array< Array <int> > compos;
	for (int i = 0; i < n; i++) if (!po.visited[i]) {
		int x = find_most_distant(po.g, i, n, po);
		int y = find_most_distant(po.g, x, n, po);
		memset(po.ps, 0, sizeof(bool) * n);
		record_path(x, y, po.g, po.ps, 0, po);
		Array<int> compo;
		for (int j = 0; j < po.path_length; j++) {
			compo += po.final_path[j];
			po.visited[po.final_path[j]] = true;
#ifdef DEBUG
			po.verts[po.final_path[j]].color = ((compos.size()%4) << 16) + po.final_path[j];
#endif
		}
		compos += compo;
	}
	//
	/*
	static bool compos_shown = false;
	if (!compos_shown) {
		printf("%d components: \n", compos.size());
		for (int i = 0; i < compos.size(); i++) {
			printf("{ ");
			for (int j = 0; j < compos[i].size(); j ++)
				printf("%d ", compos[i][j]);
			printf("}\n");
		}
		
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
		Vector & end = po.verts[all[all.size()-1]].v;
		for (int i = 0; i < compos.size(); i++) if (!used[i]) {
			double t = end.distto( po.verts[compos[i][0]].v );
			if (t < mdist) {
				bi = i;
				mdist = t;
				reversed = false;
			}
			t = end.distto( po.verts[ compos[i][compos[i].size()-1] ].v );
			if (t < mdist) {
				bi = i;
				mdist = t;
				reversed = true;
			}
		}
		used[bi] = true;
		if (reversed) compos[bi].reverse();
		/*if (!compos_shown) {
			printf("Appending current chain (ending in %d) to %d\n", all[all.size()-1], compos[bi][0]);
		}*/
		all.append(compos[bi]);
	}
	int r = 0;
	for (int i = 0; i < all.size(); i++) {
		if (r == 0 || po.mesh_poly[r-1].distto(po.verts[all[i]].v) > 1e-9) {
			po.mesh_poly [r++] = po.verts[all[i]].v;
#ifdef DEBUG
			po.mesh_color[r-1] = po.verts[all[i]].color;
#endif
		}
	}
	if (po.mesh_poly[r-1].distto(po.mesh_poly[0]) < 1e-9) r--;
	//compos_shown = true;
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

static void reversed_check(Vector verts[], const PolyInfo &pi, const Vector & l, PolyContext& po)
{
	
	int n = pi.size;
	for (int i = 0; i < n; i++) {
		Vector t = plane_cast(l, verts[pi.start + i]);
		po.casted[i][0] = (float) t.v[0];
		po.casted[i][1] = (float) t.v[2];
	}
#define fdist(a,b) (sqrt((a[0]-b[0])*(a[0]-b[0]) + (a[1]-b[1])*(a[1]-b[1])))
	float total_angle = 0;
	for (int i = 1; i <= n; i++) {
		float a = fdist(po.casted[i-1], po.casted[i%n]);
		float b = fdist(po.casted[(i+1)%n], po.casted[i%n]);
		float c = fdist(po.casted[i-1], po.casted[(i+1)%n]);
		total_angle += copysign(M_PI - acos((a*a+b*b-c*c)/(2*a*b)), face(po.casted[i-1], po.casted[i%n], po.casted[(i+1)%n]));
	}
	float mul = 1.0;
	//if (verts[pi.start][1] > l[1]) mul = -1.0;
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

static void thresh_light(Vector verts[], int n, const Vector& l)
{
	for (int i = 0; i < n; i++) {
		if (fabs(verts[i][1] - l[1]) < YOFFSET) {
			if (verts[i][1] <= l[1])
				verts[i][1] = l[1] - YOFFSET;
			else
				verts[i][1] = l[1] + YOFFSET;
		}
	}
}

static int rearrange_poly(Vector verts[], int n, const Vector& l, const Vector& cur, double alpha, double beta, PolyInfo o[], PolyContext &po)
{
	Array<Vector> up, down;
	//
	thresh_light(verts, n, l);
	for (int i = 0; i < n; i++) {
		int j = (i + 1) % n;
		Vector va = verts[i];
		Vector vb = verts[j];
		Vector a = plane_cast(l, va);
		Vector b = plane_cast(l, vb);
		bool afloor = a.v[1] == (double) daFloor;
		if ((va[1] - l[1])*(vb[1] - l[1]) < 0) {
			double t1 = (l[1] - YOFFSET - va[1])/(vb[1] - va[1]);
			double t2 = (l[1] + YOFFSET - va[1])/(vb[1] - va[1]);
			Vector dv; dv.make_vector(vb, va);
			Vector c1, c2;
			c1.macc(va, dv, t1);
			c2.macc(va, dv, t2);
			//c.v[1] -= DST_THRESHOLD;
			if (afloor) {
				down += va;
				down += c1;
				up += c2;
				up += vb;
			} else {
				up += va;
				up += c2;
				down += c1;
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
			if (i == 0 || !up[i].like(verts[pi.size-1]))
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
			if (i == 0 || !down[i].like(verts[pi.start + pi.size - 1]))
				verts[pi.start + pi.size++] = down[i];
		}
		if (pi.size > 1 && verts[pi.start + pi.size - 1] == verts[pi.start]) pi.size--;
		verts[pi.start + pi.size] = verts[pi.start];
		verts[pi.start + pi.size + 1] = verts[pi.start + 1];
		o[os++] = pi;
	}
	for (int i = 0; i < os; i++) {
		reversed_check(verts, o[i], l, po);
	}
	return os;
}

static inline bool floorness(const Vector& v, const Vector &l)
{
	return l[1] > v[1];
}

static void make_wedges(Vector verts[], int n, const Vector& l, Triangularized &solid, Triangularized &wedgy, PolyContext& po)
{
	solid.tris.clear();
	wedgy.tris.clear();
	solid.solid = true;
	wedgy.solid = false;
	Array<vec2f> inner;
	Array<vec2f> outer;
	vec2f *temp = (vec2f*) alloca(sizeof(vec2f)*n*4);
	//
	solid.isfloor = wedgy.isfloor = floorness(verts[0], l);
	//
	for (int i = 0; i < n; i++) {
		Vector a = verts[i];
		Vector b = verts[(i+1)%n];
		Vector ab, ac;
		ab.make_vector(b, a);
		ac.make_vector(l, a);
		Vector normal = ab ^ ac;
		normal.make_length(light_radius);
		for (int j = 0; j < 4; j++) {
			Vector light = l;
			if ((j > 1) ^ solid.isfloor) light += normal;
			else light -= normal;
			Vector X = (j % 2 ? b : a);
			if (floorness(X, light) != solid.isfloor) {
				if (solid.isfloor) light[1] = X[1] + YOFFSET;
				else light[1] = X[1] - YOFFSET;
			}
			Vector t = plane_cast(light, X);
			temp[i * 4 + j] = vec2f(t[0], t[2]);
		}
		
	}
	
	for (int i = 0; i < n; i++) {
		int j = (i+1)%n;
		vec2f a, b, c, d, r;
		a = temp[i * 4    ];
		b = temp[i * 4 + 1];
		c = temp[j * 4    ];
		d = temp[j * 4 + 1];
		intersect(a.v, b.v, c.v, d.v, r.v);
		//inner += r;
		inner += c;
		a = temp[i * 4 + 2];
		b = temp[i * 4 + 3];
		c = temp[j * 4 + 2];
		d = temp[j * 4 + 3];
		intersect(a.v, b.v, c.v, d.v, r.v);
		//outer += r;
		outer += c;
	}
	//
	float si = (float) shadow_intensity;
	float so = 0.0f;
	for (int i = 0; i < n; i++) {
		int j = (i + 1) % n;
		vec2f v0 = inner[i];
		vec2f v1 = inner[j];
		vec2f v2 = outer[i];
		vec2f v3 = outer[j];
		//
		
		wedgy.tris += Simplex(v2, so, v1, si, v0, si);
		wedgy.tris += Simplex(v2, so, v3, so, v1, si);
	}
	//
		
	//
	int cs1a, cs2a;
	cs1a = n; memcpy(po.cs1, &inner[0], n * sizeof(po.cs1[0]));
	while (cs1a > 2) {
		cs2a = 0;
		int i = 0;
		while (i < cs1a - 1) {
			if (face(po.cs1[i], po.cs1[i+1], po.cs1[(i+2)%cs1a]) < +1e-6) {
				//draw it
				solid.tris += Simplex(po.cs1[i], si, po.cs1[i+1], si, po.cs1[(i+2)%cs1a], si);
				//discard the second point
				po.cs2[cs2a++] = po.cs1[i];
				i += 2;
			} else {
				po.cs2[cs2a++] = po.cs1[i];
				i++;
			}
		}
		if (i == cs1a - 1) po.cs2[cs2a++] = po.cs1[i];

		if (cs2a == cs1a) break;
		cs1a = cs2a;
		memcpy(po.cs1, po.cs2, cs1a * sizeof(po.cs1[0]));
	}

}

static Simplex gen_simplex(Simplex & orig, ClipRes & cr, int ia, int ib, int ic)
{
	int ind[3] = {ia, ib, ic};
	vec2f verts[3];
	float coefs[3];
	Vector Co = Vector(orig.coeff[0], orig.coeff[1], orig.coeff[2]);
	for (int i = 0; i < 3; i++) {
		verts[i] = vec2f(cr.v[ind[i]][0], cr.v[ind[i]][2]);
		coefs[i] = (float) (Co * cr.bary[ind[i]]);
	}
	return Simplex(verts[0], coefs[0], verts[1], coefs[1], verts[2], coefs[2]);
}

static void frustrum_clip(FrustrumClipper& clipper, Triangularized &t)
{
	for (int i = 0; i < 4; i++) {
		Array<Simplex> n;
		for (int j = 0; j < t.tris.size(); j++) {
			Simplex& s = t.tris[j];
			Vector v[3];
			for (int k = 0; k < 3; k++) {
				v[k] = Vector(s.v[k][0], t.isfloor ? daFloor: daCeiling, s.v[k][1]);
			}
			ClipRes cr;
			clipper.clip(cr, v, 1<<i);
			if (cr.n) {
				n += gen_simplex(s, cr, 0, 1, 2);
				if (cr.n == 4) {
					n += gen_simplex(s, cr, 2, 3, 0);
				}
			}
		}
		t.tris = n;
	}
}

static void poly_bias(Vector verts[], int n, const Vector& light, const Vector& center)
{
	switch (g_biasmethod) {
		case 1:
		{
			for (int i = 0; i < n; i++) {
				verts[i] = center + (verts[i]-center) * 0.98;
			}
			break;
		}
		case 2:
		{
			for (int i = 0; i < n; i++) 
				if (verts[i][1] > daCeiling - 5 || verts[i][1] < daFloor + 5) {
					Vector v = light - verts[i];
					v.make_length(10);
					verts[i] += v;
				}
			break;
		}
	}
}

static void poly_display2(Triangularized &t, int xr, int yr, const Vector& cur, const Vector& mtt, const Vector& mti, const Vector& mtti, Uint16 *sbuffer, int thread_idx = 0, int thread_count = 1)
{
	Box2D all;
	
	for (int i = 0; i < t.tris.size(); i++) {
		Simplex & s = t.tris[i];
		vec2f v[3];
		for (int j = 0; j < 3; j++) {
			Vector d = Vector(s.v[j][0], t.isfloor ? daFloor : daCeiling, s.v[j][1]);
			project_point(&v[j][0], &v[j][1], d, cur, w, xr, yr);
		}
		if (t.solid) {
			int intensity = shadow_intensity << 8;
			SolidDrawer sd(sbuffer, intensity, xr);
			TriangleRasterizer ras(xr, yr, v[0], v[1], v[2], thread_idx, thread_count);
			ras.draw(sd);
			ras.update_limits(all.min_x, all.max_x, all.min_y, all.max_y);
		} else {
			raster_wedge(all,
					v[0].v, v[1].v, v[2].v, 
					(int) s.coeff[0], (int) s.coeff[1], (int) s.coeff[2], 
					xr, yr, sbuffer, thread_idx, thread_count);
		}
	}
	/* stage 3: reblend */
	
	CLAMP(all.min_x, 0, xr-1);	
	CLAMP(all.max_x, 0, xr-1);	
	CLAMP(all.min_y, 0, yr-1);	
	CLAMP(all.max_y, 0, yr-1);
	if (cpu.mmx) {
		fast_reblend_mmx(all.min_x, all.min_y, all.max_x, all.max_y, sbuffer, xr, shadow_intensity,
				thread_idx, thread_count);
	} else {
		for (int j = all.min_y; j <= all.max_y; j++) if (thread_idx == j % thread_count) {
			Uint16 *buff = &sbuffer[j * xr];
			for (int i = all.min_x; i <= all.max_x; i++) {
				Uint16 t = buff[i];
				t = (t & 0xff) + (t >> 8);
				if (t > shadow_intensity) t = shadow_intensity;
				buff[i] = t; 
			}
		}
	}
}

static int occlusions(const Sphere & a, const Sphere & b, const Vector & light)
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
		if (b.fastintersect( ray, v, ray.lengthSqr(), temp)) c++;
	}
	return c;
}

static int occlusions(const Sphere &a, Mesh & m, const Vector & light, bool full_intersections)
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
			if (m.fullintersect(v, ray)) c++;
		} else {
			if (m.testintersect(v, ray)) c++;
		}
	}
	return c;
}


void check_for_shadowed_spheres(void)
{
	Vector ml = light.p;
	Array<ShadowCaster*> all;
	
	for (int i = 0; i < spherecount; i++) {
		Sphere & s = sp[i];
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
			if ((mesh[j].get_flags() & CASTS_SHADOW) && 8 == occlusions(s, mesh[j], ml, true)) {
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
			if ((mesh[j].get_flags() & CASTS_SHADOW) && occlusions(s, mesh[j], ml, false)) {
				s.flags |= PART_SHADOWED;
				all += &mesh[j];
				csize++;
			}
		s.occluders_size = csize;
	}
	if (all.size() > 10000) printf("Too much occluders!!!\n");
	for (int i = 0; i < all.size(); i++)
		occluders[i] = all[i];
}

void shadows_close(void)
{
}


void render_shadows_init(Uint32 *target_framebuffer, Uint16 *sbuffer, int xr, int yr, const Vector& mtt, const Vector& mti, const Vector& mtti)
{
	ceilmax = floormin = -1;

	prof_enter(PROF_ZERO_SBUFFER); // reset the S-buffer;
	memset(sbuffer, 0, xr*yr*sizeof(Uint16));
	prof_leave(PROF_ZERO_SBUFFER);
	
	shadows_precalc(xr, yr, mtt, mti, mtti);
	
	rowint = 0;
	
}

static Allocator<PolyContext> allocator(ALLOCATOR_NEW_DELETE);
#ifdef DEBUG
Array<node_info> node_arr;
#endif

static void render_shadows_raster(Uint32 *target_framebuffer, Uint16 *sbuffer, int xr, int yr, 
		    const Vector& mtt, const Vector& mti, const Vector& mtti, int thread_idx)
{
	Vector ml = light.p;
	PolyContext *po = allocator[thread_idx];
		
	for (int i = 0; i < spherecount; i++) if ((sp[i].flags & CASTS_SHADOW)/* && thread_idx == i % cpu.count*/) {
		Vector poly[SPHERE_SIDES*2+8];
		
		prof_enter(PROF_POLY_GEN);
		sp[i].calculate_fixed_convex(poly, ml, SPHERE_SIDES);
		prof_leave(PROF_POLY_GEN);
		
		poly_bias(poly, SPHERE_SIDES, ml, sp[i].pos);
		
		PolyInfo pif[2];
		prof_enter(PROF_POLY_REARRANGE);
		int pir = rearrange_poly(poly, SPHERE_SIDES, ml, cur, CVars::alpha, CVars::beta, pif, *po);
		prof_leave(PROF_POLY_REARRANGE);
		

		for (int i = 0; i < pir; i++) {
			prof_enter(PROF_MAKE_WEDGES);
			Triangularized solid, wedgy;
			make_wedges(poly + pif[i].start, pif[i].size, ml, solid, wedgy, *po);
			prof_leave(PROF_MAKE_WEDGES);
			
			prof_enter(PROF_FRUSTRUM_CLIP);
			FrustrumClipper frustrum(cur, mtt, mti * (double)xr, mtti * (double)yr);
			frustrum_clip(frustrum, solid);
			frustrum_clip(frustrum, wedgy);
			prof_leave(PROF_FRUSTRUM_CLIP);
				
			prof_enter(PROF_POLY_DISPLAY);
			poly_display2(solid, xr, yr, cur, mtt, mti, mtti, sbuffer, thread_idx, cpu.count);
			poly_display2(wedgy, xr, yr, cur, mtt, mti, mtti, sbuffer, thread_idx, cpu.count);
			prof_leave(PROF_POLY_DISPLAY);
		}
	}
	
	
	for (int i = 0; i < mesh_count; i++) if ((mesh[i].get_flags() & CASTS_SHADOW)) {
		po->recu_es = 0;
		prof_enter(PROF_CONNECT_GRAPH);
		for (int j = 0; j < mesh[i].num_edges; j++)
		{
			Mesh::EdgeInfo &e = mesh[i].edges[j];
			Vector p = (e.a + e.b) * 0.5 - ml;
			if ((p * e.norm1) * (p * e.norm2) < 0.0 && !in_shadow(e, mesh[i], ml))
				po->recu_edges[po->recu_es++] = e;
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
		int r = connect_graph(po->recu_edges, po->recu_es, *po);
		prof_leave(PROF_CONNECT_GRAPH);
		
		poly_bias(po->mesh_poly, r, ml, mesh[i].center);
	
		PolyInfo pif[2];
		prof_enter(PROF_POLY_REARRANGE);
		int pir = rearrange_poly(po->mesh_poly, r, ml, cur, CVars::alpha, CVars::beta, pif, *po);
		prof_leave(PROF_POLY_REARRANGE);
		
#ifdef DEBUG
		if ( 0 == thread_idx ) {
			node_arr.clear();
			for (int j = 0; j < pir; j++) {
				for (int i = 0; i < pif[j].size; i++) {
					node_info info;
					
					bool isfloor;
					if (project_point_shadow(po->mesh_poly[pif[j].start+i], ml, &info.x, &info.y, 
						xr, yr, cur, mtt, mti, mtti, isfloor)) {
						info.x -= 6;
						info.y -= 8;
						info.number = po->mesh_color[pif[j].start+i] & 0xffff;
						//info.number = node_arr.size();
						info.color = make_color(po->mesh_color[pif[j].start + i] >> 16);
						node_arr.add(info);
					}
				}
			}
		}
#endif
		
		for (int i = 0; i < pir; i++) {
			prof_enter(PROF_MAKE_WEDGES);
			Triangularized solid, wedgy;
			make_wedges(po->mesh_poly + pif[i].start, pif[i].size, ml, solid, wedgy, *po);
			prof_leave(PROF_MAKE_WEDGES);
			
			prof_enter(PROF_FRUSTRUM_CLIP);
			FrustrumClipper frustrum(cur, mtt, mti * (double)xr, mtti * (double)yr);
			frustrum_clip(frustrum, solid);
			frustrum_clip(frustrum, wedgy);
			prof_leave(PROF_FRUSTRUM_CLIP);
			
			prof_enter(PROF_POLY_DISPLAY);
			poly_display2(solid, xr, yr, cur, mtt, mti, mtti, sbuffer, thread_idx, cpu.count);
			poly_display2(wedgy, xr, yr, cur, mtt, mti, mtti, sbuffer, thread_idx, cpu.count);
			prof_leave(PROF_POLY_DISPLAY);
		}
	}
	
}

static void render_shadows_raytracing(Uint32 *target_framebuffer, Uint16 *sbuffer, int xr, int yr, 
		    const Vector& mtt, const Vector& mti, const Vector& mtti, int thread_idx)
{
	int i, j;
	while ((j = rowint++) < yr) {
		Uint16 *s = sbuffer + (j * xr);
		for (i = 0; i < xr; i++) {
			Vector I = plane_cast(cur, mtt + mti * i + mtti * j);
			s[i] = (Uint16) (shadow_intensity * light.shadow_density(I));
		}
	}
}

void render_shadows(Uint32 *target_framebuffer, Uint16 *sbuffer, int xr, int yr, 
		    const Vector& mtt, const Vector& mti, const Vector& mtti, int thread_idx)
{
	if (CVars::shadow_algo == 0)
		render_shadows_raster(target_framebuffer, sbuffer, xr, yr, mtt, mti, mtti, thread_idx);
	else
		render_shadows_raytracing(target_framebuffer, sbuffer, xr, yr, mtt, mti, mtti, thread_idx);
}
