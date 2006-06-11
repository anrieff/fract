/***************************************************************************
 *   Copyright (C) 2006 by Veselin Georgiev                                *
 *   anrieff@mailg.com (convert to gmail)                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *  ---------------------------------------------------------------------  *
 *                                                                         *
 *    Defines SDTree class for faster ray-mesh intersection testing        *
 *                                                                         *
 ***************************************************************************/

#ifndef __SDTREE_H__
#define __SDTREE_H__

#include "bbox.h"
#include "vector3.h"
#include "triangle.h"

/**
 * @class  SDTreeNode
 * @brief  Represents a node in the SDTree (bounding box, primitive lists)
 * @date   2006-06-11
 * @author Veselin Georgiev
 * 
 * A node may be leaf or non-leaf. Leaf nodes contain indices to triangle
 * objects. Non-leaf contain pointer to subnodes, which contain possibly
 * smaller less triangles. At SDTree construction, all triangles are put
 * in a root node and it is subdivided until the resulting nodes are 
 * sufficiently small (e.g. they contain no more than 15 triangles or so)
 * Those nodes are marked as leaf and subdividing stops.
*/
class SDTreeNode {
	BBox bbox;
	int primitive_count;
	int *primitives;
	SDTreeNode *left, *right;
public:
	SDTreeNode();
	bool isleaf() const;
	
	/// @see SDTree::testintersect
	bool testintersect(const Vector& start, const Vector &dir, void *IntersectionContext);
	
	~SDTreeNode();
};

/**
 * @class  SDTree
 * @brief  Represents a SDTree
 * @date   2006-06-11
 * @author Veselin Georgiev
 * 
 * SDTree is used for fast ray-mesh intersection tests, and is faster
 * than the brute force approach when the triangle count in the mesh 
 * becomes significantly large.
 * 
 * The class is used in the following manner:
 * 1. A pointer to the triangles of the mesh is passed to init()
 * 2. The tree must be built: use build_tree() method
 * 3. Testing for intersections: use testintersect() method
 *    (it returns triangle intersection context, since it uses Triangle::intersect()
 *    for infernal calculations)
 */
class SDTree {
	SDTreeNode *root;
	Triangle *base;
	int triangle_count;
	
	void init_default();
public:
	/// Default constructor
	SDTree();
	
	/// Set the triangle base and the count on the fly
	SDTree(Triangle *base, int triangle_count);
	
	/// initialize the geometry: set up an array of triangles of size `triangle_count'
	void init(Triangle *base, int triangle_count);
	
	/// Called to build the tree
	void build_tree(void);
	
	/**
	 * Tests for intersection
	 * @param start			- The start of the ray to test with
	 * @param dir			- The ray direction
	 * @param IntersectionContext	- Data to store intersection info 
	 *	(the result format is the same as in Triangle::intersect())
	 * @returns true if intersection exists, false otherwise
	*/ 
	bool testintersect(Vector &start, Vector &dir, void *IntersectionContext);
	
	/// Destructor
	~SDTree();
	
	/// PUBLIC DATA:
	/// These parameters control tree building (you may tweak them for better perf.)
	/// Or you can just use the default values.
	int max_tree_depth;	// default value : 60
	int max_tris_per_leaf;	// default value : 15
};


#endif //__SDTREE_H__
