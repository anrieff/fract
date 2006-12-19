/***************************************************************************
 *   Copyright (C) 2006 by Veselin Georgiev                                *
 *   anrieff@mgail.com (convert to gmail)                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
***************************************************************************/
#ifndef __DISPLAY_H__
#define __DISPLAY_H__

#include "pluginmanager.h"

typedef unsigned Rgb;

#define RGB(R,G,B) ((R << 16)|(G<<8)|B)

struct FrameBuffer {
	int x, y;
	Rgb *data;

	FrameBuffer();
	FrameBuffer(int x, int y, Rgb* init_data = NULL);
	void init(int x, int y, Rgb* init_data = NULL);
	~FrameBuffer();
	void copy(const FrameBuffer &other);
};

class GUI {
	int xres, yres;
	bool she;
public:
	GUI();
	bool init(int xr, int yr);
	void display(const FrameBuffer &fb);
	void update_view(View &v);
	bool should_exit() const;
};

extern GUI gui;

#endif //__DISPLAY_H__
