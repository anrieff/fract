/***************************************************************************
 *   Copyright (C) 2005 by Veselin Georgiev                                *
 *   vesko@workhorse                                                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 *                                                                         *
 *  ---------------------------------------------------------------------  *
 *                                                                         *
 *    Defines the Mesh class                                               *
 *                                                                         *
 ***************************************************************************/

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "cvars.h"
#include "mesh.h"
#include "triangle.h"
#include "memory.h"
#include "progress.h"

Mesh mesh[MAX_OBJECTS];
int mesh_count;

struct trio_slot{
	Vector normal;
	int idx;
	int count;
	bool operator < (const trio_slot& r) const
	{
		return count > r.count;
	}
};

bool g_speedup = true;

/** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * @class Mesh::EdgeInfo                                      *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void * Mesh::EdgeInfo::operator new[](size_t size)
{
	return sse_malloc(size);
}

void Mesh::EdgeInfo::operator delete[](void *what)
{
	return sse_free(what);
}

/** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * @class Mesh                                                *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

int Mesh::get_triangle_base() const
{
	return (this - mesh) * MAX_TRIANGLES_PER_OBJECT;
}



void Mesh::scale(double factor)
{
	int base, i;
	base = get_triangle_base();
	for (i = 0; i < triangle_count; ++i) {
		for (int j = 0; j < 3; j++) {
			trio[base + i].vertex[j] -= center;
			trio[base + i].vertex[j].scale(factor);
			trio[base + i].vertex[j] += center;
		}
	}
	if (edges) {
		for (int i = 0; i < num_edges; i++) {
			edges[i].a -= center;
			edges[i].b -= center;
			edges[i].a.scale(factor);
			edges[i].b.scale(factor);
			edges[i].a += center;
			edges[i].b += center;
		}
	}
	BBox::scale(factor, center);
	total_scale *= factor;
	if (iprimitive)
		iprimitive->scale(factor);
	if (sdtree)
		sdtree->scale(factor, center);
}

void Mesh::translate(const Vector & transp)
{
	center += transp;
	int base, i;
	base = get_triangle_base();
	for (i = 0; i < triangle_count; ++i) {
		for (int j = 0; j < 3; j++) {
			trio[base + i].vertex[j] += transp;
		}
	}
	
	if (edges) {
		for (int i = 0; i < num_edges; i++) {
			edges[i].a += transp;
			edges[i].b += transp;
		}
	}
	
	total_translation += transp;
	BBox::translate(transp);
	if (iprimitive)
		iprimitive->translate(transp);
	if (sdtree)
		sdtree->translate(transp);
}

bool Mesh::bound(int compo, double minv, double maxv)
{
	Vector unit;
	unit.zero();
	unit.v[compo] = 1;
	if (vmin[compo] < minv)
		translate(unit * (minv - vmin[compo]));
	if (vmax[compo] > maxv)
		translate(unit * (maxv - vmax[compo]));
	return (vmin[compo] >= minv);
}

void Mesh::set_flags(Uint32 newflags, float refl, float opacity, int set_type, Uint32 color)
{
	flags = newflags;
	int base = get_triangle_base();
	for (int i = 0; i < triangle_count; i++) {
		if (set_type == TYPE_OR)
			trio[base + i].flags |= newflags;
			else
			trio[base + i].flags = newflags;
		if (refl >= 0) trio[base + i].refl = refl;
		if (opacity >= 0) trio[base + i].opacity = opacity;
		if (color < 0x1000000) trio[base + i].color = color;

	}
}

static int iswhitespace(char c)
{
	return (c == ' ' || c == '\t');
}

static bool read_real(char *buf, int & sp, int &ep, double &res)
{
	ep = sp;
	while (buf[ep] && buf[ep] != '-' && buf[ep] != '-' && (buf[ep] < '0' || buf[ep] > '9')) ep++;
	if (!buf[ep]) return false;
	sp = ep;
	while (buf[ep] == '-' || buf[ep]=='.' || (buf[ep] >= '0' && buf[ep] <= '9')) ep++;
	if (buf[ep]==0) {
		if (1 != sscanf(buf + sp, "%lf", &res)) return false;
		sp = ep;
	} else {
		buf[ep] = 0;
		if (1!=sscanf(buf + sp, "%lf", &res)) return false;
		sp = ep+1;
	}
	return true;
}

static bool read_real(char *buf, int & sp, int &ep, float &res)
{
	double dbl=0;
	bool ok = read_real(buf, sp, ep, dbl);
	res = (float)dbl;
	return ok;
}

static bool read_string(char *buf, int & sp, int & ep, char *out)
{
	ep = sp;
	while (buf[ep] && iswhitespace(buf[ep])) ep++;
	if (!buf[ep]) return false;
	sp = ep;
	while (buf[ep] && !iswhitespace(buf[ep])) ep++;
	if (buf[ep]==0) {
		if (1 != sscanf(buf + sp, "%s", out)) return false;
		sp = ep;
	} else {
		buf[ep] = 0;
		if (1 != sscanf(buf + sp, "%s", out)) return false;
		sp = ep+1;
	}
	return true;
}

static bool read_trio(char *buf, int & sp, int & ep, int a[3])
{
	char s[100];
	if (!read_string(buf, sp, ep, s)) return false;
	a[0] = a[1] = a[2] = 0;
	int i, k=0, l;
	l = strlen(s);
	for (i = 0;i < l ; i++) {
		switch (s[i]) {
			case '/': if (++k >= 3) return false; break;
			case ' ': break;
			default:
			{
				if (isdigit(s[i])) a[k] = a[k] * 10 + (int) (s[i] - '0');
				else return false;
				break;
			}
		}
	}
	return true;
}


static char firstchar(char *s)
{
	int i = 0;
	while (s[i] && iswhitespace(s[i])) i++;
	return s[i];
}

/*
	The file format is text and is the same as in FSAATest.
	Each line describes a triangle. First, 3x3 doubles come - the coordinates of the vertices
	(the point (0,0,0) is considered to be the center of the mesh).

	After that three floats come, each in the range [0-1], that are the color of the
	triangle in RGB order.

	if use_mapping_coords == true, then there ain't any color data, but there are six floats,
	again in [0-1], the (u,v) mapping coordinates of each vertex
*/
void Mesh::read_from_text_file(const char *fn, bool use_mapping_coords)
{
	FILE *f;
	char buf[300];
	int base = get_triangle_base();
	f = fopen(fn, "rt");
	triangle_count = 0;
	if (!f) {
		return;
	}
	while (fgets(buf, 300, f)) {
		int sp = 0, ep;
		bool skipline = false;
		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 3; j++) {
				if (!read_real(buf, sp, ep, trio[base+triangle_count].vertex[i].v[j]))
					skipline = true;
			}
		}
		if (skipline) continue;
		if (use_mapping_coords) {
			for (int i = 0; i < 3; i++)
				for (int j = 0; j < 2; j++)
					read_real(buf, sp, ep, trio[base+triangle_count].mapcoords[i][j]);
		} else {
			float x;
			Uint32 color = 0;
			for (int i = 0; i < 3; i++) {
				read_real(buf, sp, ep, x);
				if (x < 0.0f) x = 0.0f;
				if (x > 1.0f) x = 1.0f;
				color = (color << 8) + (int) (255.0f*x);
			}
			trio[base + triangle_count].color = color;
		}
		triangle_count++;
	}
	fclose(f);
	recalc();
}

static void addedge(Vector vertices[], Mesh::EdgeInfo edges[], int & edgc, int a, int b, const Vector & normal)
{
	if (a > b) {
		int t = a;
		a = b;
		b = t;
	}
	for (int i = 0; i < edgc; i++) if (edges[i].ai == a && edges[i].bi == b) {
		if (edges[i].nc != 1) {
#ifdef DEBUG
			//printf("addedge: Warning, while adding edge, unexpected number of allready added normals (%d)\n", 
			  //     edges[i].nc);
#endif
			return;
		}
		edges[i].nc = 2;
		edges[i].norm2 = normal;
		return;
	}
	Mesh::EdgeInfo & e = edges[edgc];
	edgc++;
	e.ai = a;
	e.bi = b;
	e.a = vertices[a];
	e.b = vertices[b];
	e.nc = 1;
	e.norm1 = normal;
}
/*
	The .OBJ 3D file format is used by Wavefront's Advanced Visualizer.
	Description of the format: http://www.dcs.ed.ac.uk/home/mxr/gfx/3d/OBJ.spec
	
	The implementation below follows the format spec and is tested to comply
	with the files, written by SOFTIMAGE|XSI's "export" option
*/
bool Mesh::read_from_obj(const char *fn)
{
	Task task(READOBJ, __FUNCTION__);
	FILE *f;
	int base = get_triangle_base();
	char basedir[100];
	static Vector vertices[MAX_TRIANGLES_PER_OBJECT * 3];
	static int    vert_map[MAX_TRIANGLES_PER_OBJECT * 3];
	static int    dup_map [MAX_TRIANGLES_PER_OBJECT * 3];
	static EdgeInfo    edg[MAX_TRIANGLES_PER_OBJECT * 3];
	int edgc = 0;
	int vc = 0;
	static float mappings[MAX_TRIANGLES_PER_OBJECT * 3][2];
	int tc = 0;
	BBox::recalc();
	center.zero();
	f = fopen(fn, "rt");
	triangle_count = 0;
	if (!f) {
		return false;
	}
	strcpy(basedir, fn);
	int i = strlen(basedir);
	while (i > 0 && basedir[i] != '/') i--;
	if (i) i++;
	basedir[i] = 0;
	
	char buff[1000];
	char cmd[100];
	int l = 0;
	while (fgets(buff, 1000, f)) {
		++l;
		int sp = 0, ep;
		char fc = firstchar(buff);
		
		if (fc == '\n' || fc == 0 || fc == '#' || fc == '\r') continue;
		read_string(buff, sp, ep, cmd);
		if (!strcmp("v", cmd)) {
			bool ok = true;
			for (int i = 0; i < 3; i++)
				ok &= read_real(buff, sp, ep, vertices[vc].v[i]);
			if (ok) {
				dup_map[vc] = vc;
				for (int j = 0; j < vc; j++)
					if (vertices[vc].like(vertices[j])) {
						// found a duplicate
						dup_map[vc] = j;
						break;
					}
				vc++;
			}
			else printf("Mesh::ReadFromObj(%s): error parsing line #%d\n", fn, l);
		}
		if (!strcmp("vt", cmd)) {
			bool ok = true;
			for (int i = 0; i < 2; i++)
				ok &= read_real(buff, sp, ep, mappings[tc][i]);
			if (ok) tc++;
			else printf("Mesh::ReadFromObj(%s): error parsing line #%d\n", fn, l);
		}
		if (!strcmp("mtllib", cmd)) {
			char mtl_filename[100];
			read_string(buff, sp, ep, mtl_filename);
			if (!parse_mtl_lib(mtl_filename, basedir)) {
				printf("Can't parse the material lib!\n");
				return false;
			}
		}
		if (!strcmp("usemtl", cmd)) {
			// currently ignore
		}
		if (!strcmp("f", cmd)) {
			int a[3][3];
			bool ok = true;
			for (int i = 0; i < 2; i++) {
				ok &= read_trio(buff, sp, ep, a[2*i]);
				if (a[2*i][0] > vc || a[2*i][1] > tc) {
					ok = false; break;
				}
				a[2*i][0] = dup_map[a[2*i][0]-1];
			}
			if (!ok) {
				printf("Mesh::ReadFromObj(%s): error parsing line #%d\n", fn, l);
				continue;
			}
			bool added = false;
			int poly_triangles = 2;
			int lastvert = a[2][0];
			Vector facenorm;
			while (1) {
				for (int i = 0; i < 3; i++)
					a[1][i] = a[2][i];
				if (!read_trio(buff, sp, ep, a[2])) {
					break;
				}
				a[2][0] = dup_map[a[2][0]-1];
				
				
				++poly_triangles;
				if (!added) {
					Vector A = vertices[a[0][0]];
					Vector B = vertices[a[1][0]];
					Vector C = vertices[a[2][0]];
					B -= A;
					C -= A;
					facenorm = (B^C);
					facenorm.norm();
					addedge(vertices, edg, edgc, a[0][0], a[1][0], facenorm);
				}
				added = true;
				for (int i = 0; i < 3; i++) {
					trio[base + triangle_count].vertex[i] = vertices[a[i][0]];
					if (a[i][1] != 0) {
						for (int j = 0; j < 2; j++)
							trio[base + triangle_count].mapcoords[i][j] =
								mappings[a[i][1]-1][j];
					}
				}
				addedge(vertices, edg, edgc, lastvert, a[2][0], facenorm);
				trio[base + triangle_count].flags |= flags;
				trio[base + triangle_count].tag = (this - mesh) + OB_TRIANGLE;
				trio[base + triangle_count].tri_offset = 0;
				++triangle_count;
				lastvert = a[2][0];
			}
			if (!added) {
				printf("Mesh::ReadFromObj(%s) warning: polygon defined with no triangles at line #%d!\n", fn, l);
			} else 
				addedge(vertices, edg, edgc, lastvert, a[0][0], facenorm);
			int k = base + triangle_count - (poly_triangles - 2);
			trio[k].tri_offset = poly_triangles - 3;
		}
	}
	fclose(f);
/*
	Check for non-convex faces
*/	
	int bad_faces = 0, bad_fixed = 0;
	for (i = 0; i < triangle_count; i++) if (trio[base+i].tri_offset) {
		int size = trio[base+i].tri_offset + 1;
		trio_slot TS[20];
		int tc = 0;
		memset(TS, 0, sizeof(TS));
		for (int j = 0; j < size; j++) {
			trio[base+i+j].a.make_vector(trio[base+i+j].vertex[1], trio[base+i+j].vertex[0]);
			trio[base+i+j].b.make_vector(trio[base+i+j].vertex[2], trio[base+i+j].vertex[0]);
			trio[base+i+j].normal = trio[base+i+j].a ^ trio[base+i+j].b;
			trio[base+i+j].normal.norm();
			bool found = false;
			for (int k = 0; k < tc; k++) {
				if (TS[k].normal.like(trio[base+i+j].normal, 0.01)) {
					found = true;
					TS[k].count ++;
				}
			}
			if (!found) {
				if (tc > 19) {
#ifdef DEBUG					
					printf("Error! Slots full in mesh.cpp\n");
					return false;
#else
					break;
#endif
				}
				TS[tc].normal = trio[base+i+j].normal;
				TS[tc].count = 1;
				TS[tc].idx = j;
				tc++;
			}
		}
		sort(TS, tc);
		if (tc > 1) {
			bad_faces ++;
		}
		if (tc > 1 && TS[0].count != TS[1].count) {
			bad_fixed++;
			for (int j = 0; j < size; j++) {
				if (!trio[base + i + j].normal.like(TS[0].normal, 0.01)) {
					Vector t;
					float t1[2];
					for (int q = 0; q < 2; q++) {
						t1[q] = trio[base + i + j].mapcoords[1][q];
						trio[base + i + j].mapcoords[1][q] = trio[base + i + j].mapcoords[2][q];
						trio[base + i + j].mapcoords[2][q] = t1[q] ;
					}
					t = trio[base + i + j].vertex[1];
					trio[base + i + j].vertex[1] = trio[base + i + j].vertex[2];
					trio[base + i + j].vertex[2] = t;
				}
			}
		}
	}
#ifdef DEBUG
	if (bad_faces)
		printf("WARNING: %s: found %d non-convex faces, %d fixed\n", fn, bad_faces, bad_fixed);
#endif
	
/*
	Optimize vertices and remake normal map
*/	
	task.set_steps(vc);
	map_size = 0;
	for (int i = 0; i < vc; i++) {
		bool dup = false;
		for (int j = 0; j < map_size; j++) {
			if (vertices[i] == vertices[vert_map[j]]) {
				dup = true;
				break;
			}
		}
		if (!dup) {
			vert_map[map_size++] = i;
		}
		if (i % 64 == 0) task += 64;
	}
	Vector *vertexlist = (Vector*) sse_malloc(sizeof(Vector)*map_size);
	for (int i = 0; i < map_size; i++)
		vertexlist[i] = vertices[vert_map[i]];
	for (int i = 0; i < triangle_count; i++) {
		for (int j = 0; j < 3; j++) {
			trio[base + i].nm_index[j] = 0;
			while (!(vertexlist[trio[base + i].nm_index[j]] == trio[base + i].vertex[j]))
				trio[base + i].nm_index[j]++;
		}
	}
	sse_free(vertexlist);
	// Allocate a normal map	
	normal_map = (Vector*) sse_malloc(map_size * sizeof(Vector));
/*
	Normalize the mesh size
	
*/
	task.finish();
	recalc();
	double maxd = vmax[0]-vmin[0] > vmax[1]-vmin[1] ? vmax[0]-vmin[0]:vmax[1]-vmin[1];
	maxd = maxd < vmax[2]-vmin[2] ? vmax[2]-vmin[2]: maxd;
	
	
	int valid_edges = 0;
	for (int i = 0; i < edgc; i++)
		valid_edges += edg[i].nc == 2;
#ifdef DEBUG
	printf("Valid edges %d / %d\n", valid_edges, edgc);
#endif
	num_edges = 0;
	edges = new EdgeInfo [ valid_edges ];
	for (int i = 0; i < edgc; i++) if (edg[i].nc == 2)
		edges[num_edges++] = edg[i];
/*
	Store some additional info
*/
	scale(1.0/maxd);
	
	strcpy(file_name, fn);
	total_scale = 1.0;
	total_translation.zero();
	
#ifdef DEBUG
	printf("%s: Loaded, %d triangles\n", fn, triangle_count);
#endif
	if (triangle_count > 2500)
		CVars::shadow_algo = 1;
	return true;
}

bool Mesh::parse_mtl_lib(const char *fn, const char *base_dir)
{
	char filename[256];
	char buff[1000];
	strcpy(filename, base_dir);
	strcat(filename, fn);
	FILE *f = fopen(filename, "rt");
	if (!f) return false;
	while (fgets(buff, 1000, f)) {
		if (!strncmp("map_Kd", buff, 6)) {
			char map_file[256];
			strcpy(map_file, base_dir);
			int i = 7;
			while (buff[i]) i++;
			while (i > 7 && buff[i] != '/') i--;
			if (buff[i]=='/') i++;
			strcat(map_file, buff + i);
			i = strlen(map_file);
			//if (!isalpha(map_file[i])) map_file[i] = 0;
			while (i--) if (map_file[i]<32) map_file[i] = 0;
			image = new RawImg(map_file);
			flags = MAPPED;
			fclose(f);
			return true;
		}
	}
	fclose(f);
	return false;
}

void Mesh::recalc(void)
{
	BBox::recalc();
	int base = get_triangle_base();
	for (int i = 0; i < triangle_count; i++)
		add(trio[base + i]);
	if (iprimitive)
		iprimitive->recalc(this);
	else {
		const int isp_count = 2;
		Inscribed * is[2];
		is[0] = new InscribedSphere(this);
		is[1] = new InscribedCube(this);
		
		int bi = 0;
		double bestvol = is[0]->volume();
		for (int i = 1; i < isp_count; i++) {
			if (is[i]->volume() > bestvol) {
				bestvol = is[i]->volume();
				bi = i;
			}
		}
#ifdef DEBUG
		printf("Choosing representing silhouette amongst:\n");
		for (int i = 0; i < isp_count; i++) {
			printf("%s: volume = %.3lf\n", is[i]->get_name(), is[i]->volume());
		}
		printf("\nChosen best representee %s with volume %.3lf\n", is[bi]->get_name(), is[bi]->volume());
#endif
		for (int i = 0; i < isp_count; i++)
			if (i != bi) delete is[i];
		iprimitive = is[bi];
	}
	if (sdtree)
		sdtree->build_tree();
	else {
		if (triangle_count > 140) {
			sdtree = new SDTree(trio + get_triangle_base(), triangle_count);
			sdtree->build_tree();
		}
	}
}

Uint32 Mesh::get_flags(void) const
{
	return trio[get_triangle_base()].flags;
}

void Mesh::rebuild_normal_map(void)
{
	static int normal_div[MAX_TRIANGLES_PER_OBJECT * 3];
	for (int i = 0; i < map_size; i++) {
		normal_map[i].zero();
		normal_div[i] = 0;
	}
	int base = get_triangle_base();
	for (int i = 0; i < triangle_count; i++) {
		for (int j = 0; j < 3; j++) {
			int k = trio[base + i].nm_index[j];
			normal_div[k]++;
			normal_map[k] += trio[base + i].normal;
		}
	}
	for (int i = 0; i < map_size; i++) {
		if (!normal_div[i]) {
			printf("normal_div[%d]==0!!!\n", i);
			continue;
		}
		normal_map[i].scale(1.0/normal_div[i]);
		
	}
}

static int l_all_tests, l_all_success;
static int sd_all_tests, sd_all_success;

bool Mesh::fullintersect(const Vector & start, const Vector &dir, int opt)
{
	char ctx[128];
	if (!testintersect(start, dir)) return false;
	
	int i = lastind[opt];
	int base = get_triangle_base();

	if (i >= 0 && i < triangle_count)
		if (trio[base + i].intersect(dir, start, ctx)) return true;
	
	if (g_speedup && iprimitive) {
		l_all_tests++;
		bool res = iprimitive->testintersect(start, dir);
		if (res) { l_all_success++; return true; }
	}
	
	if (g_speedup && sdtree) {
		sd_all_tests++;
		bool res = sdtree->testintersect(start, dir, ctx, NULL);
		if (res) { sd_all_success++;}
		return res;
	}
	

	
	bool found = false;
	for (i = 0; i < triangle_count; i++) {
		if (trio[base + i].intersect(dir, start, ctx)) {
			found = true;
			break;
		}
	}
	lastind[opt] = i;
	return found;
}

bool Mesh::sintersect(const Vector & start, const Vector &dir, int opt)
{
	return fullintersect(start, dir, opt);
}

/** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * @class Inscribed                                           *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void * Inscribed::operator new(size_t size)
{
	return sse_malloc(size);
}

void Inscribed::operator delete(void * what)
{
	sse_free(what);
}


/** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * @class InscribedSphere                                     *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

InscribedSphere::InscribedSphere( Mesh *m ) { recalc(m); }
double InscribedSphere::volume( void ) const { return 4.0 / 3.0 * M_PI * R * R * R; }

bool InscribedSphere::testintersect( const Vector &start, const Vector & ray)
{
	Vector t;
	double C, gB, det;
	
	t.make_vector(start, center);
	gB = t*ray;
	C = t.lengthSqr() - R * R;
	det = gB*gB - C;
	return (det >= 0.0f && gB <= 0.0f);
}

double InscribedSphere::planedist(Triangle & t)
{
	Vector va = t.vertex[1] - t.vertex[0];
	Vector vb = t.vertex[2] - t.vertex[0];
	Vector n = va ^ vb;
	n.norm();
	double X = -(t.vertex[0] * n);
	return fabs( X + (center * n) );
}

void InscribedSphere::recalc(Mesh *m)
{
	center = m->center;
	int base = m->get_triangle_base();
	R = 1e99;
	for (int i = 0; i < m->triangle_count; i++) {
		double t = planedist(trio[base + i]);
		if (t < R) R = t;
	}
	if (R < 0.01) R = 0.01;
}

const char * InscribedSphere::get_name(void) const
{
	return "inscribed sphere";
}

void InscribedSphere::scale(double factor)
{
	R *= factor;
}

void InscribedSphere::translate(const Vector& trans)
{
	center += trans;
}

/** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * @class InscribedCube                                       *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

InscribedCube::InscribedCube( Mesh *m ) { recalc(m); }
double InscribedCube::volume( void ) const { return 8.0 * R * R * R; }

bool InscribedCube::testintersect( const Vector &start, const Vector & ray)
{
	return bbox.testintersect(start, ray);
}

void InscribedCube::recalc(Mesh *m)
{
	center = m->center;
	int base = m->get_triangle_base();
	R = 1e99;
	bool ever_crossed = false;
	for (int i = 0; i < m->triangle_count; i++) {
		Triangle T = trio[base + i];
		
		// swap two vertices to negate the orientation of the triangle
		// since we are intersecting from inside of the object
		Vector vt = T.vertex[0];
		T.vertex[0] = T.vertex[1];
		T.vertex[1] = vt;
		T.recalc_normal();
		T.recalc_zdepth(center);
		//
		for (int j = 0; j < 27; j++) {
			if (j == 9 + 3 + 1) continue; // if v is the null vector
			double co[3];
			int jj = j;
			for (int k = 0; k < 3; k++) {
				co[k] = (jj % 3) - 1; jj /= 3;
			}
			Vector v(co);
			Vector nv = v; nv.norm();
			char *buf[128];
			if (T.intersect(nv, center, buf)) {
				double f = fabs(T.intersection_dist(buf) / v.length());
				if (f < R) R = f;
				ever_crossed = true;
			}
		}
	}
	// if the radius is too small or no ray has crossed the geometry, discard the
	// inscribed cube by making it very small
	if (R < 0.01 || !ever_crossed) R = 0.01;
	bbox.recalc();
	Vector vr(R,R,R);
	bbox.add(center + vr);
	bbox.add(center - vr);
}

const char * InscribedCube::get_name(void) const
{
	return "inscribed cube";
}

void InscribedCube::scale(double factor)
{
	bbox.scale(factor, center);
}

void InscribedCube::translate(const Vector& trans)
{
	bbox.translate(trans);
}

/** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Old Vanilla C functions                                                  *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/

void mesh_frame_init(const Vector & camera, const Vector & light)
{
	for (int i = 0; i < mesh_count; i++) {
		if (mesh[i].get_flags() & NORMAL_MAP)
			mesh[i].rebuild_normal_map();
	}
}

void mesh_close(void)
{
#ifdef DEBUG
	printf("l_all_tests = %d, l_all_success = %d\n", l_all_tests, l_all_success);
	printf("sd_all_tests = %d, sd_all_success = %d\n", sd_all_tests, sd_all_success);
#endif
	for (int i = 0; i < mesh_count; i++) {
		if (mesh[i].image)
			delete mesh[i].image;
		if (mesh[i].normal_map)
			sse_free(mesh[i].normal_map);
		if (mesh[i].edges)
			delete [] mesh[i].edges;
		if (mesh[i].iprimitive)
			delete mesh[i].iprimitive;
		mesh[i].iprimitive = NULL;
		if (mesh[i].sdtree)
			delete mesh[i].sdtree;
		mesh[i].sdtree = NULL;
		mesh[i].num_edges = 0;
		mesh[i].image = NULL;
		mesh[i].normal_map = NULL;
		mesh[i].triangle_count = 0;
		mesh[i].flags = 0;
		mesh[i].map_size = 0;
		memset(mesh[i].file_name, 0, sizeof(mesh[i].file_name));
		mesh[i].total_scale = 0;
		mesh[i].total_translation.zero();
		
	}
	mesh_count = 0;
}
