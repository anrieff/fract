/***************************************************************************
 *   Copyright (C) 2004 by Veselin Georgiev                                *
 *   vesko@ViruS                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#define USE_X86 0
#define USE_X86_ASM 1
#define USE_X86_FPU 2
#define USE_MMX 3
#define USE_MMX2 4
#define USE_SSE 5
#define USE_BIGMEM 10

// define the benchmark buffer size as to represent 320x240 framebuffer
#define BENCHSIZE 76800
#define BENCHSIZE_X 320
#define BENCHSIZE_Y 240
// benchmark each function 250 milliseconds
#define BENCHTICKS 250
// in benchmark mode, run all functions for a minute unconditionally
#define BENCH_LARGE_TICKS 60000

/* I'm using the following formula for RGB to YUV colorspace convertion:

  Y  =      (0.257 * R) + (0.504 * G) + (0.098 * B) + 16
  Cr = V =  (0.439 * R) - (0.368 * G) - (0.071 * B) + 128
  Cb = U = -(0.148 * R) - (0.291 * G) + (0.439 * B) + 128

  it is taken from http://www.fourcc.org/index.php?http%3A//www.fourcc.org/fccyvrgb.php

*/

// here's the multiplication matrix
#define M11  0.257
#define M12  0.504
#define M13  0.098
#define M21  0.439
#define M22 -0.368
#define M23 -0.071
#define M31 -0.148
#define M32 -0.291
#define M33  0.439
// and the integer arithmetics 16.16 fixedpoint equivallents:
#define IM11   16843
#define IM12   33030
#define IM13    6423
#define IM21   28770
#define IM22  -24117
#define IM23   -4653
#define IM31   -9699
#define IM32  -19071
#define IM33   28770

// also the integer equivallents for 8.8 arithmetics, used for MMX...
#define SIM11    66
#define SIM12   129
#define SIM13    25
#define SIM21   112
#define SIM22   -94
#define SIM23   -18
#define SIM31   -38
#define SIM32   -74
#define SIM33   112
// using 8 bits only to represent the fractional part of the multipliers may seem rather inprecise.
// The epsilon is actually 0.004, which should be acceptably small

// also the integer equivallents for 4.4 arith... oh just kidding :)


typedef void (*__convert_fn_t) (Uint32 *, Uint32 *, size_t);
typedef void (*__convert_fn_t_yv12) (Uint8 *, Uint8 *, Uint8 *, Uint32 *, int, int, int, int, int);
extern int yuv_type;

void ConvertRGB2YUV(Uint32 *dest, Uint32 *src, size_t count);
/**
 * @brief RGB2YUV_UpdateOverlay - converts a subrectangle of a RGB framebuffer to a YUY2 or YV12 overlay
 * @param plane0 - the first plane of the overlay. In the packed YUY2 mode, this is the only plane. In YV12, this holds Y data.
 * @param plane1 - second plane. YV12 only. Contains U data.
 * @param plane2 - third plane. YV12 only. Contains V data.
 * @param rgb    - the source RGB framebuffer. 32bits per pixel.
 * @param x0, @param y0 - specify the top-left corner of the rectangle to be converted.
 * @param w, @param h - width and height of the rectangle to be converted.
 * @param pitch - the width of the RGB framebuffer, in pixels.
 *
 * NOTE: this function is actually a wrapper around ConvertRGB2YUV() and ConvertRGB2YV12() function. Which
 *       YUV mode to use is selected by the `yuv_type' variable. If 0, RGB->YUY2 is used.
 *       If 1, YV12 is used. In the former, only the first plane of the YUV overlay is used, and the written
 *       data is 16 bits per one input pixel. In the latter, all three planes are used, with the first plane
 *       getting 8bpp Y data, and the other two get 8 bits per four pixels of U anv V data.
 */
void RGB2YUV_UpdateOverlay(Uint32 *plane0, Uint32 *plane1, Uint32 *plane2, Uint32 *rgb, int x0, int y0, int w, int h, int pitch);
void init_yuv_convert(int benchmark);
void yuv_benchmark(int benchmark);
void switch_rgbmethod(void);
Uint32 get_correct_cksum(void);
void set_correct_cksum(Uint32 c);
void rgb2yuv_close(void);
