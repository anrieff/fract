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
#include "shaders.h"
#include "threads.h"

int apply_blur = 0, blur_method = BLUR_DISCRETE;

#ifdef BLUR
// *************** External variables *****************************
extern SSE_ALIGN(Uint32 framebuffer[RES_MAXX*RES_MAXY]);

// **************** GLOBAL VARIABLES ******************************
Uint32 *blurbuffers[8];
Uint32 *accumulation_buffer;
Uint32 *tempstor;
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
void blur_forward(Uint32 *dest, Uint32 *src, int xr, int yr, int ti, int tc)
{
	int i;
	if (cpu.mmx) {
		//printf("Blur_Forward: not implemented\n");
		blur_forward_mmx(dest, src, xr, yr, ti, tc);
	} else {
		for (i=ti; i < yr; i+= tc)
			for (int j = 0; j < xr; j++) {
				int p = i * xr + j;
				dest[p]  = (src[p] & 0xff)  | (( src[p] & 0xff00) << 3) | (( src[p] &0xf80000) << 5);
			}
	}
}

/* Blur Backward - decomposes an accumulated framebuffer `src' into an ordinary RGB framebuffer */
void blur_backward(Uint32 *dest, Uint32 *src, int xr, int yr, int ti, int tc)
{
	int i;
	if (cpu.mmx) {
		blur_backward_mmx(dest, src, xr, yr, ti, tc);
	} else {
		for (i = ti; i < yr; i += tc)
			for (int j = 0; j < xr; j++) {
				int p = i * xr + j;
				dest[p] =	((src[p] & 0x000007f8) >> 3) |
						((src[p] & 0x003fc000) >> 6) |
						((src[p] & 0xff000000) >> 8);
			}
	}
}

/* Substracts all `src' points from `dest' (warp-around arithmetic) */
void buffer_minus(Uint32 *dest, Uint32 *src, int xr, int yr, int ti, int tc)
{
	int i;
	if (cpu.mmx) {
		buffer_minus_mmx(dest, src, xr, yr, ti, tc);
	} else {
		for (i = ti; i < yr; i += tc) 
			for (int j = 0; j < xr; j++)
				dest[i*xr + j] -= src[i*xr + j];
	}
}

/* Adds all `src' points to `dest'. Accumulates a blur-friendly-format framebuffer into a blur-friendly-format accumulation buffer */
void buffer_plus(Uint32 *dest, Uint32 *src, int xr, int yr, int ti, int tc)
{
	int i;
	if (cpu.mmx) {
		buffer_plus_mmx(dest, src, xr, yr, ti, tc);
	} else {
		for (i = ti; i < yr; i += tc)
			for (int j = 0; j < xr; j++)
				dest[i*xr + j] += src[i*xr + j];
	}
}

/*
	Does the actual blur procedure
*/

struct BlurProcedure : public Parallel {
	Uint32 *cbuff, *display_buff, *src;
	int xres, yres, frame_num;

	BlurProcedure(Uint32 *_src, Uint32 *_display_buff, int xr, int yr, int fn)
	{
		src = _src;
		display_buff = _display_buff;
		xres = xr;
		yres = yr;
		frame_num = fn;
	}
	void entry(int ti, int tc)
	{
		buffer_minus(accumulation_buffer, cbuff, xres, yres, ti, tc);
		blur_forward(cbuff, src, xres, yres, ti, tc);
		buffer_plus(accumulation_buffer, cbuff, xres, yres, ti, tc);
		if (blur_method == BLUR_DISCRETE) {
			if (frame_num % 8 == 0)
				blur_backward(tempstor, accumulation_buffer, xres, yres, ti, tc);
			mt_fb_memcpy(display_buff, tempstor, xres, yres, ti, tc);
		} else
			blur_backward(display_buff, accumulation_buffer, xres, yres, ti, tc);
	}
};

void blur_do(Uint32 *src, Uint32 *display_buff, int xres, int yres, int frame_num, ThreadPool &thread_pool)
{
	BlurProcedure bp(src, display_buff, xres, yres, frame_num);
	bp.cbuff = blurbuffers[blur_current];
	thread_pool.run(&bp, cpu.count);

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
		blur_forward(blurbuffers[i], framebuffer, xres(), yres(), 0, 1);
	blur_forward(accumulation_buffer, framebuffer, xres(), yres(), 0, 1);

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

void blur_init(void)
{
	int size = xres()*yres()*4;
	blur_close();
	for (int i = 0; i < 8; i++) {
		blurbuffers[i] = (Uint32*) sse_malloc(size);
	}
	accumulation_buffer = (Uint32 *) sse_malloc(size);
	tempstor = (Uint32*) sse_malloc(size);
}

void blur_close(void)
{
	for (int i = 0; i < 8; i++)
		if (blurbuffers[i]) { sse_free(blurbuffers[i]); blurbuffers[i] = NULL; }
	if (accumulation_buffer) { sse_free(accumulation_buffer); accumulation_buffer = NULL; }
	if (tempstor) { sse_free(tempstor); tempstor = NULL; }
}

#endif // BLUR
