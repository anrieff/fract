/***************************************************************************
 *   Copyright (C) 2004 by Veselin Georgiev                                *
 *   vesko@ViruS                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "MyGlobal.h"
#include "MyTypes.h"

Uint32 get_ticks(void);
double bTime(void);
int lform(int x, int y, int lx, int ly, int ysq);
#ifdef ACTUALLYDISPLAY
SDL_Surface *get_screen(void);
#endif
void kbd_do(int *ShouldExit);
void kbd_tiny_do(int *ShouldExit);
void init_fract_array(void);


extern double daFloor, daCeiling;
extern bool WantToQuit;
extern int defaultconfig;
extern bool show_aa;
