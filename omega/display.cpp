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
#include "display.h"

static SDL_Surface *surface = NULL;

int viewmode = 1;
bool viewmode_changed = false;

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
	return true;
}

void GUI::display(const FrameBuffer& fb)
{
	if (fb.x != xres || fb.y != yres || !fb.data) return;
	memcpy(surface->pixels, fb.data, sizeof(Rgb) * xres * yres);
	SDL_Flip(surface);
}

void GUI::update_view(View &v)
{
	SDL_Event e;
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
			}
		}
		if (e.type == SDL_MOUSEBUTTONUP) {
			double scaler = e.button.button == 1 ? 0.75 : 1.3333333;
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
		}
	}
}

double GUI::time() const
{
	return (SDL_GetTicks() - ticks)*0.001;
}
