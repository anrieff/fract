/***************************************************************************
 *   Copyright (C) 2006 by Veselin Georgiev                                *
 *   anrieff@mailg.com (convert to gmail)                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *-------------------------------------------------------------------------*
 * Shows the final "credits" screen                                        *
 ***************************************************************************/

#include "MyGlobal.h"
#ifdef ACTUALLYDISPLAY 

#include <string.h>
#include <stdlib.h>
#include "bitmap.h"
#include "credits.h"
#include "fract.h"
#include "scene.h"
#include "gfx.h"

const int el = 15;
const int bobnum = 2000;
const int bobspeed = 80;
const Uint32 normcol = 0xf0f0f0;
const Uint32 boldcol = 0xcdf9ff;
const Uint32 graycol = 0x9f9f9f;
const Uint32 orgcol = 0xf8c88f;
const double fontspeed = 0.15; // y screen size per second
extern Font font0;

const char * credits[] = {
	"[U]CREDITS",
	"","",
	
	"[C]Friends:",
	"",
	"barnie",
	"BeaZtiE",
	"DJ Bamby",
	"espr1t",
	"fellix",
	"nickysn",
	"vegasis",
	"Vesu",
	"vlado",
	"","",

	"[C]My sweetheart:",
	"",
	"Ivet - Love you very much!",
	"","",

	"[C]Dev tools/libraries:",
	"",
	"SDL",
	"wxWidgets",
	"",
	
	"[C]Dev People:",
	"",
	"Emil 'Humus' Persson [G]- object glow effect",
	"Martin Reddy         [G]- OBJ format        ",
	"Setfan Hetzl         [G]- BMP format        ",
	"Paulo Barreto        [G]- AES lib           ",
	"Rick Reed            [G]- Snell's law       ",
	"Paul Bourke          [G]- FFTs              ",
	"The fourcc page      [G]- RGB/YUV stuff     ",
	"Stefano Tommesani    [G]- MMX/SSE stuff     ",
	"Matthew Jones        [G]- Optimized x^(-1/4)",
	"John Amanatides      [G]- Grid traversal    ",
	"Nils Pipenbrick      [G]- 6DOF heightfield  ",
	"Tomas Akenine-Moller                     ",
	"Ulf Assarsson        [G]- Soft shadows      ",
	"","","",
	
	"[C]People I admire:",
	"",
	"[R]aardappel",
	"[R]humus",
	"","",
	
	"[C]These sites rock:",
	"",
	"ujas.org",
	"hardwarebg.com/forum",
	"www.thedailywtf.com",
	"sentinel"
};

static Uint8 doseffect[DOS_RES_X * DOS_RES_Y];
static Uint32 palette[256];

Uint32 *get_frame_buffer(void);

static void add_blob(void)
{
	int ballx = (rand()%(DOS_RES_X - 2*el - 1)) + el;
	int bally = (rand()%(DOS_RES_Y - 2*el - 1)) + el;
	//
	for (int i = -el; i <= el; i++)
		for (int j = -el; j <= el; j++) {
			int dist = i*i+j+j;
			if (dist < 100) dist = 100;
			int increaser = bobnum / dist;
			doseffect[(i+bally)*DOS_RES_X + (j+ballx)] += increaser;
		}
}

static void add_blobs(double time)
{
	static int blobs_done = 0;
	int blobs_todo = (int) (time*bobspeed);
	for (;blobs_done < blobs_todo; blobs_done++) {
		add_blob();
	}
}

static unsigned kol(int r, int g, int b)
{
	if (r > 255) r = 255; if (r < 0) r = 0;
	if (g > 255) g = 255; if (g < 0) g = 0;
	if (b > 255) b = 255; if (b < 0) b = 0;
	r/=2; g/=2; b/=2;
	return (r << 16) + (g << 8) + b;
}

static void init_palette(void)
{
	for (int i = 0; i < 256; i++) {
		int rg = 0, b = 0;
		if (i >=  64 && i < 128) rg = (i-64)*2;
		if (i >= 128 && i < 192) rg = 256 - (i-128)*4;
		if (i < 64) b = 4*i;
		if (i >= 64 && i < 192) b = 256 - (i-64)*2;
		palette[i] = kol(rg, rg, b);
	}
}

static void rescale(Uint8* dos, Uint32 *fb, int xr, int yr)
{
	memset(fb, 0, xr*yr*4);
	
	int lineoffset = (yr - xr * DOS_RES_Y / DOS_RES_X)/2;
	for (int y = lineoffset; y < yr - lineoffset; y++)
	{
		int oy = (y - lineoffset) * DOS_RES_Y / (yr - 2*lineoffset);
		if (oy >= DOS_RES_Y) oy = DOS_RES_Y;
		for (int x = 0; x < xr; x++) {
			int ox = x * DOS_RES_X / xr;
			if (ox >= DOS_RES_X) ox = DOS_RES_X;
			fb[y * xr + x] = palette[doseffect[oy * DOS_RES_X + ox]];
		}
	}
}

static void display_it(SDL_Surface *s, Uint32 *fb)
{
	surface_lock(s);
	surface_memcpy(s, fb);
	surface_unlock(s);
	SDL_Flip(s);
}

static void credits_callback(Uint32 & col, bool & underline, const char *format)
{
	for (unsigned i = 0; i < strlen(format); i++) {
		if (format[i] == 'C') col = boldcol;
		if (format[i] == 'G') col = graycol;
		if (format[i] == 'R') col = orgcol;
		if (format[i] == 'U') underline = true;
	}
}

static void add_text(double time, int *should_exit, int xr, int yr, Uint32 *fb)
{
	int i;
	font0.set_print_callback(credits_callback);
	for (i = 0; strcmp(credits[i], "sentinel"); i++) {
		int ly = yr + (2 + font0.h()) * i - (int) (time*fontspeed*yr);
		if (ly < yr && ly > -font0.h()) {
			int startx = (xr-font0.get_text_xlength(credits[i]))/2;
			font0.printxy(NULL, fb, startx, ly, normcol, 1.0, credits[i]);
		}
	}
	if (yr + (2 + font0.h()) * i - (int) (time*fontspeed*yr) < 0) *should_exit = 1;
}

void show_credits(void)
{
	memset(doseffect, 0, sizeof(doseffect));
	int xr = xres(); int yr = yres();
	SDL_Surface *screen = get_screen();
	Uint32 *p = get_frame_buffer();
	
	init_palette();
	int she = 0;
	double xt = bTime();
	while (!she ) {
		add_blobs(bTime() - xt);
		rescale(doseffect, p, xr, yr);
		add_text(bTime() - xt, &she, xr, yr, p);
		display_it(screen, p);
		kbd_tiny_do(&she);
	}
}


#else
void show_credits(void)
{
}
#endif
