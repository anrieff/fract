/***************************************************************************
 *   Copyright (C) 2006 by Veselin Georgiev                                *
 *   anrieff@mailg.com (convert to gmail)                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *-------------------------------------------------------------------------*
 * Displays and manages the fract console                                  *
 ***************************************************************************/
 
#include "MyGlobal.h"
#include "konsole.h"
#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#ifdef _WIN32
#include <malloc.h>
#endif

#ifdef ACTUALLYDISPLAY
#endif

Konsole::Konsole()
{
	data = NULL;
	konsole_background = NULL;
	is_on = false;
	font = NULL;
}

Konsole::~Konsole()
{
	is_on = false;
	if (data) delete [] data;
	if (konsole_background) delete [] konsole_background;
	data = NULL;
	konsole_background = NULL;
}

void Konsole::init(int xres, int yres, Font *font_in)
{
	current_color = 0xcccccc;
	font = font_in;
	xr = xres;
	yr = yres;
	int fsx = (int) ceil(font->w());
	int fsy = font->h();
	lines = yres / 2 / fsy;
	cols = xres / fsx;
	data = new KonsoleChar[cols * lines];
	for (int i = 0; i < cols * lines; i++)
		data[i].neutral();
	cur_x = cur_y = 0;
	
	write("Welcome to fract's console!\nType `help' if you feel lost here\n\n");
	
	// create the console background:
	konsole_background = new Uint32[xr * (yr/2)];
	// render background:
	for (int j = 0; j < yr/2; j++) {
		float y = (float) j / yr;
		for (int i = 0; i < xr; i++) {
			float x = (float) i / yr;
			float dist1 = sqrt(sqr(x - 0.66) + sqr(y - 0.25));
			float dist2 = sqrt(sqr(x - 0.1) + sqr(y - 0.66));
			konsole_background[j * xr + i] = 
					multiplycolorf(0x113355, (sin((dist1+dist2)*420.0f)+1.0f)*0.33f);
		}
	}

}

void Konsole::scroll(void)
{
	for (int i = 0; i < lines-1; i++)
		for (int j = 0; j < cols; j++)
			data[i * cols + j] = data[(i+1) * cols + j];
	for (int j = 0; j < cols; j++)
		data[(lines-1)*cols + j].neutral();
}

void Konsole::puts(const char *s)
{
	for (int i = 0; s[i]; i++) putchar(s[i]);
}

void Konsole::putchar(char c)
{
	if (c >= 32) {
		int idx = cur_x + cur_y * cols;
		data[idx].color = current_color;
		data[idx].ch = c;
		advance();
	} else {
		switch (c) {
			case '\n': 
			{
				cur_x = 0;
				cur_y++;
				if (cur_y >= lines) {
					--cur_y;
					scroll();
				}
				break;
			}
			default:
			{
				putchar('?');
				break;
			}
		}
	}
}

void Konsole::advance(void)
{
	cur_x ++;
	if (cur_x >= cols) {
		cur_x = 0;
		cur_y++;
		if (cur_y >= lines) {
			--cur_y;
			scroll();
		}
	}
}

void Konsole::write(const char *buf, ...)
{
	va_list arg;
	static char s[1024];
	va_start(arg, buf);
#ifdef _MSC_VER
	_vsnprintf(s, 1024, buf, arg); // M$ sucks with those _names :)
#else
	vsnprintf(s, 1024, buf, arg);
#endif
	va_end(arg);

	puts(s);
}

void Konsole::toggle()
{
	is_on = !is_on;
}

void Konsole::show(bool reallyshow)
{
	is_on = reallyshow;
}

bool Konsole::visible() const
{
	return is_on;
}

void Konsole::render(void* screen, Uint32 *fb)
{
	if (!is_on) return;
#ifdef ACTUALLYDISPLAY
	char *temp = (char *) alloca(cols+2);
	SDL_Surface *p = (SDL_Surface*) screen;
	
	int xp = 0;
	for (int j = 0; j < yr/2; j++)
		for (int i = 0; i < xr; i++, xp++)
			fb[xp] = konsole_background[xp];
	
	//push the text:
	for (int j = 0; j < lines; j++) if (data[j*cols].ch) {
		int i=0, k=0, cc=data[j*cols].color, start = 0;
		while (1) {
			while (i < cols && data[j * cols + i].color == cc && data[j * cols + i].ch) {
				temp[k++] = data[j * cols + i].ch;
				i++;
			}
			temp[k] = 0;
			font->printxy(p, fb, start * (font->w_int()), j * font->h(), cc, 0.8f, temp);
			start += k;
			if (i >= cols || data[j * cols + i].ch == 0) break;
			cc = data[j*cols + i].color;
			k = 0;
		}	
	}
	
	// place the cursor:
	font->printxy(p, fb, font->w_int() * cur_x, font->h() * cur_y, 0x00ff00, 0.8f, "_");
#endif
}

bool Konsole::handle_keycode( int code, bool shift )
{
#ifndef ACTUALLYDISPLAY
	return false;
#else
	char c = try_char(code, shift);
	if (c != 0) {
		putchar(c);
		return true;
	}
	if (code == SDLK_RETURN || code == SDLK_KP_ENTER) {
		putchar('\n');
		//execute the command
		return true;
	}
	return false;
#endif
}

char Konsole::try_char(int code, bool shift)
{
#ifdef ACTUALLYDISPLAY
	if (code >= SDLK_a && code <= SDLK_z) {
		char base = shift ? 'A' : 'a';
		return base + (code - SDLK_a);
	}
	if (code >= SDLK_SPACE && code <= SDLK_BACKQUOTE)
		return (char) code;
	
#endif
	return 0;
}



//////// data...

Konsole konsole;
