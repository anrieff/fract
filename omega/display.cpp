/****************************************************************************
 *   Copyright (C) 2006 by Veselin Georgiev                                *
 *   anrieff@mgail.com (convert to gmail)                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL/SDL.h>
#include <SDL/SDL_video.h>
#include <SDL/SDL_keyboard.h>
#include <vector>
#include <string>
#include <algorithm>
#include "display.h"

using namespace std;

static SDL_Surface *surface = NULL;

int viewmode = 1;
bool viewmode_changed = false;
bool zoom_updated = false;
bool newplug = true;

/**
 * @class FrameBuffer
*/ 
FrameBuffer::FrameBuffer() 
{
	x = y = 0;
	data = NULL;
}
FrameBuffer::FrameBuffer(int x, int y, Rgb *d)
{
	data = NULL;
	init(x, y, d);
}

void FrameBuffer::init(int x, int y, Rgb *d)
{
	this->x = x;
	this->y = y;
	
	if (x * y == 0) {
		data = NULL;
		return;
	}
	
	data = new Rgb[x*y];
	if (d) {
		memcpy(data, d, sizeof(Rgb) * x * y);
	} else {
		memset(data, 0, sizeof(Rgb) * x * y);
	}
}

FrameBuffer::~FrameBuffer()
{
	if (data) delete [] data;
	data = NULL;
	x = y= 0;
}

void FrameBuffer::copy(const FrameBuffer &rhs)
{
	if (rhs.x != x || rhs.y != y || !rhs.data) return;
	memcpy(data, rhs.data, x*y*sizeof(Rgb));
}

void FrameBuffer::zero(void)
{
	if (x && y && data)
		memset(data, 0, sizeof(Rgb) * x * y);
}

void FrameBuffer::inc_lum(int x, int y, int amount)
{
	if (x < 0 || y < 0 || x >= this->x || y >= this->y) return;
	Rgb &pix = data[x + y * this->x];
	int l = pix & 0xff;
	l += amount;
	if (l > 255) l = 255;
	pix = RGB(l,l,l);
}

/**
 * @class GUI
*/

GUI gui;

GUI::GUI()
{
	she = false;
}

bool GUI::should_exit() const { return she; }


bool GUI::init(int x, int y)
{
	xres = x;
	yres = y;
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0)
		return false;
	atexit(SDL_Quit);
	surface = SDL_SetVideoMode(x, y, 32, 0);
	if (!surface)
		return false;
	ticks = SDL_GetTicks();
	fm = new FontMan(NULL, x, y);
	return true;
}

void GUI::display(const FrameBuffer& fb)
{
	if (fb.x != xres || fb.y != yres || !fb.data) return;
	memcpy(surface->pixels, fb.data, sizeof(Rgb) * xres * yres);
	SDL_Flip(surface);
}

void GUI::display_fractal_selection_menu(void)
{
	char choices[1024];
	choices[0] = 0;
	for (int i = 0; i < get_plugin_count(); i++) {
		PluginInfo info = get_plugin(i)->get_info();
		strcat(choices, info.name);
		strcat(choices, " - ");
		strcat(choices, info.description);
		if (i < get_plugin_count() - 1) 
			strcat(choices, "|");
	}
	int res = menu("Choose fractal", choices);
	if (res != -1) {
		set_plugin(res);
		newplug = true;
	}
}

void GUI::update_view(View &v)
{
	SDL_Event e;
	zoom_updated = false;
	while (SDL_PollEvent(&e)) {
		if (e.type == SDL_QUIT) she = true;
		if (e.type == SDL_KEYDOWN) {
			int s = e.key.keysym.sym;
			switch (s) {
				case SDLK_ESCAPE: she = true; break;
				case SDLK_F1:
				case SDLK_F2:
				{
					int newviewmode = s == SDLK_F1 ? 1 : 2;
					viewmode_changed = newviewmode != viewmode;
					viewmode = newviewmode;
					break;
				}
				case SDLK_RETURN:
				{
					display_fractal_selection_menu();
					break;
				}
			}
		}
		if (e.type == SDL_MOUSEBUTTONUP) {
			if (e.button.button != SDL_BUTTON_LEFT && 
			    e.button.button != SDL_BUTTON_RIGHT) {
				// convenience, and it seems weird to use the
				// middle button for zooming so assign it here.
				display_fractal_selection_menu();
				continue;
			}
			double scaler = e.button.button == SDL_BUTTON_LEFT ? 0.75 : 1.3333333;
			int ix = e.button.x;
			int iy = e.button.y;
			double rx = (ix - xres/2) / (xres/2.0) * v.size;
			double ry = (iy - yres/2) / (xres/2.0) * v.size;
			if (scaler < 1) {
				v.x += rx * (1-scaler);
				v.y += ry * (1-scaler);
			} else {
				v.x -= rx * (scaler-1);
				v.y -= ry * (scaler-1);
			}
			v.size *= scaler;
			zoom_updated = true;
		}
	}
}

double GUI::time() const
{
	return (SDL_GetTicks() - ticks)*0.001;
}

int GUI::menu(const char *prompt, const char * choices)
{
	vector<string> a;
	string s = choices;
	while (s.find('|') != string::npos) {
		a.push_back(s.substr(0, s.find('|')));
		s = s.substr(s.find('|') + 1);
	}
	if (s != "")
		a.push_back(s);
	//
	FrameBuffer fbx(xres, yres, (unsigned*)surface->pixels);
	//
	unsigned mlen = strlen(prompt);
	for (unsigned i = 0; i < a.size(); i++) {
		mlen = max(mlen, a[i].length());
	}
	FractFont *ff = fm->get_current_font();
	int fx = ff->charwidth, fy = ff->y;
	int mx = 2 + 2 + 5 + 5 + mlen * fx;
	int my = 2 + 3 + fy + 3 + 1 + 2 + (int) a.size() * (fy+2) + 2;
	int sx = 15, sy = 15;
	//
	int cursel = 0, n = (int) a.size();
	while (1) {
		// display:
		fm->set_fb((unsigned*)surface->pixels);
		unsigned *p = (unsigned*) surface->pixels;
		//
		memcpy(p, fbx.data, xres*yres * sizeof(unsigned));
		for (int i = 0; i < my; i++)
			for (int j = 0; j < mx; j++) {
				unsigned & u = p[(sy+i)*xres + sx + j];
				if (i <= 1 || 8 + fy == i || i >= my - 2 || j <= 1 || j >= mx - 2) {
					u = 0xdddddd;
				} else {
					u = (u >> 2) & 0x3f3f3f;
					int ty = i - fy - 10;
					if (ty < 0) continue;
					ty /= (fy + 2);
					if (ty >= n) continue;
					if (ty == cursel)
						u += 0x102060;
				}
			}
		fm->set_color(0xd0f0f0);
		fm->printxy(sx + 7, sy + 5, "%s", prompt);
		fm->set_color(0xd0d0d0);
		for (int i = 0; i < n; i++)
			fm->printxy(sx + 7, sy + fy + 10 + i * (fy+2), "%s", a[i].c_str());
		SDL_Flip(surface);
		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) return -1;
			if (e.type == SDL_KEYDOWN) {
				int s = e.key.keysym.sym;
				switch (s) {
					case SDLK_DOWN:
						if (cursel < n - 1) cursel++;
						break;
					case SDLK_UP:
						if (cursel > 0) cursel--;
						break;
					case SDLK_RETURN:
						return cursel;
					case SDLK_ESCAPE:
						return -1;
					default:
						break;
				}
			}
			if (e.type == SDL_MOUSEMOTION) {
				int tx = e.motion.x - sx;
				int ty = e.motion.y - sy;
				if (tx < 0 || tx >= mx || ty < 0 || ty >= my)  continue;
				ty -= fy + 10;
				if (ty < 0) continue;
				ty /= (fy + 2);
				if (ty >= n) continue;
				cursel = ty;
			}
			if (e.type == SDL_MOUSEBUTTONUP) {
				if (e.button.button != 1) continue;
				int tx = e.button.x - sx;
				int ty = e.button.y - sy;
				if (tx < 0 || tx >= mx || ty < 0 || ty >= my)  continue;
				ty -= fy + 10;
				if (ty < 0) continue;
				ty /= (fy + 2);
				if (ty >= n) continue;
				cursel = ty;
				return cursel;
			}
		}
	}
}
