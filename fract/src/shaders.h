/***************************************************************************
 *   Copyright (C) 2004 by Veselin Georgiev                                *
 *   vesko@ViruS                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef __SHADERS_H__
#define __SHADERS_H__

#include "MyTypes.h"

// if defined, this will reduce the size of the shaders.cpp framebuffers to save some memory
//#define SAVE_MEM

#ifdef SAVE_MEM
#define SHADER_MAXX 8
#define SHADER_MAXY  8
#define MAX_FFT_SIZE 8
#else // full config:
#define SHADER_MAXX 1280
#define SHADER_MAXY  960
#define MAX_FFT_SIZE 1024
#endif // SAVE_MEM

#define MA3X_MAX_SIZE 8

//["None", "EdgeGlow", "Sobel", "FFTFilter", "Blur", "Inversion", "ObjectGlow"]
// SHADER ID DEFINES:
#define SHADER_ID_EDGE_GLOW		0X00000010
#define SHADER_ID_SOBEL			0X00000020
#define SHADER_ID_FFT_FILTER		0X00000040
#define SHADER_ID_INVERSION		0X00000080
#define SHADER_ID_OBJECT_GLOW		0X00000100
#define SHADER_ID_BLUR			0X00000200

#define SHADER_ID_SHUTTERS		0X00000001
#define SHADER_ID_GAMMA			0x00000002



typedef struct{
float re, im;
}complex;

typedef struct {
	int coeff[MA3X_MAX_SIZE][MA3X_MAX_SIZE];
	int n;
	int shiftrequired;
}ConvolveMatrix;

extern ConvolveMatrix pshop_ma3x_3, pshop_ma3x_5, pshop_ma3x_7;
extern ConvolveMatrix blur_ma3x_3, blur_ma3x_5;
extern int pp_state;
extern float shader_param;

void shader_cmdline_option(const char *opt);
void effects_init(void);
void shaders_close(void);

void fft_2d_complex(complex *src, complex *dst, int n, int isi);

/* Does FFT on the whole image, applies the given filter and does an inverted FFT. Quite slow. */
// CAN DO THE EFFECT IN-PLACE
int shader_FFT_Filter(Uint32 *src, Uint32 *dst, int resx, int resy);

/* Does convolution with the given ConvolveMatrix. Matrices may be of size 3x3, 5x5 and 7x7 with the
   3x3 and 5x5 version being exceptably fast on MMX enabled machines*/
// CAN'T DO THE EFFECT IN-PLACE
void shader_convolution(Uint32 *src, Uint32 *dest, int resx, int resy, ConvolveMatrix *M);




/* Does an Edge Glow effect using convolution with 5x5 matrix and 3x3 matrix blur */
// CAN DO THE EFFECT IN-PLACE
void shader_edge_glow(Uint32 *src, Uint32 *dest, int resx, int resy);

// does a simple matrix blur. blur_size indicates the amount of blur can be 3, 5 or 7
void shader_blur(Uint32 *src, Uint32 *dest, int resx, int resy, int blur_size);

void shader_gamma_shl(Uint32 *src, Uint32 *dest, int resx, int resy, int shift);
void shader_gamma_shr(Uint32 *src, Uint32 *dest, int resx, int resy, int shift);

// does geometrical inversion with a Pole at (resx/2, resy/2) and the given radius
// can't do the effect in-place
void shader_inversion(Uint32 *src, Uint32 *dest, int resx, int resy, double radius);

void shader_outro_effect(Uint32 *fb);

void shader_sobel(Uint32 *src, Uint32 *dest, int resx, int resy);

/**
 * shader_prepare_for_glow
 * @brief prepares an intensity buffer to be used later with shader_obeject glow
 * @param which      - a string for object identification. Example: "all", "mesh:0", "sphere:5"
 * @param prefb      - a framebuffer with object IDs, in fact, the one from the prefiller part
 * @param glowbuff   - the "intensity" buffer which is generated as a result
 * @param resx       - x resolution
 * @param resy       - y resolution
*/ 
void shader_prepare_for_glow(const char *which, const Uint32 *prefb, Uint8 * glowbuff, int resx, int resy);
/**
 * shader_object_glow
 * @brief implements the soft object glow, as proposed by HUMUS
 * @param fb         - the destination framebuffer
 * @param glowbuff   - the intensity buffer, as prepared by shader_prepare_for_glow()
 * @param glow_color - RGB24 standart color for the glow (should match object's own color)
 * @param resx       - x resolution
 * @param resy       - y resolution
 * @param effect_intensity - how intense should the glow be in the range [0.0-1.0]
*/
void shader_object_glow(Uint32 *fb, Uint8 * glowbuff, Uint32 glow_color, int resx, int resy, float effect_intensity);

// Displays shutters with color `col' and intensity `amount' (0-1)
void shader_shutters(Uint32 *fb, Uint32 col, int resx, int resy, float amount);

// Multiplies the pixels by the given gamma (0-1)
void shader_gamma(Uint32 *fb, int resx, int resy, float multiplier);

#endif //__SHADERS_H__
