/***************************************************************************
 *   Copyright (C) 2005 by Veselin Georgiev                                *
 *   vesko@ChaosGroup.com                                                  *
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
 ***************************************************************************/
 
#ifdef DEVBUILD


#include <SDL/SDL.h>
#include <SDL/SDL_video.h>
#include <iostream>
#include <string>
#include <math.h>
using namespace std;

struct CompareInfo {
	string name;
	float fps;
	int mhz;
};

const unsigned COLOR_GRID = 0xababab;
const unsigned COLOR_GRID_LIGHT = 0xefefef;
const int PER_ENTRY = 60;


SDL_Surface *screen;
int xres = 600, yres = 600;

static bool should_exit(void)
{
	SDL_Event e;
	while (SDL_PollEvent(&e)) {
		if (e.type == SDL_QUIT) return true;
		if (e.type == SDL_KEYDOWN) {
			if (e.key.keysym.sym == SDLK_ESCAPE) return true;
		}
	}
	return false;
}

static float fun_fps(CompareInfo & x) { return x.fps; }

unsigned *drawbuff, drawcol;

static inline void decomp(unsigned x, float r[3])
{
	r[2] = x & 0xff; x >>= 8;
	r[1] = x & 0xff; x >>= 8;
	r[0] = x & 0xff; 
}

static inline unsigned recompose(float r[3])
{
	unsigned x = 0;
	for (int i = 0; i < 3; i++) {
		if (r[i] < 0) r[i] = 0;
		if (r[i] > 255.0f) r[i] = 255.0f;
		x <<= 8;
		x |= (unsigned) r[i];
	}
	return x;
}

static unsigned mulcol(unsigned x, float mul)
{
	float r[3];
	decomp(x, r);
	for (int i = 0; i < 3; i++)
		r[i] *= mul;
	return recompose(r);
}

static void draw(unsigned *fb, int xr, int x, int y, unsigned color, double alpha)
{
	unsigned &p = fb[y * xr + x];
	float src[3], dst[3];
	decomp(p, src); decomp(color, dst);
	for (int i = 0; i < 3; i++)
		src[i] = (1.0 - alpha) * src[i] + alpha * dst[i];
	p = recompose(src);
}

static void draw_line(int x1, int y1, int x2, int y2)
{
	int xsteps, ysteps, steps;
	unsigned color = drawcol, *fb = drawbuff;
	xsteps = x1 - x2; if (xsteps < 0) xsteps = -xsteps;
	ysteps = y1 - y2; if (ysteps < 0) ysteps = -ysteps;
	steps = max(max(xsteps, ysteps), 1);
	
	for (int i = 0; i <= steps; i++) {
		float xf = x1 + ((float)x2-x1)*i/steps;
		float yf = y1 + ((float)y2-y1)*i/steps;
		int x = (int) xf;
		int y = (int) yf;
		if (xsteps > ysteps) {
			draw(fb, xres, x, y, color, 1 - (yf - y));
			draw(fb, xres, x, y + 1, color, (yf - y));
		} else {
			draw(fb, xres, x, y, color, 1 - (xf - x));
			draw(fb, xres, x + 1, y, color, (xf - x));
		}
	}
}

float lift(float x) { return x*x*x*x*x*x*x*x*x*x*x*x*x*x*x*x*x*x*x*x*x*x*x*x*x*x*x*x*x*x*x*x*x*x*x*x*x*x*x*x; }

static void draw_chart(CompareInfo a[], int n, unsigned *p, float (*fun) (CompareInfo&), int sx, int sy, int sizex, int sizey)
{
	float minval, maxval;
	minval = fun(a[0]);
	maxval = minval;
	for (int i = 0; i < n; i++) {
		float val = fun(a[i]);
		if (val < minval) minval = val;
		if (val > maxval) maxval = val;
	}

	int mtick = (int) ceil(maxval/minval);
	
	drawbuff = p;
	drawcol = COLOR_GRID_LIGHT;
	if ((sizex - 20) / mtick > 20) {
		for (int i = 1; i < mtick * 10; i++) if (i % 10) {
			int xx = i * (sizex-20) / (10*mtick);
			draw_line(sx + xx, sy + sizey, sx + xx + 20, sy + sizey - 15);
			draw_line(sx + xx + 20, sy + sizey - 15, sx + xx + 20, sy );
		}
	}
	drawcol = COLOR_GRID;
	draw_line(sx, sy + 15, sx, sy + sizey);
	draw_line(sx, sy + sizey, sx + sizex - 20, sy + sizey);
	
	draw_line(sx + 20, sy, sx + 20, sy + sizey - 15);
	draw_line(sx + 20, sy + sizey - 15, sx + sizex, sy + sizey - 15);
	draw_line(sx + sizex, sy + sizey - 15, sx + sizex, sy);
	draw_line(sx + 20, sy, sx + sizex, sy);
	
	draw_line(sx, sy + 15, sx + 20, sy);
	draw_line(sx, sy + sizey, sx + 20, sy + sizey - 15);
	draw_line(sx + sizex - 20, sy + sizey, sx + sizex, sy + sizey - 15);
	
	for (int i = 1; i < n; i++) {
		int yy = i * (sizey-15) / n;
		draw_line(sx, sy + yy + 15, sx + 20, sy + yy);
		draw_line(sx + 20, sy + yy, sx + sizex, sy + yy);
	}
	
	for (int i = 1; i < mtick; i++) {
		int xx = i * (sizex-20) / mtick;
		draw_line(sx + xx, sy + sizey, sx + xx + 20, sy + sizey - 15);
		draw_line(sx + xx + 20, sy + sizey - 15, sx + xx + 20, sy );
	}
	
	int per_entry = (sizey-15)/n;
	
	for (int i = 0; i < n; i++) {
		float coeff = (fun(a[i]) - minval) / (maxval - minval);
		unsigned mycolor = (int) (0xff * (1-coeff)) | (((int) (0xff0000 * coeff))&0xff0000);
		/*int yy = i * (sizey-15) / n + 25;
		int xx = sx + (int) ((fun(a[i]) / minval / mtick) * (sizex-20))+7;
		for (int j = 0; j < per_entry; j++)
			draw_line(sx+5, yy+j, xx, yy+j);
			*/
		for (float angle = 0.0f; angle < 2 * M_PI; angle += 0.01) {
			int sgx = 10 + (int) ( 10.0f * sin(angle));
			int sgy = i * (sizey-15) / n + per_entry/2 + 5 - (int) ( 15.0f * cos(angle));
			float beta = fabs(angle - 5.12);
			if (beta > M_PI) beta = 2 * M_PI - beta;
			
			float intensity = cos(beta); if (intensity < 0) intensity = 0;
			intensity += 2.5f*lift(intensity);
			drawcol = mulcol(mycolor, intensity);
			draw_line(sx + sgx, sy + sgy, sx + sgx + (int) ((sizex-20)*fun(a[i])/minval/mtick), sy + sgy);
		}
	}
	
}

void dowork(CompareInfo a[], int n, unsigned *p)
{
	memset(p, 0xff, xres*yres*4);
	draw_chart(a, n, p, fun_fps, 5, 5, 550, n*PER_ENTRY);
}

int main(void)
{	
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		cout << "Cannot initialize SDL, beacuse " << SDL_GetError() << endl;
		return 0;
	}
	SDL_Surface *screen = SDL_SetVideoMode(xres, yres, 32, 0);
	atexit(SDL_Quit);
	
	if (!screen) return 0;
	
	CompareInfo a[4];
	a[0].name = "Opteron 165 (my PC, \nno O/C)";
	a[0].fps = 27.23;
	a[0].mhz = 1800;
	a[1].name = "Opteron 165 (my PC, \nwith O/C)";
	a[1].fps = 35.87;
	a[1].mhz = 2253;
	a[2].name = "Xeon Prestonia, 533,\n2 CPUs w/ HT";
	a[2].fps = 21.05;
	a[2].mhz = 2400;
	a[3].name = "Athlon XP, 1.4GHz";
	a[3].fps = 11.50;
	a[3].mhz = 1402;
	
	
	dowork(a, 4, (unsigned*) screen->pixels);
	
	SDL_Flip(screen);
	while (!should_exit()) SDL_Delay(100);

	return 0;
}


















#else

#include "comparedlg.h"


CompareDialog::CompareDialog(CompareInfo a[], int count) :
	wxDialog((wxDialog *)NULL, wxID_ANY, "Result comparison", 
	wxDefaultPosition, wxSize(600, 600))
{
	wxButton *closebtn = new wxButton(this, wxID_OK, "&Close", 
		wxPoint(250, 550), wxSize(100, 30));
	closebtn->Refresh();
}

#endif
