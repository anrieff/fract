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


int bestmethod_yuv = 0;
int bestmethod_yv12 = 0;
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
Uint8 fakeYBuffer[BENCHSIZE], fakeUBuffer[BENCHSIZE/4], fakeVBuffer[BENCHSIZE/4];

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

void ssConvertRGB2YUV_MMX2(Uint32 *dest, Uint32 *src, size_t count)
{
 SSE_ALIGN(Sint16 pmm3[4])={SIM13, SIM33, SIM13, SIM23};
 SSE_ALIGN(Sint16 pmm4[4])={SIM12, SIM32, SIM12, SIM22};
 SSE_ALIGN(Sint16 pmm5[4])={SIM11, SIM31, SIM11, SIM21};
 SSE_ALIGN(Uint16 pmm6[4])={4096, 32768, 4096, 32768};
 SSE_ALIGN(Uint16 pmm7[4])={0xffff, 0x00ff, 0xffff, 0x00ff};
 //Sint16 pmm7[4]={0, 0, 0, 0};

 count /= 2;

 // load some constants:
 __asm __volatile (
 "	pxor	%%mm7,	%%mm7\n"
 "	movq	%0,	%%mm6\n"
 ::"m"(*pmm7):"memory"
 );

 while (count--) {
 	__asm __volatile (
	"	movq	%1,	%%mm0\n"
	"	pand	%%mm6,	%%mm0\n"
	"	pshufw	$0x0e,	%%mm0,	%%mm1\n"
// mm0 contains pixel 1 and pixel 2
// low dword of mm1 contains pixel 2

	"	punpcklbw	%%mm7,	%%mm0\n"
	"	punpcklbw	%%mm7,	%%mm1\n"

// mm0 = (b0 g0 r0 00) (words)
// mm1 = (b1 g1 r1 00) (words)

// SOME REAALY WEIRD SHUFFLES::: :)
	"	pshufw	$0x30,	%%mm0,	%%mm2\n" // 00110000
	"	pshufw	$0xcf,	%%mm1,	%%mm3\n" // 11001111
	"	pshufw	$0x75,	%%mm0,	%%mm4\n" // 01110101
	"	pshufw	$0xdf,	%%mm1,	%%mm5\n" // 11011111
	"	pshufw	$0xba,	%%mm0,	%%mm0\n" // 10111010
	"	pshufw	$0xef,	%%mm1,	%%mm1\n" // 11101111
// ends up like this:
	"	por	%%mm3,	%%mm2\n" // mm2 - b0 b0 b1 b0
	"	por	%%mm5,	%%mm4\n" // mm4 - g0 g0 g1 g0
	"	por	%%mm1,	%%mm0\n" // mm0 - r0 r0 r1 r0

	"	pmullw	%2,	%%mm2\n" // mm2 *= M13 M33 M13 M23
	"	pmullw	%3,	%%mm4\n" // mm4 *= M12 M32 M12 M22
	"	pmullw	%4,	%%mm0\n" // mm0 *= M11 M31 M11 M21

	"	paddw	%5,	%%mm2\n" // mm2 += 16  128  16 128
	"	paddw	%%mm4,	%%mm0\n"
	"	paddw	%%mm2,	%%mm0\n"
	"	psrlw	$8,	%%mm0\n" // mm0 = Y0 U0 Y1 V0

	"	packuswb	%%mm7,	%%mm0\n"
	"	movd	%%mm0,	%0\n"


	:"=m"(*dest)
	:"m"(*src), "m"(*pmm3), "m"(*pmm4), "m"(*pmm5), "m"(*pmm6)
	:"memory"
	);
 	src+=2;
	dest++;
 }

 __asm __volatile("emms");
}

void ConvertRGB2YV12_MMX2(Uint8 *Y, Uint8 *U, Uint8 *V, Uint32 *src, int x0, int y0, int w, int h, int pitch)
{
	assert(x0 % 2 == 0);
	assert(y0 % 2 == 0);
	assert(w % 4 == 0);
	assert(h % 4 == 0);
	assert(pitch % 4 == 0);
	//
	SSE_ALIGN(Sint16 pmm3[4])={SIM13, SIM13, SIM33, SIM23};
	SSE_ALIGN(Sint16 pmm4[4])={SIM12, SIM12, SIM32, SIM22};
	SSE_ALIGN(Sint16 pmm5[4])={SIM11, SIM11, SIM31, SIM21};
	SSE_ALIGN(Uint16 pmm6[4])={4096,   4096, 32768, 32768};
	SSE_ALIGN(Uint16 pmm7[4])={0xffff, 0x00ff, 0xffff, 0x00ff};
	//
	src += x0 + y0 * pitch;
	Y += x0 + y0 * pitch;
	U += (x0/2) + (y0/2) * pitch/2;
	V += (x0/2) + (y0/2) * pitch/2;
	__asm __volatile(
	// -- init
	"	mov	%3,		%%esi\n"
	"	mov	%0,		%%edi\n"
	"	pxor	%%mm7,		%%mm7\n"
	"	movq	%13,		%%mm6\n"
	
	".balign	16\n"
	".y:\n"
	"	mov	%6,		%%ecx\n"
	
	".balign	16\n"
	".x:\n"
	"	movq	(%%esi),	%%mm0\n"
	"	pand	%%mm6,		%%mm0\n"
	"	pshufw	$0x0e,	%%mm0,	%%mm1\n"

	"	punpcklbw	%%mm7,	%%mm0\n"
	"	punpcklbw	%%mm7,	%%mm1\n"
// mm0 = (b0 g0 r0 00) (words)
// mm1 = (b1 g1 r1 00) (words)
// As in the packed version, 6 beautiful weird shuffles
	"	pshufw	$0x0c,	%%mm0,	%%mm2\n" // 00001100
	"	pshufw	$0xf3,	%%mm1,	%%mm3\n" // 11110011
	"	pshufw	$0x5d,	%%mm0,	%%mm4\n" // 01011101
	"	pshufw	$0xf7,	%%mm1,	%%mm5\n" // 11110111
	"	pshufw	$0xae,	%%mm0,	%%mm0\n" // 10101110
	"	pshufw	$0xfb,	%%mm1,	%%mm1\n" // 11111011
// ends up like this:
	"	por	%%mm3,		%%mm2\n" // mm2 - b0 b1 b0 b0
	"	por	%%mm5,		%%mm4\n" // mm4 - g0 g1 g0 g0
	"	por	%%mm1,		%%mm0\n" // mm0 - r0 r1 r0 r0

	"	pmullw	%9,		%%mm2\n" // mm2 *= M13 M13 M33 M23
	"	pmullw	%10,		%%mm4\n" // mm4 *= M12 M12 M32 M22
	"	pmullw	%11,		%%mm0\n" // mm0 *= M11 M11 M31 M21

	"	paddw	%12,		%%mm2\n" // mm2 += 16   16 128 128
	"	paddw	%%mm4,		%%mm0\n"
	"	paddw	%%mm2,		%%mm0\n"
	"	psrlw	$8,		%%mm0\n" // mm0 = Y0 Y1 U0 V0

	"	packuswb	%%mm7,	%%mm0\n"
	"	movd	%%mm0,		%%eax\n"
	
	//store Y0, Y1, U0, V0
	"	movw	%%ax,		(%%edi)\n"
	"	shr	$16,		%%eax\n"
	"	push	%%esi\n"
	"	push	%%edi\n"
	"	push	%%edi\n"
	"	mov	%1,		%%edi\n"
	"	movb	%%ah,		(%%edi)\n"
	"	incl	%1\n"
	"	mov	%2,		%%edi\n"
	"	movb	%%al,		(%%edi)\n"
	"	incl	%2\n"
	"	pop	%%edi\n"
	
	// -- go to the line below:
	
	"	mov	%8,		%%eax\n"
	
	"	lea	(%%esi, %%eax, 4),	%%esi\n"
	"	lea	(%%edi, %%eax),	%%edi\n"
	
	// fetch another two pixels:
	
	"	movq	(%%esi),	%%mm0\n"
	"	pand	%%mm6,		%%mm0\n"
	"	pshufw	$0x0e,	%%mm0,	%%mm1\n"

	"	punpcklbw	%%mm7,	%%mm0\n"
	"	punpcklbw	%%mm7,	%%mm1\n"
// mm0 = (b0 g0 r0 00) (words)
// mm1 = (b1 g1 r1 00) (words)
// As in the packed version, 6 beautiful weird shuffles
	"	pshufw	$0x0c,	%%mm0,	%%mm2\n" // 00001100
	"	pshufw	$0xf3,	%%mm1,	%%mm3\n" // 11110011
	"	pshufw	$0x5d,	%%mm0,	%%mm4\n" // 01011101
	"	pshufw	$0xf7,	%%mm1,	%%mm5\n" // 11110111
	"	pshufw	$0xae,	%%mm0,	%%mm0\n" // 10101110
	"	pshufw	$0xfb,	%%mm1,	%%mm1\n" // 11111011
// ends up like this:
	"	por	%%mm3,		%%mm2\n" // mm2 - b0 b1 b0 b0
	"	por	%%mm5,		%%mm4\n" // mm4 - g0 g1 g0 g0
	"	por	%%mm1,		%%mm0\n" // mm0 - r0 r1 r0 r0

	"	pmullw	%9,		%%mm2\n" // mm2 *= M13 M13 M33 M23
	"	pmullw	%10,		%%mm4\n" // mm4 *= M12 M12 M32 M22
	"	pmullw	%11,		%%mm0\n" // mm0 *= M11 M11 M31 M21

	"	paddw	%12,		%%mm2\n" // mm2 += 16   16 128 128
	"	paddw	%%mm4,		%%mm0\n"
	"	paddw	%%mm2,		%%mm0\n"
	"	psrlw	$8,		%%mm0\n" // mm0 = Y0 Y1 U0 V0

	"	packuswb	%%mm7,	%%mm0\n"
	"	movd	%%mm0,		%%eax\n"
	
	//store Y2, Y3
	"	movw	%%ax,		(%%edi)\n"
	
	"	pop	%%edi\n"
	"	pop	%%esi\n"
	
	"	lea	2(%%edi),	%%edi\n"
	"	lea	8(%%esi),	%%esi\n"
	"	sub	$2,		%%ecx\n"
	"	jnz	.x\n"
	
	"	mov	%8,	%%eax\n"
	"	shl	$1,	%%eax\n"
	"	sub	%6,	%%eax\n"
	"	lea	(%%edi,	%%eax),		%%edi\n"
	"	lea	(%%esi,	%%eax, 4),	%%esi\n"
	
	"	sub	%8,	%%eax\n"
	"	shr	$1,	%%eax\n"
	"	mov	%1,	%%ecx\n"
	"	sub	%%eax,	%%ecx\n"
	"	mov	%%ecx,	%1\n"
	"	mov	%2,	%%ecx\n"
	"	sub	%%eax,	%%ecx\n"
	"	mov	%%ecx,	%2\n"
	
	"	decl	%7\n"
	"	decl	%7\n"
	"	jnz	.y\n"
	
	"	emms\n"
	:
	//   0       1       2        3        4        5        6       7       8           9            10          11          12          13
	:"m"(Y), "m"(U), "m"(V), "m"(src), "m"(x0), "m"(y0), "m"(w), "m"(h), "m"(pitch), "m"(*pmm3), "m"(*pmm4), "m"(*pmm5), "m"(*pmm6), "m"(*pmm7)
	:"memory", "eax", "edi", "esi", "ecx"
	);
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
	switch (bestmethod_yuv) {
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
	switch (bestmethod_yv12) {
		case USE_X86: ConvertRGB2YV12_X86(Y, U, V, rgb, x0, y0, w, h, pitch); break;
		case USE_MMX2: ConvertRGB2YV12_MMX2(Y, U, V, rgb, x0, y0, w, h, pitch); break;
	}
}

// Benchmarks a RGB-to-YUY2 conversion function.
// Exact results may depend on the test data (fakeRGBBuffer) and the BENCHSIZE
// random data is a killer for ConvertRGB2YUV_MEM, as it performs about 6-10 times worse, but works fine on real data.
void benchmark_function(void *fu, int code, char *method, int &maxfps, int &bestmethod, Uint32 timetorun)
{
	int cfps, frames=0;
	Uint32 ticks;
	printf("  ConvertRGB2%s: ", method);
	__convert_fn_t fun_yuv = NULL;
	__convert_fn_t_yv12 fun_yv12 = NULL;
	if (method[1] != 'V')
		fun_yuv = (__convert_fn_t) fu;
	else
		fun_yv12 = (__convert_fn_t_yv12) fu;
	if (fun_yuv)
		fun_yuv(fakeYUVBuffer, fakeRGBBuffer, BENCHSIZE);
	else
		fun_yv12(fakeYBuffer, fakeUBuffer, fakeVBuffer, fakeRGBBuffer, 0, 0, BENCHSIZE_X, BENCHSIZE_Y, BENCHSIZE_X);
	ticks = get_ticks();
	while (ticks == get_ticks())
		/*nothing*/;
	ticks = get_ticks();
	while (get_ticks()-ticks<timetorun) {
		if (fun_yuv)
			fun_yuv(fakeYUVBuffer, fakeRGBBuffer, BENCHSIZE);
		else
			fun_yv12(fakeYBuffer, fakeUBuffer, fakeVBuffer, fakeRGBBuffer, 0, 0, BENCHSIZE_X, BENCHSIZE_Y, BENCHSIZE_X);
		frames++;
	}
	ticks = get_ticks() - ticks;
	cfps = frames * 1000 / ticks;
	printf("%d fps\n", cfps);
	if (cfps > maxfps) {
		maxfps = cfps;
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
		bestmethod_yuv = -1;
		if (option_exists("--use-x86")) bestmethod_yuv = USE_X86;
	#ifdef USE_ASSEMBLY
		if (option_exists("--use-x86asm")) bestmethod_yuv = USE_X86_ASM;
		if (option_exists("--use-x86-fpu")) bestmethod_yuv = USE_X86_FPU;
	#endif
		if (option_exists("--use-mmx") && cpu.mmx) bestmethod_yuv = USE_MMX;
		if (option_exists("--use-sse") && cpu.mmx2 && cpu.sse) bestmethod_yuv = USE_SSE;
		if (option_exists("--use-mmx2") && cpu.mmx2) bestmethod_yuv = USE_MMX2;
		if (bestmethod_yuv!=-1) return;
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
	switch (bestmethod_yuv) {
		case USE_X86	: bestmethod_yuv = USE_X86_ASM;	break;
		case USE_X86_ASM: bestmethod_yuv = USE_X86_FPU;	break;
		case USE_X86_FPU: bestmethod_yuv = USE_MMX;	break;
		case USE_MMX	: bestmethod_yuv = USE_MMX2;	break;
		case USE_MMX2	: bestmethod_yuv = USE_SSE;	break;
		case USE_SSE	: bestmethod_yuv = USE_X86;	break;
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
	int maxfps_yv12 = 0;
	int timetorun;
	timetorun = (benchmark?BENCH_LARGE_TICKS:BENCHTICKS);
	printf("Benchmarking RGB-to-YUY2 conversion functions:\n");
	benchmark_function((void*)ConvertRGB2YV12_X86, USE_X86, "YV12_X86", maxfps_yv12, bestmethod_yv12, timetorun);
	benchmark_function((void*)ConvertRGB2YV12_MMX2, USE_MMX2, "YV12_MMX2", maxfps_yv12, bestmethod_yv12, timetorun);
	printf("Benchmarking RGB-to-YUY2 conversion functions:\n");
	benchmark_function((void*)ConvertRGB2YUV_X86, USE_X86, "YUV_X86", maxfps, bestmethod_yuv, timetorun); task++;
#ifdef USE_ASSEMBLY
	benchmark_function((void*)ConvertRGB2YUV_X86_ASM, USE_X86_ASM, "YUV_X86_ASM", maxfps, bestmethod_yuv, timetorun); task++;
	benchmark_function((void*)ConvertRGB2YUV_X86_FPU, USE_X86_FPU, "YUV_X86_FPU", maxfps, bestmethod_yuv, timetorun); task++;
#endif	
	if (cpu.mmx) {
		benchmark_function((void*)ConvertRGB2YUV_MMX, USE_MMX, "YUV_MMX", maxfps, bestmethod_yuv, timetorun); 
		task++;
	}
	if (cpu.mmx2) {
		benchmark_function((void*)ConvertRGB2YUV_MMX2, USE_MMX2, "YUV_MMX2", maxfps, bestmethod_yuv, timetorun);
		task++;
	}
	if (cpu.mmx2 && cpu.sse) {
		benchmark_function((void*)ConvertRGB2YUV_SSE, USE_SSE, "YUV_SSE", maxfps, bestmethod_yuv, timetorun);
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
			ConvertRGB2YUV((Uint32*)(p + x0 + j * pitch),
			               rgb + x0 + j * pitch, w);
	} else {
		ConvertRGB2YV12((Uint8*)plane0, (Uint8*)plane1, (Uint8*)plane2, rgb, x0, y0, w, h, pitch);
	}
}
