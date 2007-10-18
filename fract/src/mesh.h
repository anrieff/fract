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

#ifndef __MESH_H__
#define __MESH_H__

#include "MyLimits.h"
#include "bbox.h"
#include "bitmap.h"
#include "common.h"
#include "object.h"
#include "triangle.h"
#include "vector3.h"
#include "sdtree.h"

#define fmax(a,b) ((a)>(b)?(a):(b))

enum {
	TYPE_OR,
	TYPE_SET
};

/**
 * @class	Inscribed
 * @brief	Represents a simple primitive, inscribed in a more complex one
 * @author	Veselin Georgiev
 * @date	2006-06-10
*/
struct Mesh;
class Inscribed {
public:

	/// gets the volume of the primitive
	virtual double volume(void) const = 0;
	
	/// Given a mesh, recalculate the position and volume of the primitive
	virtual void recalc(Mesh *) = 0;
	
	/// Try intersecting the primitive with a ray (very fast)
	/// @param start - the beginning of the ray
	/// @param dir   - the direction of the ray (must be unitary)
	virtual bool testintersect(const Vector &start, const Vector &dir) = 0;
	
	/// Get the name of the inscribing primitive
	virtual const char* get_name(void) const = 0;
	
	/// Scale the primitive with that factor
	virtual void scale(double factor) = 0;
	
	/// Translate the primitive with the given movement vector
	virtual void translate(const Vector & movement) = 0;
	
	virtual ~Inscribed() {}

	void * operator new(size_t);
	void operator delete(void *);
};

class InscribedSphere: public Inscribed {
	Vector center;
	double R;
	double planedist(Triangle&);
public:
	double volume(void) const;
	InscribedSphere(Mesh *);
	void recalc(Mesh *);
	bool testintersect(const Vector & start, const Vector &dir);
	const char *get_name(void) const;
	void scale(double factor);
	void translate(const Vector &v);
};

class InscribedCube: public Inscribed {
	Vector center;
	BBox bbox;
	double R;
public:
	double volume(void) const;
	InscribedCube(Mesh *);
	void recalc(Mesh *);
	bool testintersect(const Vector & start, const Vector &dir);
	const char *get_name(void) const;
	void scale(double factor);
	void translate(const Vector &v);
};

/**
 * @class Mesh.
 * @brief Represents a classic triangle mesh
 * @author Veselin Georgiev
 * @date 2005-12-12
*/
struct Mesh : public BBox, public ShadowCaster {
	int triangle_count;
	int flags;
	int map_size;
	float glossiness;
	Vector *normal_map;
	RawImg *image;
	Vector center;
	Inscribed *iprimitive;
	SDTree *sdtree;
	
	struct EdgeInfo {
		int ai, bi, nc;
		Vector a, b;
		Vector norm1, norm2;
		void * operator new[] (size_t);
		void operator delete [] (void *);
	};
	
	EdgeInfo *edges;
	int num_edges;
	
	/// Initializes per-scene stuff.
	void scene_init(void);

	/// get the index (in the trio[] array) of the first triangle of the mesh
	int get_triangle_base() const;

	/// reads a mesh from a simplistic text file (see mesh.cpp for format details)
	void read_from_text_file(const char *fn, bool use_mapping_coords = false);

	/// reads a mesh from a Wavefront .obj file
	/// @returns true on success, false otherwise
	bool read_from_obj(const char *fn);

	/// scales the mesh around its center
	void scale(double factor);

	/// moves the mesh
	void translate(const Vector& transp);

	/// bound the mesh within [minv..maxv] with respect to the compo
	/// component of the vector.
	///
	/// compo = 0 -- X
	/// compo = 1 -- Y
	/// compo = 2 -- Z
	/// if the alloted size (maxv-minv) is insufficient, the object
	/// is moved indefinitely and false is returned 
	bool bound(int compo, double minv, double maxv);

	/// sets the flags, reflection and opacity on all mesh's triangles
	/// refl and opacity might be -1, which means "don't change"
	void set_flags(Uint32 newflags, float refl = -1, float opacity = -1, int set_type = TYPE_OR,
		Uint32 color = 0x1000000);
	
	/// gets the flags of the first triangle (effectively, the flags
	/// of the whole mesh
	Uint32 get_flags(void) const;
	
	/// Rebuilds the normal map. Works ONLY if the object has been loaded from
	/// .OBJ file
	void rebuild_normal_map(void);

	/// Checks if the ray, starting at start, crosses any triangle of the mesh
	/// The optional parameter opt is an optimization hint and should be in [0..15]
	bool fullintersect(const Vector &start, const Vector & dir, int opt = 0);
	
	// From ShadowCaster
	bool sintersect(const Vector &start, const Vector & dir, int opt = 0);

	char file_name[64];
	Vector total_translation;
	double total_scale;

private:
	bool parse_mtl_lib(const char *fn, const char *base_dir);
	void recalc(void);
	int lastind[16];
};
extern Mesh mesh[MAX_OBJECTS];
extern int mesh_count;


class TriangleIterator {
	int obj, tri;
	
public:
	TriangleIterator()
	{
		tri = obj = 0;
	}
	TriangleIterator(int num)
	{
		obj = (num & OBJ_ID_MASK) >> OBJ_ID_BITS;
		tri = num & TRI_ID_MASK;
	}
	inline void operator ++ (void)
	{
		++tri;
		if (tri >= mesh[obj].triangle_count) {
			++obj;
			tri = 0;
		}
	}
	inline Triangle* current(void) const
	{
		if (obj >= mesh_count)
			return NULL; // cause SIGSEGV
		else
			return trio + (obj*MAX_TRIANGLES_PER_OBJECT) + tri;
	}

	inline Triangle* operator->(void) const
	{
		if (obj >= mesh_count)
			return NULL; // cause SIGSEGV
		else
			return trio + (obj*MAX_TRIANGLES_PER_OBJECT) + tri;
	}

	inline int get_id(void) const
	{
		return (obj*MAX_TRIANGLES_PER_OBJECT) + tri;
	}

	inline int group_id(void) const
	{
		return obj;
	}

	inline void group_skip(void)
	{
		obj ++;
		tri = -1;
	}

	inline bool ok() const { return obj < mesh_count; }
};

void mesh_frame_init(const Vector & camera, const Vector & light);
void mesh_scene_init(void); // prepare all meshes for per-scene stuff
void mesh_close(void);

#endif // __MESH_H__
