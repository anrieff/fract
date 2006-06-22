/***************************************************************************
 *   Copyright (C) 2004 by Veselin Georgiev                                *
 *   vesko@ViruS                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef __BLUR_H__
#define __BLUR_H__
#include "MyGlobal.h"
#include "MyTypes.h"

#ifdef BLUR



#define BLUR_DISCRETE	0
#define BLUR_CONTINUOUS 1
#define BLUR_COUNT	2

#define BLUR_ID_NORMAL     0x00000004
#define BLUR_ID_CONTINUOUS 0x00000008

void blur_do(Uint32 *src, Uint32 *display_buff, int count, int frame_num);
void cycle_blur_mode(void);
void set_blur_method(int newmethod);
void blur_reinit(void);

void blur_init(void);
void blur_close(void);

extern int apply_blur;
#endif // BLUR


#endif // __BLUR_H__
