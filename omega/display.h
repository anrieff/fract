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
#include "fonts.h"

struct FrameBuffer {
	int x, y;
	Rgb *data;

	FrameBuffer();
	FrameBuffer(int x, int y, Rgb* init_data = NULL);
	void init(int x, int y, Rgb* init_data = NULL);
	void zero(void);
	~FrameBuffer();
	void copy(const FrameBuffer &other);
	void inc_lum(int x, int y, int amount);
};

class GUI {
	int xres, yres;
	bool she;
	unsigned ticks;
	FontMan *fm;
public:
	GUI();
	bool init(int xr, int yr);
	void display(const FrameBuffer &fb);
	void update_view(View &v);
	void display_fractal_selection_menu(void);
	/**
	 * Displays a multiple-choice menu, using the currently-painted picture
	 * on the surface as background
	 *
	 * @param prompt  - The prompt for the menu
	 *                  (e.g. "How much is 2+2*2")
	 * @param choices - Pipe ('|')-separated list of possible answers 
	 *                  (e.g. "2|4|6|8|who cares")
	 *
	 * @returns the index of the chosen answer (zero-based). If the user
	 *          did not answer (e.g. by pressing ESC), return value is -1.
	*/
	int menu(const char *prompt, const char * choices);
	bool should_exit() const;
	double time() const;
};

extern GUI gui;
extern int viewmode;
extern bool viewmode_changed;
extern bool zoom_updated;
extern bool newplug;

#endif //__DISPLAY_H__
