/***************************************************************************
 *   Copyright (C) 2006 by Veselin Georgiev                                *
 *   anrieff@mailg.com (convert to gmail)                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "sdtree.h"
#include "array.h"
#include "random.h"
#include "common.h"
#include "memory.h"
#include "progress.h"


/** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * @class SDTreeNode                                              *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
 
SDTreeNode::SDTreeNode()
{
	primitives = NULL;
	primitive_count = 0;
	left = right = NULL;
}

bool SDTreeNode::isleaf() const
{
	return primitives != NULL;
}

SDTreeNode::~SDTreeNode()
{
	if (primitives) {
		delete [] primitives;
		primitives = NULL;
	} else {
		delete left;
		delete right;
		left = right = NULL;
	}
}

void SDTreeNode::scale(double x, const Vector& center)
{
	bbox.scale(x, center);
	if (left)
		left->scale(x, center);
	if (right)
		right->scale(x, center);
}

void SDTreeNode::translate(const Vector &v)
{
	bbox.translate(v);
	if (left)
		left->translate(v);
	if (right)
		right->translate(v);
}

int SDTreeNode::testintersect(Triangle *base, const Vector& start, const Vector &dir, void *ctx)
{
	if (primitives) {
		int bi = -1;
		double mdist = 0.0;
		for (int i = 0; i < primitive_count; i++) {
			Triangle& t = base[primitives[i]];
			if (t.intersect(dir, start, ctx)) {
				if (bi == -1) {
					bi = i;
					mdist = t.intersection_dist(ctx);
				} else {
					double temp = t.intersection_dist(ctx);
					if (temp < mdist) {
						mdist = temp;
						bi = i;
					}
				}
			}
		}
		if (bi == -1) return -1;
		else {
			base[primitives[bi]].intersect(dir, start, ctx);
			return primitives[bi];
		}
	} else {
		SDTreeNode *nodes[2] = {left, right};
		double dists[2] = {1e99, 1e99};
		if (left->bbox.testintersect(start, dir))
			dists[0] = left->bbox.intersection_dist(start, dir);
		if (right->bbox.testintersect(start,dir))
			dists[1] = right->bbox.intersection_dist(start, dir);
		if (dists[0] > dists[1]) {
			double t = dists[0]; dists[0] = dists[1]; dists[1] = t;
			SDTreeNode *n = nodes[0]; nodes[0] = nodes[1]; nodes[1] = n;
		}
		if (dists[0] < 1e90) {
			int r = nodes[0]->testintersect(base, start, dir, ctx);
			if (r != -1) return r;
		}
		if (dists[1] < 1e90) {
			int r = nodes[1]->testintersect(base, start, dir, ctx);
			if (r != -1) return r;
		}
		return -1;
	}
}

void * SDTreeNode::operator new(size_t size) { return sse_malloc(size); }
void SDTreeNode::operator delete(void * what) { sse_free(what); }

/** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * @class SDTree                                                  *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

SDTree::SDTree()
{
	init_default();
}

SDTree::SDTree(Triangle *b, int c)
{
	init_default();
	init(b, c);
}

SDTree::~SDTree()
{
	if (root)
		delete root;
	root = NULL;
}

void SDTree::init_default(void)
{
	root = NULL;
	base = NULL;
	triangle_count = 0;
	max_tree_depth = DEFAULT_MAX_TREE_DEPTH;
	max_tris_per_leaf = DEFAULT_MAX_TRIS_PER_LEAF;
	subdivision_tests = DEFAULT_SUBDIVISION_TESTS;
}

void SDTree::init(Triangle xbase[], int xcount)
{
	base = xbase;
	triangle_count = xcount;
}

void SDTree::build_tree(void)
{
	Task task(TREEBUILD, __FUNCTION__);
	task.set_steps(32);
	if (!base || !triangle_count) return;
	if (root)
		delete root;
	root = NULL;

	stat_nodes = stat_leafs = stat_maxdepth = stat_largest_leaf = 0;
	Array<int> a;
	root = new SDTreeNode;
	for (int i = 0; i < triangle_count; i++) {
		a += i;
		root->bbox.add(base[i]);
	}
	build(root, a, 0, task);
#ifdef DEBUG
	printf("SDTree statistics:\n");
	printf("\tMax depth: %d\n", stat_maxdepth);
	printf("\tNodes: %d\n", stat_nodes);
	printf("\tLeafs: %d\n", stat_leafs);
	printf("\tMax triangles in leaf node: %d\n", stat_largest_leaf);
	printf("\tAvg. triangles per leaf: %.2lf\n", (double) triangle_count / stat_leafs);
#endif
}

void SDTree::build(SDTreeNode *node, const Array<int>& a, int depth, Task & task)
{
	stat_nodes++;
	if (depth == 5) ++task;
	if (depth > stat_maxdepth) stat_maxdepth = depth;
	if (depth > max_tree_depth || a.size() <= max_tris_per_leaf) {
		// make a leaf node
		stat_leafs++;
		if (a.size() > stat_largest_leaf)
			stat_largest_leaf = a.size();
		int allocsize = a.size();
		if (allocsize < 2) allocsize = 2;
		node->primitives = new int[allocsize];
		node->primitive_count = a.size();
		for (int i = 0; i < a.size(); i++)
			node->primitives[i] = a[i];
	} else {
		BBox l, r;
		
		//
		int bestdiff = inf;
		double bestdiv = 0.5;
		for (int i = 0; i < subdivision_tests; i++) {
			double d = drandom();
			r = l = node->bbox;
			int result = testdivision(a, l, r, d, depth % 3);
			if (result < 0) result = -result;
			if (result < bestdiff) {
				bestdiff = result;
				bestdiv = d;
			}
		}
		
		//
		Array<int> xa, xb;
		r = l = node->bbox;
		testdivision(a, l, r, bestdiv, depth % 3, &xa, &xb);
		node->left = new SDTreeNode;
		node->right = new SDTreeNode;
		node->left->bbox = l;
		node->right->bbox = r;
		
		build(node->left, xa, depth + 1, task);
		build(node->right, xb, depth + 1, task);
	}
}

/** Tries to divide the current node.
 * @param in	the triangles to test against
 * @param l	the resulting bbox of the left subnode
 * @param r	the resulting bbox of the right subnode
 * @param x	where to divide [0 .. 1]
 * @param dim	which dimension to divide [0..2]
 * @param la	optional - where to put the triangles, belonging to the left subnode
 * @param lb	optional - where to put the triangles, belonging to the right subnode
 * 
 * @returns The difference of the load of the left and right subnode (e.g. 0 for balanced load)
*/ 
int SDTree::testdivision(const Array<int>& in, BBox &l, BBox &r, double x, int dim, Array<int> *la, Array<int> *ra)
{
	double edge = l.vmin[dim] + x * (l.vmax[dim] - l.vmin[dim]);
	
	l.vmax[dim] = edge;
	r.vmin[dim] = edge;
	
	int left_load = 0, right_load = 0;
	
	for (int i = 0; i < in.size(); i++) {
		Triangle& t = base[in[i]];
		if (l.triangle_intersect(t)) {
			++left_load;
			if (la) la->add(in[i]);
		}
		if (r.triangle_intersect(t)) {
			++right_load;
			if (ra) ra->add(in[i]);
		}
	}
	
	return left_load - right_load;
}

void SDTree::scale(double scalar, const Vector & center)
{
	if (root)
		root->scale(scalar, center);
}

void SDTree::translate(const Vector& trans)
{
	if (root)
		root->translate(trans);
}

bool SDTree::testintersect(const Vector &start, const Vector &dir, void *ctx, Triangle** t)
{
	if (root) {
		int r = root->testintersect(base, start, dir, ctx);
		if (r != -1)  {
			if (t) *t = base + r;
			return true;
		}
	}
	return false;
}

