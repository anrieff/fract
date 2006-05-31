/***************************************************************************
 *   Copyright (C) 2004 by Veselin Georgiev                                *
 *   vesko@ViruS                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef __INFINITE_PLANE_H__
#define __INFINITE_PLANE_H__
 
#include "MyGlobal.h"
#include "MyTypes.h"
#include "scene.h"
#include "sphere.h"
#include "vector3.h"

enum lectionTypes {
	REFLECTION,
	REFRACTION
};

struct ML_Entry {
	void *obj;
	int	value;
};

struct FilteringInfo {
	Vector camera, xinc, yinc;
	Vector through;
	Vector IP;
	lectionTypes lectionType;
	ML_Entry *ml;
	int last_val;
	FilteringInfo()
	{
		last_val = LOG2_MAX_TEXTURE_SIZE;
	}
};

extern FilteringInfo def_finfo;
struct sphere;
Uint32 texture_handle_nearest (int x, int y, int miplevel, int ysqrd_raytrace);
Uint32 texture_handle_bilinear(double x, double y, int miplevel, int ysqrd_raytrace);
Uint32 bilinea_p5(Uint32 x0y0, Uint32 x1y0, Uint32 x0y1, Uint32 x1y1, unsigned Fx, unsigned Fy);

// renders the infinite plane. Puts data in frame buffer; the rendering
// resolution is xr by yr pixels; `tt' determines topleft corner of the
// vector grid. `ti' is column vector increment, `tti' is row increment.
// In multithreaded execution, `start_line' determines the thread's number
void render_infinite_plane(Uint32 *frame_buffer, int xr, int yr, Vector& tt, Vector& ti, Vector& tti, int start_line);

#endif // __INFINITE_PLANE_H__
