/***************************************************************************
 *   Copyright (C) 2004 by Veselin Georgiev                                *
 *   vesko@ViruS                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "MyGlobal.h"
#include "MyTypes.h"
#include "sphere.h"
#include "vector2f.h"
#include "vector3.h"

#define vfabs(x) (((x)<0)?-(x):(x))
#define FONT_XSIZE 10.66666666666
#define FONT_YSIZE 18
#define FONT_XSIZE_FLOOR 10
#define FONT_XSIZE_CEIL (FONT_XSIZE_FLOOR+1)

#define PROG_FRAME_COLOR  0x8c8d90
#define PROG_INNER_COLOR1 0x6488af
#define PROG_INNER_COLOR2 0x698eb5
#define PROG_TEXT_COLOR PROG_INNER_COLOR1
#define PROG_XSPACE 16
#define PROG_YSPACE 6


 // this should be 1/22
 #define LIGHTCONST 0.045454545455
 #define __1_22 LIGHTCONST

 // a resolution is accepted if the number of columns is a multiple of eight.
 #define RESOLUTION_X_ALIGN 8

extern double PreSC[MAX_SPHERE_SIDES+1][MAX_SPHERE_SIDES+1][2];

#define ISSET(buffer, __x, __y, __xres) \
	(0!=(buffer[(__x + __y * __xres) >> 5] & (1u << ((__x + __y * __xres)&31))))


typedef Uint32 (*_blend_fn) (Uint32, Uint32, float);

#ifdef ACTUALLYDISPLAY
 void surface_lock(SDL_Surface *screen);
 void surface_unlock(SDL_Surface *screen);
 void paste_raw(SDL_Surface *p, const RawImg& a, int x, int y);
 void intro_progress(SDL_Surface *p, double prog);
 void intro_progress_init(SDL_Surface *p, char * message);
 void printxy(SDL_Surface *p, Uint32 *a, const RawImg& font, int x, int y, Uint32 col, float opacity, const char *buf, ...);
#endif

 int xres(void);
 int yres(void);
 void set_new_videomode(int x, int y);
 void gfx_update_2nds(void);
 void draw_pixel(Uint32 *p, int x, int y, Uint8 r, Uint8 g, Uint8 b);
 void draw_pixeld(Uint32 *p, int x, int y, Uint32 v);
 Uint32 get_pixeld(Uint32 *p, int x, int y);
 void draw_transparent(Uint32 *p, int x, int y, Uint32 col, float alpha);
 void w_line(Uint32 *p, int x1, int y1, int x2, int y2, Uint32 col);
 Uint32 multiplycolor(Uint32 color, int amp);
 Uint32 multiplycolorf(Uint32 color, float amp);
 Uint32 desaturate(Uint32 color, double factor);
 //Uint32 blend(Uint32 foreground, Uint32 background, float opacity);
 extern _blend_fn blend;
 Uint32 blend_sse(Uint32 foreground, Uint32 background, float opacity);
 Uint32 blend_p5(Uint32 foreground, Uint32 background, float opacity);
 int project_it(Object *a, 
	       Vector pt[], 
	       int *ns, 
	       Uint32 *fb, 
	       Vector& cur, 
	       Vector w[3], 
	       int xres, 
	       int yres, 
	       int color, 
	       int & min_y, 
	       int & max_y);
 void gfx_init (void);
 void gfx_close(void);

int fun_min(int a, int b);
int fun_max(int a, int b);
bool fun_less(int a, int b);
bool fun_more(int a, int b);
int truncate(float x);
int fround(float x);

void project_hull_part(int *hull, Vector pt[], int start, int incr, int count, int sides, int color, Vector c, Vector w[3], int (*fun) (int,int), int (*rounding_fn) (float) = truncate, int yres = -1);
void map_hull(Uint32 *fb, int *left, int *right, int ys, int ye, int color, int bias = 0, int xres=-1, int yres=-1);
int accumulate(Vector pt[], int sides, Vector c, Vector w[3], bool (*fun) (int, int), int start_val, int & bi);
void map_to_screen(Uint16 *sb, Vector pt[], int sides, Uint16 color, Vector& cur, Vector w[3], int xr, int yr);
void outline_to_screen(Uint16 *sbuffer, Vector pt[], int sides, Uint16 color, Vector &cur, Vector w[3], int xres, int yres);


struct AbstractDrawer {
	virtual inline bool draw_line(int,int,int) { return false; }
};

/**
 * @class	TriangleRasterizer
 * @date	2006-05-06
 * @author	Veselin Georgiev
 * @brief	A generic class for rasterizing triangles on screen
 * 
 * The class determines which pixels of the screen are to be drawn. In fact,
 * it assembles a list of ranges for each screen row and passes them to 
 * the custom draw routine. This class accepts another one, which it
 * calls back for each screen line, passing the range for the screen line to
 * be drawn.
 * The exact definition of the callback must be:
 *
 *    inline bool draw_line(int x, int y, int size)
 *
 *    where
 *      x - the x coordinate of the start of the range
 *      size - the size of the range (> 0)
 *      y - the y coordinate of the current line
 *      if the return value is false, the rasterization halts
 *
 * @see @class AbstractDrawer for more info
*/
class TriangleRasterizer {
	int xr, yr;
	int ymin, ymax;
	int xmin, xmax;
	vec2f p[3];

	static inline void swap2f(vec2f & a, vec2f & b) {
		vec2f c = a;
		a = b;
		b = c;
	}
public:
	/**
	*** TriangleRasterizer constructor
	*** @param xres - the screen resolution (columns)
	*** @param yres - the screen resolution (rows)
	*** @param a, b, c - the verts of the triangle
	**/
	TriangleRasterizer(int xres, int yres, const vec2f& a, const vec2f& b, const vec2f& c)
	{
		ymin = inf; ymax = -inf;
		xr = xres; yr = yres;
		p[0] = a;
		p[1] = b;
		p[2] = c;
		if (p[1] < p[0]) swap2f(p[0], p[1]);
		if (p[2] < p[1]) swap2f(p[1], p[2]);
		if (p[1] < p[0]) swap2f(p[0], p[1]);
		xmax = xmin = (int) round(p[0][0]);
		for (int i = 1; i < 3; i++)
		{
			int t = (int) round(p[i][0]);
			if (t > xmax) xmax = t;
			if (t < xmin) xmin = t;
		}
		if (xmin < 0) xmin = 0;
		if (xmax >= xr) xmax = xr - 1;
	}
	void draw(AbstractDrawer & drawer)
	{
	//	if (p[0][1] == p[2][1] || p[0][1] == p[1][1] || p[1][1] == p[2][1]) return;
		ymin = (int) round(p[0][1]);
		ymax = (int) round(p[2][1]);
		if (ymin >= yr) return;
		if (ymax < 0) return;
		if (ymin < 0) ymin = 0;
		if (ymax >= yr) ymax = yr-1;
		float m01 = (p[1][0]-p[0][0])/(p[1][1]-p[0][1]);
		float m02 = (p[2][0]-p[0][0])/(p[2][1]-p[0][1]);
		float m12 = (p[2][0]-p[1][0])/(p[2][1]-p[1][1]);
		
		int x0, x1;
		for (int y = ymin; y <= ymax; y++) {
			x0 = (int) (p[0][0] + (y - p[0][1])*m02);
			if ( y < p[1][1] )
				x1 = (int) (p[0][0] + (y - p[0][1])*m01);
			else
				x1 = (int) (p[1][0] + (y - p[1][1])*m12);
			if (x1 < x0) {
				int t = x1; x1 = x0; x0 = t;
			}
			if (x0 >= xr) continue;
			if (x1 < 0) continue;
			if (x0 < 0) x0 = 0;
			if (x1 >= xr) x1 = xr - 1;
			if (x0 < xmin) x0 = xmin;
			if (x1 > xmax) x1 = xmax;
			drawer.draw_line(x0, y, x1-x0+1);
		}
	}
	inline void update_limits(int & _xmin, int & _xmax, int & _ymin, int & _ymax)
	{
		if (xmin != +inf  && _xmin > xmin) _xmin = xmin;
		if (ymin != +inf  && _ymin > ymin) _ymin = ymin;
		if (xmax != -inf && _xmax < xmax) _xmax = xmax;
		if (ymax != -inf && _ymax < ymax) _ymax = ymax;
	}
};
