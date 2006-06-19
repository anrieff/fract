/***************************************************************************
 *   Copyright (C) 2004 by Veselin Georgiev                                *
 *   vesko@ViruS                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *-------------------------------------------------------------------------*
 *                                                                         *
 * Handles all Blurring and Accumulation routines                          *
 *                                                                         *
 ***************************************************************************/

#include <string.h>

#include "MyGlobal.h"
#include "MyTypes.h"
#include "blur.h"
#include "cpu.h"
#include "gfx.h"

int apply_blur = 0, blur_method = BLUR_DISCRETE;

#ifdef BLUR
// *************** External variables *****************************
extern Uint32 framebuffer[RES_MAXX*RES_MAXY];

// **************** GLOBAL VARIABLES ******************************
SSE_ALIGN(Uint32 blurbuffers[8][RES_MAXX*RES_MAXY]);
SSE_ALIGN(Uint32 accumulation_buffer[RES_MAXX*RES_MAXY]);
Uint32 tempstor[RES_MAXX*RES_MAXY];
int blur_current;


// ---------- include the assembly routines -----------------------
#define blur_asm
#include "x86_asm.h"
#undef blur_asm


//extern Uint32 blurbuffers[8][RES_MAXX*RES_MAXY], accumulation_buffer[RES_MAXX*RES_MAXY];
//extern int blur_current;

/*
 *	These functions cope with the blur effect (if it is enabled in MyGlobal.h) - the idea used here is
 *	to generate 8 consecutive frames and merge them in a single frame which is copied to the real framebuffer and
 *	displayed as usual. We are using 8 temporary (hidden) framebuffers, one accumulation buffer and one temporary buffer.
 *	.
 *	There are two methods of blurring, selectable by `blur_method'
 *
 *		1) if it is BLUR_CONTINUOUS then the last eight frames are merged into one and displayed after each frame
 *		2) if it is BLUR_DISCRETE then each eight frames the merged result is shown, then new accumulation begins and so on.
 *
 *	All data in the `blur' framebuffers is stored the following way:
 *
 *	32 bit Unsigned int:
 *
 *	   10-bit red                11-bit green      11-bit blue
 *	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *	#R.R.R.R.R.R.R.r|r.r.G.G.G.G.G.G|G.G.g.g.g.B.B.B|B.B.B.B.B.b.b.b|
 *	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *	 31                                                            0  bits
 *
 *	MMX versions of these routines exist in x86_asm.h
 */


/* Blur Forward - convert an ordinary RGB framebuffer `src' into the format, suitable for blur accumulation */
void blur_forward(Uint32 *dest, Uint32 *src, int count)
{
	int i;
	if (cpu.mmx) {
		//printf("Blur_Forward: not implemented\n");
		blur_forward_mmx(dest, src, count);
	} else {
		for (i=0; i < count; i++)
			dest[i]  = (src[i] & 0xff)  | (( src[i] & 0xff00) << 3) | (( src[i] &0xf80000) << 5);
	}
}

/* Blur Backward - decomposes an accumulated framebuffer `src' into an ordinary RGB framebuffer */
void blur_backward(Uint32 *dest, Uint32 *src, int count)
{
	int i;
	if (cpu.mmx) {
		blur_backward_mmx(dest, src, count);
	} else {
		for (i=0;i<count; i++)
			dest[i] =	((src[i] & 0x000007f8) >> 3) |
					((src[i] & 0x003fc000) >> 6) |
					((src[i] & 0xff000000) >> 8);
	}
}

/* Substracts all `src' points from `dest' (warp-around arithmetic) */
void buffer_minus(Uint32 *dest, Uint32 *src, int count)
{
	int i;
	if (cpu.mmx) {
		buffer_minus_mmx(dest, src, count);
	} else {
		for (i=0;i<count;i++) dest[i] -= src[i];
	}
}

/* Adds all `src' points to `dest'. Accumulates a blur-friendly-format framebuffer into a blur-friendly-format accumulation buffer */
void buffer_plus(Uint32 *dest, Uint32 *src, int count)
{
	int i;
	if (cpu.mmx) {
		buffer_plus_mmx(dest, src, count);
	} else {
		for (i=0;i<count;i++) dest[i] += src[i];
	}
}

/*
	Does the actual blur procedure
*/
void blur_do(Uint32 *src, Uint32 *display_buff, int count, int frame_num)
{
	Uint32 *cbuff = blurbuffers[blur_current];
	buffer_minus(accumulation_buffer, cbuff, count);
	blur_forward(cbuff, src, count);
	buffer_plus(accumulation_buffer, cbuff, count);
	if (blur_method == BLUR_DISCRETE) {
		if (frame_num % 8 == 0)
			blur_backward(tempstor, accumulation_buffer, count);
		memcpy(display_buff, tempstor, count*sizeof(Uint32));
	} else
		blur_backward(display_buff, accumulation_buffer, count);

	blur_current = (blur_current + 1) % 8;
}

/*
	Triggered when blur is being toggled on. Makes an illusion of the blur effect slowly appearing.
*/
void blur_reinit(void)
{
	int i, count = xres()*yres();
	blur_current = 0;
	for (i=0;i<8;i++)
		blur_forward(blurbuffers[i], framebuffer, count);
	blur_forward(accumulation_buffer, framebuffer, count);

	for (i=0; i < count; i++)
		accumulation_buffer[i] <<= 3; // make as if 8 frames were layered in the accumulation buffer
	memcpy(tempstor, framebuffer, count*sizeof(Uint32));
}


void cycle_blur_mode(void)
{
	blur_method = (blur_method + 1) % BLUR_COUNT;
}

void set_blur_method(int newmethod)
{
	blur_method = newmethod % BLUR_COUNT;
}

#endif // BLUR
