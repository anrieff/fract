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
 *   rgb2yuv.cpp - handles RGB-to-YUV colorspace convesions.               *
 *                                                                         *
 ***************************************************************************/

#define RGB2YUV_CPP
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "MyTypes.h"
#include "asmconfig.h"
#include "bitmap.h"
#include "cmdline.h"
#include "cpu.h"
#include "fract.h"
#include "rgb2yuv.h"
#include "progress.h"


int bestmethod=0;
int yuv_type = 0;

Uint32 checksums[2][2][4] = {
	{ /*AMD*/
/*LinuX*/	{ 0x654A9447, 0x5FCB94B2, 0x5FC09B67, 0x5B50DC1B },
/*WindowS*/	{ 0x654B6199, 0x5FCC61F6, 0x5FC16BEF, 0x5B50DC1B },
	},
	{ /*Intel*/
/*LinuX*/	{ 0x654B6199, 0x5FCC61F6, 0x5FC16BEF, 0x5B4DEA18 },
/*WindowS*/	{ 0x654B6199, 0x5FCC61F6, 0x5FC16BEF, 0x5B4DEA18 }
	}
};

Uint32 csc=0xffffffff;

// these are used for benchmarking purposes
Uint32 fakeRGBBuffer[BENCHSIZE], fakeYUVBuffer[BENCHSIZE];

// ConvertRGB2YUV_X86 - convers [count] pixels from [*src] to [*dest]
// transformation is FROM RGB colorspace TO YUY2 packed format
// the only portable function  :) Uses i386 arighmetics only. Fairly precise
// count must be even
void ConvertRGB2YUV_X86(Uint32 *dest, Uint32 *src, size_t count)
{
	register int r, g, b;
	int r1, g1, b1;
	count /= 2;
	while (count--) {
		b = (*src) & 0xff;
		g = ((*src)>>8) & 0xff;
		r = ((*src)>>16) & 0xff;
		src++;
		b1 = (*src) & 0xff;
		g1 = ((*src)>>8) & 0xff;
		r1 = ((*src)>>16) & 0xff;
		(*dest) = /* Y0 */ (((0x00100000 + r * IM11 + g * IM12 + b * IM13)>>16)&0x000000ff)
			| /* U0 */ (((0x00800000 + r * IM31 + g * IM32 + b * IM33)>> 8)&0x0000ff00)
			| /* V0 */ (((0x00800000 + r * IM21 + g * IM22 + b * IM23)<< 8)&0xff000000)
			| /* Y1 */ (((0x00100000 + r1* IM11 + g1* IM12 + b1* IM13)    )&0x00ff0000)
			;
		dest++;
		src++;
	}
}

/*
 *
 */
void ConvertRGB2YV12_X86(Uint8 *Y, Uint8 *U, Uint8 *V, Uint32 *src, int x0, int y0, int w, int h, int pitch)
{
	assert(x0 % 2 == 0);
	assert(y0 % 2 == 0);
	assert(w % 2 == 0);
	assert(h % 2 == 0);
	assert(pitch % 2 == 0);
	src += x0 + y0 * pitch;
	Y += x0 + y0 * pitch;
	U += (x0/2) + (y0/2) * pitch/2;
	V += (x0/2) + (y0/2) * pitch/2;
	for (int j = 0; j < h; j += 2) {
		for (int i = 0; i < w; i += 2, src+=2, Y += 2, U++, V++) {
			Uint32 r, g, b;
			r =  src[0]        & 0xff;
			g = (src[0] >>  8) & 0xff;
			b = (src[0] >> 16) & 0xff;
			Y[0] = (((0x00100000 + r * IM11 + g * IM12 + b * IM13)>>16)&0x000000ff);
			U[0] = (((0x00800000 + r * IM31 + g * IM32 + b * IM33)>>16)&0x000000ff);
			V[0] = (((0x00800000 + r * IM21 + g * IM22 + b * IM23)>>16)&0x000000ff);
			r =  src[1]        & 0xff;
			g = (src[1] >>  8) & 0xff;
			b = (src[1] >> 16) & 0xff;
			Y[1] = (((0x00100000 + r * IM11 + g * IM12 + b * IM13)>>16)&0x000000ff);
			r =  src[pitch]        & 0xff;
			g = (src[pitch] >>  8) & 0xff;
			b = (src[pitch] >> 16) & 0xff;
			Y[pitch] = (((0x00100000 + r * IM11 + g * IM12 + b * IM13)>>16)&0x000000ff);
			r =  src[pitch+1]        & 0xff;
			g = (src[pitch+1] >>  8) & 0xff;
			b = (src[pitch+1] >> 16) & 0xff;
			Y[pitch+1] = (((0x00100000 + r * IM11 + g * IM12 + b * IM13)>>16)&0x000000ff);
		}
		src = src-w+2*pitch, Y = Y - w + 2*pitch, U = U - w/2 + pitch/2, V = V - w/2 + pitch/2;
	}
}

// include assembly rgb2yuv routines:
#define rgb2yuv_asm
#include "x86_asm.h"
#undef rgb2yuv_asm


// Master function, switches to the fastest available conversion routine
/***
 *** you may wonder why I've included the X86 version at all (it'll be always slower than the X86_ASM version).
 *** well, someday this thing would be really portable ... hopefully...
 *************************************************************************************************************/

void ConvertRGB2YUV(Uint32 *dest, Uint32 *src, size_t count)
{
	switch (bestmethod) {
		case USE_X86: ConvertRGB2YUV_X86(dest, src, count); break;
		case USE_MMX: ConvertRGB2YUV_MMX(dest, src, count); break;
		case USE_X86_ASM: ConvertRGB2YUV_X86_ASM(dest, src, count); break;
		case USE_X86_FPU: ConvertRGB2YUV_X86_FPU(dest, src, count); break;
		case USE_MMX2: ConvertRGB2YUV_MMX2(dest, src, count); break;
		case USE_SSE: ConvertRGB2YUV_SSE(dest, src, count); break;
	}
}

void ConvertRGB2YV12(Uint8 *Y, Uint8 *U, Uint8 *V, Uint32 *rgb, int x0, int y0, int w, int h, int pitch)
{
	ConvertRGB2YV12_X86(Y, U, V, rgb, x0, y0, w, h, pitch);
}

// Benchmarks a RGB-to-YUY2 conversion function.
// Exact results may depend on the test data (fakeRGBBuffer) and the BENCHSIZE
// random data is a killer for ConvertRGB2YUV_MEM, as it performs about 6-10 times worse, but works fine on real data.
void benchmark_function( __convert_fn_t fu, int code, char *method, int *maxfps, Uint32 timetorun)
{
	int cfps, frames=0;
	Uint32 ticks;
	printf("  ConvertRGB2YUV%s: ", method);
	fu(fakeYUVBuffer, fakeRGBBuffer, BENCHSIZE);
	ticks = get_ticks();
	while (ticks == get_ticks())
		/*nothing*/;
	ticks = get_ticks();
	while (get_ticks()-ticks<timetorun) {
		fu(fakeYUVBuffer, fakeRGBBuffer, BENCHSIZE);
		frames++;
	}
	ticks = get_ticks() - ticks;
	cfps = frames * 1000 / ticks;
	printf("%d fps\n", cfps);
	if (cfps > *maxfps) { 
		*maxfps = cfps; 
		bestmethod = code; 
	}
	fflush(stdout);
}

/*
	This is called only when RGB2YUV conversion is needed.
	The benchmark flag is just passed to yuv_benchmark and defines the
	amount of time each function is benchmarked.
*/
static bool yuv_benchmarked = false;
void init_yuv_convert(int benchmark)
{
	if (yuv_benchmarked) return;
	yuv_benchmarked = true;
	int i;
	// disable all benchmarking and algorithm selection if we have some input on the command line
	if (	option_exists("--use-x86")    ||option_exists("--use-x86asm")||
		option_exists("--use-mmx")    ||option_exists("--use-mem")   ||
		option_exists("--use-x86-fpu")||option_exists("--use-sse")   ||
		option_exists("--use-mmx2")) {
		bestmethod = -1;
		if (option_exists("--use-x86")) bestmethod = USE_X86;
	#ifdef USE_ASSEMBLY
		if (option_exists("--use-x86asm")) bestmethod = USE_X86_ASM;
		if (option_exists("--use-x86-fpu")) bestmethod = USE_X86_FPU;
	#endif
		if (option_exists("--use-mmx") && cpu.mmx) bestmethod = USE_MMX;
		if (option_exists("--use-sse") && cpu.mmx2 && cpu.sse) bestmethod = USE_SSE;
		if (option_exists("--use-mmx2") && cpu.mmx2) bestmethod = USE_MMX2;
		if (bestmethod!=-1) return;
	}
	
	// put random data in the RGB buffer... this will produce more realistic results when benchmarking
	// all convesion functions that utilize lookup tables (will eliminate cache on large tables)
	for (i=0;i<BENCHSIZE;i++)
		fakeRGBBuffer[i] = rand()%0x1000000;
	yuv_benchmark(benchmark);
}

Uint32 get_correct_cksum(void)
{
	int os=1, iss;
	if (csc!=0xffffffff) return csc;
#ifdef linux
	os=0;
#endif
	iss = 0;
	if (cpu.mmx) iss++;
	if (cpu.mmx2) iss++;
	if (cpu.sse) iss++;
	csc = checksums[cpu.is_intel()][os][iss];
	return csc;
}

void switch_rgbmethod(void)
{
	switch (bestmethod) {
		case USE_X86	: bestmethod = USE_X86_ASM;	break;
		case USE_X86_ASM: bestmethod = USE_X86_FPU;	break;
		case USE_X86_FPU: bestmethod = USE_MMX;		break;
		case USE_MMX	: bestmethod = USE_MMX2;	break;
		case USE_MMX2	: bestmethod = USE_SSE;		break;
		case USE_SSE	: bestmethod = USE_X86;		break;
	}
}

void set_correct_cksum(Uint32 c)
{
	csc = c;
}

void rgb2yuv_close(void)
{
}

// the parameter is to specify --benchmark mode. If set, all conversion functions are run one minute unconditionally.
void yuv_benchmark(int benchmark)
{
	Task task(YUVBENCH, __FUNCTION__);
	task.set_steps( 3 + cpu.mmx + cpu.mmx2 + cpu.sse );
	int maxfps = 0;
	int timetorun;
	timetorun = (benchmark?BENCH_LARGE_TICKS:BENCHTICKS);
	printf("Benchmarking RGB-to-YUV conversion functions:\n");
	benchmark_function((__convert_fn_t) ConvertRGB2YUV_X86, USE_X86, "_X86", &maxfps, timetorun); task++;
#ifdef USE_ASSEMBLY
	benchmark_function((__convert_fn_t) ConvertRGB2YUV_X86_ASM, USE_X86_ASM, "_X86_ASM", &maxfps, timetorun); task++;
	benchmark_function((__convert_fn_t) ConvertRGB2YUV_X86_FPU, USE_X86_FPU, "_X86_FPU", &maxfps, timetorun); task++;
#endif	
	if (cpu.mmx) {
		benchmark_function((__convert_fn_t) ConvertRGB2YUV_MMX, USE_MMX, "_MMX", &maxfps, timetorun); 
		task++;
	}
	if (cpu.mmx2) {
		benchmark_function((__convert_fn_t) ConvertRGB2YUV_MMX2, USE_MMX2, "_MMX2", &maxfps, timetorun);
		task++;
	}
	if (cpu.mmx2 && cpu.sse) {
		benchmark_function((__convert_fn_t) ConvertRGB2YUV_SSE, USE_SSE, "_SSE", &maxfps, timetorun);
		task++;
	}
}

void RGB2YUV_UpdateOverlay(Uint32 *plane0, Uint32 *plane1, Uint32 *plane2, Uint32 *rgb, int x0, int y0, int w, int h, int pitch)
{
	if (yuv_type == 0) {
		// YUY2:
		Uint16 *p = (Uint16*) plane0;
		if (x0 == 0 && y0 == 0 && w == pitch)
			ConvertRGB2YUV(plane0, rgb, w*h);
		else for (int j = y0; j < h + y0; j++)
			ConvertRGB2YUV_X86((Uint32*)(p + x0 + j * pitch),
			                   rgb + x0 + j * pitch, w);
	} else {
		ConvertRGB2YV12((Uint8*)plane0, (Uint8*)plane1, (Uint8*)plane2, rgb, x0, y0, w, h, pitch);
	}
}
