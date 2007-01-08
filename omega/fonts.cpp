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

extern unsigned char luxi_data[];

#ifdef _MSC_VER
#define strcasecmp stricmp
#endif

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "fonts.h"

LuxiFont::LuxiFont()
{
	x = 760; y = 17; charwidth = 8; data = &luxi_data[0];
}

const char *LuxiFont::get_name() const
{
	return "luxi";
}

/**
 * @class FontMan
*/ 

FontMan::FontMan(unsigned *p, int resx, int resy)
{
	buff = p;
	xr = resx;
	yr = resy;
	color = 0;
	cx = cy = 0;
	contrast = false;
	//
	memset(fontlist, 0, sizeof(fontlist));
	fontcount = 1;
	curfont = 0;
	fontlist[0] = new LuxiFont;
	global_opacity = 1.0f;
}

FontMan::~FontMan()
{
	for (int i = 0; i < fontcount; i++) {
		delete fontlist[i];
		fontlist[i] = NULL;
	}
}

void FontMan::set_color(unsigned new_color) { color = new_color; }
void FontMan::set_contrast(bool on) { contrast = on; }
void FontMan::set_cursor(int x, int y) 
{
	if (x != -1) cx = x;
	if (y != -1) cy = y;
}

void FontMan::set_opacity(float new_opacity)
{
	global_opacity = new_opacity;
}

void FontMan::set_font(const char *name)
{
	for (int i = 0; i < fontcount; i++) {
		if (0 == strcasecmp(name, fontlist[i]->get_name())) {
			curfont = i; return;
		}
	}
	curfont = 0;
}

FractFont* FontMan::get_current_font() const
{
	return fontlist[curfont];
}

void FontMan::set_fb(unsigned *new_fb)
{
	buff = new_fb;
}

int FontMan::get_x() const { return cx; }
int FontMan::get_y() const { return cy; }

void FontMan::print(const char *format, ... )
{
	va_list arg;
	static char s[1024];
	va_start(arg, format);
	#ifdef _MSC_VER
	_vsnprintf(s, 1024, format, arg); // M$ sucks with those _names :)
	#else
	vsnprintf(s, 1024, format, arg);
	#endif
	va_end(arg);
	
	write(s);
}

void FontMan::printxy(int xx, int yy, const char *format, ...)
{
	cx = xx;
	cy = yy;
	va_list arg;
	static char s[1024];
	va_start(arg, format);
	#ifdef _MSC_VER
	_vsnprintf(s, 1024, format, arg); // M$ sucks with those _names :)
	#else
	vsnprintf(s, 1024, format, arg);
	#endif
	va_end(arg);
	
	write(s);
}

void FontMan::write(const char *s) 
{
	FractFont *ff = fontlist[curfont];
	for (unsigned k = 0; k < strlen(s); k++) {
		if (s[k] > 32 && s[k] < 127) {
			for (int j = 0; j < ff->y; j++) {
				for (int i = 0; i < ff->charwidth; i++) {
					draw(cx+i, cy+j, ff->data[i+j*ff->x+ff->charwidth*((int) (s[k]-33))]);
				}
			}
		}
		cx += ff->charwidth;
	}
}

static inline int iabs(int x) { return x < 0 ? -x:x; }

void FontMan::draw(int x, int y, unsigned char opacity)
{
	if (x < 0 || x >= xr || y < 0 || y >= yr) return;

	unsigned opc = (unsigned) (0.5f + opacity * global_opacity);
	
	unsigned t = buff[y*xr+x];
	unsigned col1[3] = { t&0xff, (t>>8)&0xff, (t>>16)&0xff};
	unsigned col2[3] = { color & 0xff, (color >> 8) & 0xff, (color >> 16) & 0xff };
	if (contrast) {
		int diff = 0;
		for (int i = 0; i < 3; i++)
			diff += iabs(col1[i]-col2[i]);
		if (diff < 64) {
			for (int i = 0; i < 3; i++) {
				col2[i] = 255-col2[i];
			}
		}
	}
	t = 0;
	for (int i = 2; i >= 0; i--) {
		t = (t << 8) | ((opc*col2[i]+(255-opc)*col1[i])>>8);
	}
	buff[y*xr+x] = t;
}
