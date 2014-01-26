/***************************************************************************
 *   Copyright (C) 2007 by Veselin Georgiev                                *
 *   anrieff@mailg.com (convert to gmail)                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 * ----------------------------------------------------------------------- *
 * Support for SOR (Surface of revolution) object type.                    *
 *                                                                         *
 ***************************************************************************/

#include "MyTypes.h"
#include "bitmap.h"
#include "object.h"
#include "bbox.h"

#define MAX_SORS 10

/**
 * @class CubicSpline
 * @brief a Helper class that represents the profile of the SOR
 * @author Veselin Georgiev
 * @date 2007-09-26
 *
 * A helper program, `spliner' is used to design SORs. They are created by
 * specifying the control points of the cubic spline. Then, spliner calculates
 * the cubic coefficients for each spline part and writes them to a .sor file.
 * The fract's .fsv reader calls Sor::read_from_file to input those
 * coefficients
 */
struct CubicSpline {
	double *coeffs;
	double *x;
	int n;
		
	CubicSpline();
	~CubicSpline();
	double eval(double X);
	bool in_range(double X);
	double eval(int i, double X);
	double eval_deriv(double X);
	double max_radius(double, double);
};

/**
 * @class Sor
 * @brief Represents a Surface Of Revolution (sor) object
 * @author Veselin Georgiev
 * @date 2007-09-26
 *
 * In detail: A SOR object is created by revolving a 2D profile around some
 * axis (in the case of fract, around the Y axis). The resulting 3D object
 * is entirely symmetric around Oy, and is useful for lightweight
 * representation of glasses, vases and other everyday objects.
 *
 * Most of the SOR's properties depend entirely on the method of
 * specifying the 2D profile - in the case of fract, cubic natural splines
 * are used for this. The generated surfaces are smooth and intersecting them
 * is not computationaly expensive - usually, a few cubic polynomials must be
 * solved per intersection.
 *
 * Fract simplifies creation of, e.g. wine glasses, by allowing multiple
 * splines per SOR to be used. Every even-numbered spline is a "outer"
 * shell, meaning that intersection may only occur from the "outside"
 * to the "inside" of the profile. Every odd-numbered spline is an
 * "inner" shell. Thus, creating a SOR for a wine glass is as simple as
 * creating two splines - the first describing the outer profile of the
 * glass, the second describing the inner profile (the fluid-holding part
 * of the glass).
 *
 */

class Sor : public Object, public ShadowCaster
{
	double start, end;
	Vector movement;
	double scale;
	double miny, maxy;
	double max_radius;
	CubicSpline *spline;
	int spline_count;
	Uint32 color;
	float refl, opacity;
	RawImg *texture;
	BBox bbox;
public:
	Sor();
	~Sor();
	// FROM Object:
	double get_depth(const Vector &camera);
	bool is_visible(const Vector & camera, Vector w[3]);
	int calculate_convex(Vector pt[], const Vector& camera);
	void map2screen(Uint32 *framebuffer, int color, int sides, Vector pt[], const Vector& camera, Vector w[3], int & min_y, int & max_y);
	bool fastintersect(const Vector& ray, const Vector& camera, const double& rls, void *IntersectContext) const;
	bool intersect(const Vector& ray, const Vector &camera, void *IntersectContext);
	double intersection_dist(void *IntersectContext) const;
	Uint32 shade(Vector& ray, const Vector& cam, const Vector& light, double rr, float &o, void *ic, int iteration, FilteringInfo& finfo);
	int get_best_miplevel(double x0, double z0, FilteringInfo & fi);
	OBTYPE get_type(bool generic = true) const;
	
	// FROM ShadowCaster:
	bool sintersect(const Vector & start, const Vector & dir, int opt = 0);
	
	// Interface for initialization/manipulation
	bool read_from_file(const char *filename);
	void move(Vector movement);
	void set_scale(double scaler);
	void set_reflectivity(float f);
	void set_opacity(float f);
	void set_color(int value, char which);
	bool set_texture(const char * texture_file_name);
	
	// must be called prior to rendering a frame to setup some stuff.
	void perframe_init(void);
	//
	int flags;
};

extern Sor sor[MAX_SORS];
extern int sorcount;
