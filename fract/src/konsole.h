/***************************************************************************
 *   Copyright (C) 2006 by Veselin Georgiev                                *
 *   anrieff@mailg.com (convert to gmail)                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/
 
#ifndef __KONSOLE_H__
#define __KONSOLE_H__

#include "MyTypes.h"
#include "bitmap.h"
#include "gfx.h"

struct KonsoleChar {
	int color;
	char ch;
	
	void neutral(void)
	{
		color = 0;
		ch = 0;
	}
};

class Konsole {
	int lines, cols;
	int xr, yr;
	int current_color;
	KonsoleChar *data;
	char *buffer; int buffpos;
	Uint32 *konsole_background;
	int cur_x, cur_y;
	bool is_on;
	Font *font;
	
	void advance(void);
	void putchar(char c);
	void scroll(void);
	char try_char(int code, bool shift);
public:
	Konsole();
	~Konsole();
	
	void init(int xres, int yres, Font *font);
	void toggle();
	void show(bool reallyshow = true);
	bool visible() const;
	void render(void *screen, Uint32* framebuff);
	bool handle_keycode(int keycode, bool shift);
	void write(const char *format, ...);
	void puts(const char *s);
	void set_color(int new_color);
	void set_default_color(void);
	int execute(const char *cmd);
};

extern Konsole konsole;

#endif // __KONSOLE_H__
