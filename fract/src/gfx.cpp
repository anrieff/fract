/***************************************************************************
 *   Copyright (C) 2004 by Veselin Georgiev                                *
 *   vesko@ViruS                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *-------------------------------------------------------------------------*
 *                                                                         *
 *   gfx.cpp - contains all graphic operations that could be used, namely  *
 *             single pixel operations, color multiplications, flood fill  *
 *             and even sphere pre-filler to speed the things up.          *
 *                                                                         *
 ***************************************************************************/
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "MyGlobal.h"
#include "MyTypes.h"
#include "antialias.h"
#include "common.h"
#include "cpu.h"
#include "gfx.h"
#include "vectormath.h"

// this is 1/3.5
#define CRITSTR_UPDATE 0.2857
#define QUEUESIZE 1200

const double nocalcthresh = 10;
const double CircConst    = 0.0000390625;

double lastgX=-9999999.0, lastgY=-9999999.0, lastr;

// default resolution:
int Xres=640, Yres=480;
int Xres2, Yres2;
extern int camera_moved;
extern int RowMin[], RowMax[];
extern Font font0;

static inline int inrange(int x, int y)
{
 return ((unsigned) x< (unsigned)Xres && (unsigned) y<(unsigned)Yres);
}

static inline int inrange2(int x, int y)
{
 return ((unsigned) x<(unsigned)Xres2 && (unsigned) y<(unsigned)Yres2);
}

// PreSC[i][j][0] = sin(j/i*2*pi)
// PreSC[i][j][1] = cos(j/i*2*pi)
double PreSC[MAX_SPHERE_SIDES+1][MAX_SPHERE_SIDES+1][2];

#ifdef ACTUALLYDISPLAY
void surface_lock(SDL_Surface *screen)
{
  if ( SDL_MUSTLOCK(screen) )
  {
    if ( SDL_LockSurface(screen) < 0 )
    {
      return;
    }
  }
}

void surface_unlock(SDL_Surface *screen)
{
  if ( SDL_MUSTLOCK(screen) )
  {
    SDL_UnlockSurface(screen);
  }
}
#endif

int xres(void){
	return Xres;
}

int yres(void){
	return Yres;
}

void set_new_videomode(int x, int y)
{
 if (x%RESOLUTION_X_ALIGN) {
 	printf("I can't set the desired resolution (%dx%d)\n - setting X to the previous multiple-of-%d value\n",
	x, y, RESOLUTION_X_ALIGN);
	x&=~(RESOLUTION_X_ALIGN-1);
 }
 y = x/4*3;
 Xres = x;
 Yres = y;
 Xres2 = xsize_render(x);
 Yres2 = ysize_render(y);
 if (Xres2 > RES_MAXX || Yres2 > RES_MAXY) {
 	set_fsaa_mode(FSAA_MODE_NONE);
 	Xres2 = xsize_render(x);
 	Yres2 = ysize_render(y);
 	}
}

void gfx_update_2nds(void)
{
 Xres2 = xsize_render(Xres);
 Yres2 = ysize_render(Yres);
}

static inline int intensity(Uint32 color)
{
	return (color & 0xff) + ((color >> 8) & 0xff) + ((color >> 8) & 0xff);
}


void draw_pixel(Uint32 *p, int x, int y, Uint8 r, Uint8 g, Uint8 b)
{
 Uint32 *bfp;
 bfp = p + y*Xres + x;
 *bfp = (int) b + (((int)g)<<8) + (((int)r)<<16);
}

void draw_pixeld(Uint32 *p, int x, int y, Uint32 v)
{
 Uint32 *bfp;
 bfp = p + y*Xres + x;
 *bfp = v;
}

Uint32 get_pixeld640(Uint32 *p, int x, int y)
{
 Uint32 *bfp;
 bfp = p + y*640 + x;
 return ((*bfp));
}

void draw_pixel640(Uint32 *p, int x, int y, Uint8 r, Uint8 g, Uint8 b)
{
 Uint32 *bfp;
 bfp = p + y*640 + x;
 *bfp = (int) b + (((int)g)<<8) + (((int)r)<<16);
}

void draw_pixeldsafe(Uint32 *p, int x, int y, Uint32 v)
{
 Uint32 *bfp;
 if ((unsigned) x >= (unsigned)Xres || (unsigned) y >= (unsigned)Yres)
 	return;
 bfp = p + y*Xres + x;
 *bfp = v;
}

void draw_pixeldxsafe(Uint32 *p, int x, int y, Uint32 v)
{
 Uint32 *bfp;
 if ((unsigned) x >= (unsigned)Xres2 || (unsigned) y >= (unsigned)Yres2)
 	return;
 bfp = p + y*Xres2 + x;
 *bfp = ((*bfp)<<16) + v;
}


Uint32 get_pixeld(Uint32 *p, int x, int y)
{
 Uint32 *bfp;
 bfp = p + y*Xres + x;
 return (*bfp);
}

void draw_pixeldx(Uint32 *p, int x, int y, Uint32 v)
{
 Uint32 *bfp;
 bfp = p + y*Xres2 + x;
 *bfp = ((*bfp)<<16) + v;
}


Uint32 get_pixeldx(Uint32 *p, int x, int y)
{
 Uint32 *bfp;
 bfp = p + y*Xres2 + x;
 return ((*bfp)&0xffff);
}


void line_fast( Uint32 *p, int x0, int y0, int x1, int y1, Uint32 color)
{
	int x, y, i, steps, xi, yi, x_int, y_int;
	
	x = abs(x1-x0); y = abs(y1-y0);
	if (x>y) steps = x; else steps = y;
	if (!steps) steps =1 ;
	
	xi = (int) (65536.0 * ((double)(x1-x0)/(double)steps));
	yi = (int) (65536.0 * ((double)(y1-y0)/(double)steps));
	x = x0 << 16;
	y = y0 << 16;
	for (i=1;i<=steps;i++) {
		x_int = x>>16;
		y_int = y>>16;
		if ((unsigned) y_int < (unsigned) Yres2) {
			draw_pixeldxsafe(p, x_int, y_int, color);
			if (x_int > RowMax[y_int])
				RowMax[y_int] = x_int;
			if (x_int < RowMin[y_int])
				RowMin[y_int] = x_int;
		}
		x += xi;
		y += yi;
	}
}




// draws transparent pixel at X, Y
// with color col and transparency factor alpha
void draw_transparent(Uint32 *p, int x, int y, Uint32 col, float alpha)
{Uint32 z;
 if ((unsigned)x>=(unsigned)Xres||(unsigned)y>=(unsigned)Yres) return;
 z = get_pixeld(p, x, y);
 draw_pixel(p, x, y, (int) (alpha * ((col>>16)& 0xff) + (1-alpha) * ((z >> 16)& 0xff)),
                    (int) (alpha * ((col>>8)& 0xff) + (1-alpha) * ((z >> 8)& 0xff)),
		    (int) (alpha * (float) (col&0xff) + (1-alpha) * (float) (z& 0xff)));
}

void w_line(Uint32 *p, int x1, int y1, int x2, int y2, Uint32 col)
{int i, steps, v1, v2;
 float x, xi=0, y, yi=0;
 v1 = x1-x2;
 v2 = y1-y2;
 if (v1<0) v1 = -v1;
 if (v2<0) v2 = -v2;
 steps = (v1>v2?v1:v2);
 if (steps) {
    xi = (x2-x1)/(float)steps;
    yi = (y2-y1)/(float)steps;
    }
 if (v1<v2) {
    for (i=0,x=x1,y=y1;i<=steps;i++,x+=xi,y+=yi) {
	draw_transparent(p, (int) floor(x), (int) y, col, ceil(x)-x);
	draw_transparent(p, (int) ceil(x), (int) y, col, x-floor(x));
	}
    } else
    {
    for (i=0,x=x1,y=y1;i<=steps;i++,y+=yi,x+=xi) {
	draw_transparent(p, (int) x, (int) floor(y), col, ceil(y)-y);
	draw_transparent(p, (int) x, (int) ceil (y), col,y-floor(y));
	}
    }
}

#ifdef ACTUALLYDISPLAY
/* paste_raw -- puts a RawImg on a surface at coords x, y */
/* comments: really FAST */
void paste_raw(SDL_Surface *p, const RawImg &a, int x, int y)
{int i, bytx=a.get_x()*4, rowy=a.get_y();
 Uint32 *xx ;
 if (y+a.get_y()>p->h)
 	rowy = p->h - y;
 if (x+a.get_x()>p->w)
 	bytx = (p->w - x)*4;
 surface_lock(p);
 int *adata = (int*) a.get_data();
 int ax = a.get_x();
 for (i=0;i<rowy;i++)
 	{
	xx = (Uint32 *) p->pixels + y*p->pitch/4 + i*p->pitch/4 + x;
 	memcpy(xx, adata+i*ax, bytx);
	}
 surface_unlock(p);
 SDL_Flip(p);
}
/*
	Prepares the screen for the progress bar
*/
struct introstruct {
	int x, y;
	int width, height;
	int last;
};
introstruct intro;
void intro_progress_init(SDL_Surface *p, char * message)
{
	surface_lock(p);
	memset(p->pixels, 0, p->w * p->h * sizeof(Uint32));
	int xr = p->w, yr = p->h;
	intro.width  = font0.w_int() * strlen(message) + PROG_XSPACE;
	intro.height = font0.h() + PROG_YSPACE;
	if (intro.width > xr - 6)
		intro.width = xr - 6;
	intro.x = (xr - intro.width) / 2;
	intro.y = (yr - intro.height) / 2;
	Uint32 * pix = (Uint32*) p->pixels;
	for (int i = 0; i < intro.width; i++) {
		if (i == 0 || i == intro.width - 1) continue;
		int _0 = 0;
		if (i == 1 || i == intro.width - 2) {
			_0 = 1;
		}
		draw_pixeld(pix, 
			   intro.x + i,
			   intro.y + _0,
			   PROG_FRAME_COLOR);
		draw_pixeld(pix, 
			   intro.x + i,
			   intro.y + intro.height - 1 - _0,
			   PROG_FRAME_COLOR);
	}
	for (int i = 0; i < intro.height; i++) {
		if (i < 2 || i >= intro.height - 2) continue;
		draw_pixeld(pix,
			   intro.x,
			   intro.y+i,
			   PROG_FRAME_COLOR);
		draw_pixeld(pix,
			   intro.x + intro.width - 1,
			   intro.y + i,
			   PROG_FRAME_COLOR);
	}
	font0.printxy(p, pix, intro.x + PROG_XSPACE / 2, intro.y + PROG_YSPACE / 2,
	       PROG_TEXT_COLOR, 1.0f, message);
	intro.last = 0;
	surface_unlock(p);
	SDL_Flip(p);
}

/* Shows a progress bar (for the intro purposes), with a progress prog (should be within 0.0 and 1.0)
   flips every time ;( try to use as rarely as possible ) */
void intro_progress_old(SDL_Surface *p, double prog)
{
 int i, j, endx, sx=31;
 int rg = 81, b = 250, rgi=2, bi=4;
 if (prog > 1.0+1E-14) return;
 surface_lock(p);
 endx = (int) (31 + 592.0 * prog);
 for (j=233;j<=247;j++, sx--, endx--, rg-=rgi, b-=bi) {
	for (i=sx;i<=endx;i++)
		if (get_pixeld640((Uint32*)p->pixels, i, j) < 0x300000) {
			draw_pixel640((Uint32*)p->pixels, i, j, rg + rand()%5-2, rg + rand()%5-2, b + rand()%10-5);
			}
 	}
 surface_unlock(p);
 SDL_Flip(p);
}

void intro_progress(SDL_Surface *p, double prog)
{
	int ex;
	ex = (int)((intro.width - 2) * prog);
	if (prog > 1.0 + 1E-14 || ex <= intro.last) return;
	surface_lock(p);
	Uint32 *pix = (Uint32 *) p->pixels;
	for (int i = intro.last + 1; i <= ex; i++) {
		Uint32 color = (i/16)%2 ? PROG_INNER_COLOR1 : PROG_INNER_COLOR2;
		int _0 = (i == 1 || i == intro.width - 2);
		for (int j = intro.y + 1 + _0; j < intro.y + intro.height - _0; j++) {
			Uint32 old = get_pixeld(pix, intro.x + i, j);
			float rel_intensity = intensity(old) / (float)intensity(PROG_TEXT_COLOR);
			draw_pixeld(pix, intro.x + i, j, multiplycolorf(color, 1 - rel_intensity));
		}
	}
	surface_unlock(p);
	SDL_Flip(p);
	intro.last = ex;
}

#endif

#ifdef USE_ASSEMBLY
SSE_ALIGN(static float w2_flones[4]) = {1.0, 1.0, 1.0, 1.0};
SSE_ALIGN(static Uint16 ii_ones[4])  = {0xffff, 0xffff, 0xffff, 0xffff};
#endif

#define gfx_asm
#include "x86_asm.h"
#undef gfx_asm

Uint32 blend_p5(Uint32 foreground, Uint32 background, float opacity)
{
	return(multiplycolorf(foreground, opacity) + multiplycolorf(background, (1-opacity)));
}

// blends two colors : more opacity => result is closer to foreground.
// uses assembly SSE version if it is supported

// this is a pointer to function. If SSE is present, faster SSE-based version is being called
// else it falls back to the x86 version
// this disables runtime checks every time "blend" is called
_blend_fn blend = blend_p5;

// amplifies color by amp (amp=65536 (=1.0 in float) -> no change)
Uint32 multiplycolor(Uint32 color, int amp)
{int r, g, b;
 b = (amp * (color & 0xff))>>16; color>>=8;
 g = (amp * (color & 0xff))>>16; color>>=8;
 r = (amp * (color & 0xff))>>16;
 b = (b>255?255:b); g = (g>255?255:g); r = (r>255?255:r);
 return (b + (g << 8) + (r << 16));
}

// amplifies color by amp (amp=1.0 -> no change)
Uint32 multiplycolorf(Uint32 color, float amp)
{int r, g, b;
 b = (int) ((amp * (double)(color & 0xff))); color>>=8;
 g = (int) ((amp * (double)(color & 0xff))); color>>=8;
 r = (int) ((amp * (double)(color & 0xff)));
 r = (r>255?255:r); g = (g>255?255:g); b = (b>255?255:b);
 return (b + (g << 8) + (r << 16));
}

// returns the given color, desaturated to factor (0 - no desaturation. 1 - all color info is lost)
Uint32 desaturate(Uint32 color, double factor)
{int r, g, b,m;
 b = color & 0xff; color>>=8;
 g = color & 0xff; color>>=8;
 r = color & 0xff;
 m = (r+g+b)/3;
 b = b + (int)(factor * ((double)(m-b)));
 g = g + (int)(factor * ((double)(m-g)));
 r = r + (int)(factor * ((double)(m-r)));
 return (b | (g << 8) |(r<<16));
}


static inline Uint32 addrgb(Uint32 A, Uint32 B)
{int r,g,b;

 b = (A&0xff) + (B&0xff);// b = (b>255?255:b);
 g = ((A>>8)&0xff) + ((B>>8)&0xff); //g = (g>255?255:g);
 r = ((A>>16)&0xff) + ((B>>16)&0xff); //r = (r>255?255:r);

 return ((r<<16)+(g<<8) + b);

}

#ifdef ACTUALLYDISPLAY


// a printf-like procedure... prints a message at x,y with a color col and the given opacity
void Font::printxy(SDL_Surface *p, Uint32 *a, int x, int y, Uint32 col, float opacity, const char *buf, ...)
{
	va_list arg;
	static char s[1024];
	int i, fx, x0, y0;
	int ampi, indeh;
	int *fontdata = (int*) font.get_data();
	double amp;
	bool underline = false;
	
	va_start(arg, buf);
	#ifdef _MSC_VER
	_vsnprintf(s, 1024, buf, arg); // M$ sucks with those _names :)
	#else
	vsnprintf(s, 1024, buf, arg);
	#endif
	va_end(arg);
	i = 0;
	while (s[i] && i < 1024) {
		if (printxy_callback && s[i] == '[') {
			int j = i;
			while (s[j] && s[j] != ']') j++;
			if (!s[j]) break;
			char temp[32];
			s[j] = 0;
			strcpy(temp, s + i + 1);
			s[j] = ']';
			printxy_callback(col, underline, temp);
			i = j + 1;
			if (!s[i]) break;
		}
		if (s[i]>32 && s[i]<=126) {
			fx = (int) ((s[i]-33)*font_xsize_float);
			for (x0=0;x0<font_xsize_floor; x0++) {
				for (y0=0;y0<font_ysize;y0++) {
					ampi= fontdata[fx+x0 + y0*font.get_x()] & 0xff;
					amp = ampi / 255.0;
					ampi<<=8;
					if (amp>0.0005) {
						ampi = (int) (ampi* opacity);
						indeh = x + x0 + (y + y0)*Xres;
						if (inrange(x + x0, y + y0)) {
							a[ indeh ] = addrgb	(
											multiplycolor(a[ indeh ], 65535 - ampi),
											multiplycolor(col, ampi)
										);
						}
					}
				}
				y0 = font_ysize;
				if (underline) {
					if (inrange(x + x0, y + y0))
						a[x + x0 + (y + y0) * Xres] = col;
					if (inrange(x + x0 + 1, y + y0))
						a[x + x0 + 1 + (y + y0) * Xres] = col;
				}
			}
		}
		i++; x+=font_xsize_ceil;
		if (x>=Xres-1- font_xsize_ceil) break;
	}
}

void Font::set_print_callback(void (*callback) (Uint32&, bool &, const char *))
{
	printxy_callback = callback;
}

int Font::get_text_xlength(const char *s)
{
	int res = 0;
	int i = 0;
	while (s[i] && i < 1024) {
		if (printxy_callback && s[i] == '[') {
			int j = i;
			while (s[j] && s[j] != ']') j++;
			if (!s[j]) break;
			i = j + 1;
			if (!s[i]) break;
		}
		i++; res += font_xsize_ceil;
	}
	return res;
}

bool Font::init(const char *image_file, float fx, int fy)
{
	if (!font.load_bmp(image_file)) return false;
	if (-1 == fx) {
		fx = font.get_x() / 95.0f;
		fy = font.get_y();
	}
	font_xsize_floor = (int) floor(fx);
	font_xsize_ceil = 1 + font_xsize_floor;
	font_xsize_float = fx;
	font_ysize = fy;
	return true;
}

float Font::w() const
{
	return font_xsize_floor;
}

int Font::w_int() const
{
	return font_xsize_ceil;
}

int Font::h() const
{
	return font_ysize;
}

#endif

/* calculates the needed number of "shell" 3D points needed for the pre-filler

	In fact, it generates points of the circle, which is the cross bween the given sphere and the plane A' (perpendicular to
	the ray CA.) It determines how many points to generate so that the resulting polygon would be smooth enough.

*/
int calculate_convex(Sphere *a, Vector pt[], Vector& c)
{double alpha, beta;
 double sina, sinb, cosa, cosb;
 int steps,i;
 double *f;
 steps = 4;
 while (steps < MAX_SPHERE_SIDES &&
 	a->d*sqrt(2.0f-2.0f*cos(2.0f*M_PI/steps)) / a->dist * Xres > M_SIDE_CONSTANT)
		steps+=4;
 if (steps == MAX_SPHERE_SIDES) steps-=4;
 if (steps>=MAX_SPHERE_SIDES) steps = MAX_SPHERE_SIDES-1;
 if (steps<MIN_SPHERE_SIDES) steps = MIN_SPHERE_SIDES;

 f = &PreSC[steps][0][0];
 alpha = atan2(a->pos[0]-c[0], a->pos[2]-c[2])+M_PI/2;
 beta = atan2(a->pos[1] - c[1], sqrt(sqr(a->pos[2]-c[2]) + sqr(a->pos[0]-c[0])));
 sina = sin(alpha);
 sinb = sin(beta);
 cosa = cos(alpha);
 cosb = cos(beta);

 double D = c.distto(a->pos);
 double K = sqrt(D*D-a->d*a->d);
 double K_plus_delta_K = D*D/K;
 double delta_K = K_plus_delta_K - K;
 double delta_D = D*delta_K/K_plus_delta_K;
 Vector newCenter;
 newCenter.make_vector(c, a->pos);

 newCenter.make_length(delta_D);
 newCenter += a->pos;
 double newRadius = sqrt(a->d*a->d - delta_D*delta_D);

 for (i=0;i<=steps;i++) {
 	pt[(i+steps/4*3)%steps] = newCenter + Vector(
		newRadius * (sina * f[1] + cosa * f[0] * sinb),
		newRadius *  f[0] * cosb,
		newRadius * (cosa * f[1] - sina * f[0] * sinb));
 	f+=2;
 }
 pt[steps] = pt[0];
 return steps;
}

int fun_min(int a, int b) { return a < b ? a: b;}
int fun_max(int a, int b) { return a > b ? a: b;}
bool fun_less(int a, int b) { return a < b; }
bool fun_more(int a, int b) { return a > b; }
int truncate(float x) { return (int) x; }
int fround(float x) { return (int) (x + 0.5); }

void project_hull_part(int *hull, Vector pt[], int start, int incr, int count, int sides, int color, const Vector& c, Vector w[3], int (*fun) (int,int), int (*rounding_fn) (float), int yres)
{
	int x, yy, last_y=0, wy=-1;
	if (yres == -1) yres = Yres2;
	float f_x=0, f_e, f_xi;
	for (int i = 0; i <= count; i++,start+=incr) {
		project_point(&x, &yy, pt[(start+sides) % sides], c, w, Xres2, Yres2);
		f_e = x;
		if (i) {
			int steps = imax(1, yy - last_y);
			int y, yi = (yy - last_y) / steps;
			y = last_y;
			f_xi = (f_e-f_x)/steps;
			for (int j = 0; j <= steps; j++, f_x += f_xi, y += yi) {
				if (y < 0 || y >= yres) continue;
				if (y > wy) {
					wy = y;
					hull[y] = rounding_fn( f_x );
				} else hull[y] = fun(hull[y], rounding_fn( f_x ));
			}
		}
		last_y = yy;
		f_x = f_e;
	}
}

void map_hull(Uint32 *fb, int *left, int *right, int ys, int ye, int color, int bias, int xres, int yres)
{
	if (xres == -1) xres = Xres2;
	if (yres == -1) yres = Yres2;
	if ( ys >= yres || ye < 0) return;
	if (ys < 0) ys = 0;
	if (ye >= yres ) ye = yres - 1;
	for (int y = ys; y <= ye; y++)
	{
		int xs = left[y]-bias;
		int xe = right[y]+bias;
		if (xs >= xres) continue;
		if (xe < 0) continue;
		if (xs < 0) xs = 0;
		if (xe >= xres) xe = xres - 1;
		if (xs < RowMin[y])
			RowMin[y] = xs;
		if (xe > RowMax[y])
			RowMax[y] = xe;

		Uint32 * rowpointer = fb + xres * y;
		for (int x = xs; x <= xe; x++) {
			if ((rowpointer[x] & 0xffff) != (unsigned) color) {
				rowpointer[x] = rowpointer[x] << 16 | color;
			}
		}
	}
}

void map_hull_16(Uint16 *fb, int *left, int *right, int ys, int ye, Uint16 color, int bias, int xres, int yres)
{
	if (xres == -1) xres = Xres2;
	if (yres == -1) yres = Yres2;
	if ( ys >= yres || ye < 0) return;
	if (ys < 0) ys = 0;
	if (ye >= yres ) ye = yres - 1;
	for (int y = ys; y <= ye; y++)
	{
		int xs = left[y]-bias;
		int xe = right[y]+bias;
		if (xs >= xres) continue;
		if (xe < 0) continue;
		if (xs < 0) xs = 0;
		if (xe >= xres) xe = xres - 1;

		Uint16 * rowpointer = fb + xres * y;
		for (int x = xs; x <= xe; x++) {
			rowpointer[x] |= color;
		}
	}
}


int accumulate(Vector pt[], int sides, const Vector& c, Vector w[3], bool (*fun) (int, int), int start_val, int & bi)
{
	int opt = start_val;
	for (int i = 0; i < sides; i++) {
		int x, y;
		project_point(&x, &y, pt[i], c, w, Xres2, Yres2);
		if (fun(y, opt)) {
			opt = y;
			bi = i;
		}
	}
	return opt;
}

/*
	Maps a sphere on the framebuffer. Pixels, coloured with its number are "expected" to be rendered there
	The Mapper's error is within 0.4% in most cases.
*/
void mapsphere(Uint32 *fb, int Ox, int Oy, int color, int sides, Vector pt[], const Vector& c, Vector w[3], sphere *A)
{
	int L[RES_MAXY], R[RES_MAXY];
	int ys, ye, bs, be;
	ys = accumulate(pt, sides, c, w, fun_less, 999666111, bs);
	ye = accumulate(pt, sides, c, w, fun_more,-999666111, be);
	int size = (be + sides - bs) % sides;
	project_hull_part(L, pt, bs, +1, size, sides, color, c, w, fun_min);
	project_hull_part(R, pt, bs, -1, sides - size, sides, color, c, w, fun_max);
	map_hull(fb, L, R, ys, ye, color);

}

// the pre-filler part: this fills the circular area of the screen where the selected sphere will map with the given color
// returns 0 if the sphere does not visualise at all
int project_it(Object *a, int *ns, Uint32 *fb, Vector& cur, Vector w[3], int xres, int yres, int color, int & min_y, int & max_y)
{
	Vector pt[MAX_SPHERE_SIDES];
	if (!a->is_visible(cur, w)) {
		min_y = -20;
		max_y = -10;
		return 0;
	}
	*ns = a->calculate_convex(pt, cur);
	a->map2screen(fb, color, *ns, pt, cur, w, min_y, max_y);
	return 1;
}

void map_to_screen(
		Uint16 *sbuffer, 
		Vector pt[], 
		int sides, 
		Uint16 color, 
		Vector& cur, 
		Vector w[3], 
		int xres, int yres)
{
	int L[RES_MAXY], R[RES_MAXY];
	int ys, ye, bs, be;
	ys = accumulate(pt, sides, cur, w, fun_less, 999666111, bs);
	ye = accumulate(pt, sides, cur, w, fun_more,-999666111, be);
	int size = (be + sides - bs) % sides;
	project_hull_part(L, pt, bs, +1, size, sides, color, cur, w, fun_min, truncate,  yres);
	project_hull_part(R, pt, bs, -1, sides - size, sides, color, cur, w, fun_max, truncate, yres);
	map_hull_16(sbuffer, L, R, ys, ye, color, 0, xres, yres);
}

void outline_to_screen(Uint16 *sbuffer, Vector pt[], int sides, Uint16 color, Vector& cur, Vector w[3], int xres, int yres)
{
	for (int i = 0; i < sides; i++) {
		float a[2], b[2];
		int res = project_point(a, a+1, pt[i], cur, w, xres, yres);
		res |= project_point(b, b+1, pt[(i + 1) % sides], cur, w, xres, yres);
		if (res < 8) {
			int steps = (int) ceil(fabs(a[0]- b[0]));
			int t = (int) ceil(fabs(a[1] - b[1]));
			if (t > steps) steps = t;
			for (int j = 0; j <= steps; j++) {
				int x = (int) round(a[0] + (b[0]-a[0])/steps * j);
				int y = (int) round(a[1] + (b[1]-a[1])/steps * j);
				if (x >= 0 && x < xres && y >= 0 && y < yres) {
					sbuffer[y * xres + x] = color;
				}
			}
		}
	}
}

void gfx_init(void)
{
 int i,j;

 for (j=1;j<=MAX_SPHERE_SIDES;j++)
 	for (i=0;i<=j;i++) {
		PreSC[j][i][0] = sin(((double)i/(double)j)*2*M_PI);
		PreSC[j][i][1] = cos(((double)i/(double)j)*2*M_PI);
		}
 gfx_update_2nds();
 if (cpu.sse)
 	blend = blend_sse;
}

void gfx_close(void)
{
}
