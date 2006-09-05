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

#include "bitmap.h"

class Konsole {
	int lines, rows;
	int xr, yr;
	char *data;
	int cur_x, cur_y;
	bool is_on;
	RawImg *font;
public:
	Konsole();
	~Konsole();
	
	void init(int xres, int yres, RawImg *font);
	void toggle();
	void show(bool reallyshow = true);
	void render(Uint32* framebuff);
};


#endif // __KONSOLE_H__
