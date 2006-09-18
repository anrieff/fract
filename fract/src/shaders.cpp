/***************************************************************************
 *   Copyright (C) 2004 by Veselin Georgiev                                *
 *   vesko@ViruS                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *-------------------------------------------------------------------------*
 *  Handles some full-screen transformations                               *
 *                                                                         *
 ***************************************************************************/

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "MyGlobal.h"
#include "MyTypes.h"
#include "shaders.h"
#include "asmconfig.h"
#include "common.h"
#include "cpu.h"
#include "fract.h"
#include "MyLimits.h"
#include "gfx.h"
#include "render.h"
#include "profile.h"


extern int user_pp_state;
int pp_state = 0;
float shader_param = 0.0f;

#define roundint(a) ((int) (a+0.5))


int multi_ma3x_8_int16[8][8] = {
	{ 0x2000, 0x4000, 0x6000, 0x8000, 0x8000, 0x6000, 0x4000, 0x2000 },
	{ 0x4000, 0x6000, 0x8000, 0xa000, 0xa000, 0x8000, 0x6000, 0x4000 },
	{ 0x6000, 0x8000, 0xa000, 0xc000, 0xc000, 0xa000, 0x8000, 0x6000 },
	{ 0x8000, 0xa000, 0xc000, 0xe000, 0xe000, 0xc000, 0xa000, 0x8000 },
	{ 0x8000, 0xa000, 0xc000, 0xe000, 0xe000, 0xc000, 0xa000, 0x8000 },
	{ 0x6000, 0x8000, 0xa000, 0xc000, 0xc000, 0xa000, 0x8000, 0x6000 },
	{ 0x4000, 0x6000, 0x8000, 0xa000, 0xa000, 0x8000, 0x6000, 0x4000 },
	{ 0x2000, 0x4000, 0x6000, 0x8000, 0x8000, 0x6000, 0x4000, 0x2000 }
};

SSE_ALIGN(float multi_ma3x_8_float[8][8]) = {
	{ 0.125, 0.250, 0.375, 0.500, 0.500, 0.375, 0.250, 0.125 },
	{ 0.250, 0.375, 0.500, 0.625, 0.625, 0.500, 0.375, 0.250 },
	{ 0.375, 0.500, 0.625, 0.750, 0.750, 0.625, 0.500, 0.375 },
	{ 0.500, 0.625, 0.750, 0.875, 0.875, 0.750, 0.625, 0.500 },
	{ 0.500, 0.625, 0.750, 0.875, 0.875, 0.750, 0.625, 0.500 },
	{ 0.375, 0.500, 0.625, 0.750, 0.750, 0.625, 0.500, 0.375 },
	{ 0.250, 0.375, 0.500, 0.625, 0.625, 0.500, 0.375, 0.250 },
	{ 0.125, 0.250, 0.375, 0.500, 0.500, 0.375, 0.250, 0.125 }
};

Uint32 shader_tmp[SHADER_MAXX*SHADER_MAXY];
float *fft_filter = NULL;

ConvolveMatrix pshop_ma3x_3 = {
{
	{ +1, -1, +1 },
	{ -1,  0, -1 },
	{ +1, -1, +1 } // the matrix itself
},
	3, // size
	0 // required postprocessing shift
};

ConvolveMatrix pshop_ma3x_5 = {
{
	{ +1, -1, +1, -1, +1 },
	{ -1, +1, -1, +1, -1 },
	{ +1, -1,  0, -1, +1 },
	{ -1, +1, -1, +1, -1 },
	{ +1, -1, +1, -1, +1 } // the matrix itself
},
	5, // size
	2  // requred postprocessing shift
};

ConvolveMatrix pshop_ma3x_5_1 = {
{
	{ -4, +5, -7, +5, -4 }, //-2
	{ +5, -6, +7, -6, +5 }, //+2
	{ -7, +7,  0, +7, -7 },
	{ +5, -6, +7, -6, +5 }, //+2
	{ -4, +5, -7, +5, -4 }  //-2
	// the matrix itself
},
	5, // size
	0  // requred postprocessing shift
};


ConvolveMatrix pshop_ma3x_7 = {
{
	{ +1, -1, +1, -1, +1, -1, +1 },
	{ -1, +1, -1, +1, -1, +1, -1 },
	{ +1, -1, +1, -1, +1, -1, +1 },
	{ -1, +1, -1,  0, -1, +1, -1 },
	{ +1, -1, +1, -1, +1, -1, +1 },
	{ -1, +1, -1, +1, -1, +1, -1 },
	{ +1, -1, +1, -1, +1, -1, +1 } // the matrix itself
},
	7, // size
	1  // required postprocessing shift
};

ConvolveMatrix blur_ma3x_3 = {
{
	{ 1, 2, 1 },
	{ 2, 4, 2 },
	{ 1, 2, 1 }
},
	3 //size;

};

ConvolveMatrix blur_ma3x_5 = {
{
	{ 1, 2, 2, 2, 1 }, // 16
	{ 2, 4, 4, 4, 2 }, // 32
	{ 2, 4, 4, 4, 2 }, // 32
	{ 2, 4, 4, 4, 2 }, // 32
	{ 1, 2, 2, 2, 1 }  // 16
},//---------------------- // 64
	5 //size;
};


ConvolveMatrix blur_ma3x_7 = {
{
	{ 1, 1, 1, 2, 1, 1, 1}, //  8
	{ 1, 2, 2, 6, 2, 2, 1}, // 16
	{ 1, 2, 4, 6, 4, 2, 1}, // 20
	{ 2, 6, 6,12, 6, 6, 2}, // 40
	{ 1, 2, 4, 6, 4, 2, 1}, // 20
	{ 1, 2, 2, 6, 2, 2, 1}, // 16
	{ 1, 1, 1, 2, 1, 1, 1}, //  8
},//----------------------------  128
	7 //size;
};

ConvolveMatrix avg_ma3x_5 = {
{
	{ 1, 1, 1, 1, 1 }, 
	{ 1, 1, 1, 1, 1 }, 
	{ 1, 1, 1, 1, 1 }, 
	{ 1, 1, 1, 1, 1 }, 
	{ 1, 1, 1, 1, 1 }
},
	7
};


ConvolveMatrix avg_ma3x_7 = {
{
	{ 1, 1, 1, 1, 1, 1, 1 }, 
	{ 1, 1, 1, 1, 1, 1, 1 }, 
	{ 1, 1, 1, 1, 1, 1, 1 }, 
	{ 1, 1, 1, 1, 1, 1, 1 }, 
	{ 1, 1, 1, 1, 1, 1, 1 }, 
	{ 1, 1, 1, 1, 1, 1, 1 }, 
	{ 1, 1, 1, 1, 1, 1, 1 } 
},
	7
};



void shader_cmdline_option(const char *opt)
{
	if (!strcmp(opt, "EdgeGlow")) {
		user_pp_state = SHADER_ID_EDGE_GLOW;
	}
	if (!strcmp(opt, "Sobel")) {
		user_pp_state = SHADER_ID_SOBEL;
	}
	if (!strcmp(opt, "FFTFilter")) {
		user_pp_state = SHADER_ID_FFT_FILTER;
	}
	if (!strcmp(opt, "Inversion")) {
		user_pp_state = SHADER_ID_INVERSION;
	}
	if (!strcmp(opt, "ObjectGlow")) {
		user_pp_state = SHADER_ID_OBJECT_GLOW;
	}
	if (!strcmp(opt, "Blur")) {
		user_pp_state = SHADER_ID_BLUR;
	}
}

// multithread framebuffer copy (M-th thread in N-threaded configuration copies
// only rows Y where Y mod N == M
void mt_fb_memcpy(Uint32 *dest, Uint32 *src, int resx, int resy, int thread_idx, int threads_count)
{
	if (threads_count == 1)
		memcpy(dest, src, resx * resy * 4);
	else {
		for (int j = thread_idx; j < resy; j += threads_count)
			for (int i = 0; i < resx; i++)
				dest[j * resx + i] = src[j * resx + i];
	}
}


/*
	The following code is taken directly from http://astronomy.swin.edu.au/~pbourke/analysis/fft2d/
	(I hope it's not copyrighted...)
*/


/*-------------------------------------------------------------------------
   This computes an in-place complex-to-complex FFT
   x and y are the real and imaginary arrays of 2^m points.
   dir =  1 gives forward transform
   dir = -1 gives reverse transform

     Formula: forward
                  N-1
                  ---
              1   \          - j k 2 pi n / N
      X(n) = ---   >   x(k) e                    = forward transform
              N   /                                n=0..N-1
                  ---
                  k=0

      Formula: reverse
                  N-1
                  ---
                  \          j k 2 pi n / N
      X(n) =       >   x(k) e                    = forward transform
                  /                                n=0..N-1
                  ---
                  k=0
*/
void fft_1D_complex(int dir,int m,float *x,float *y)
{
   long nn,i,i1,j,k,i2,l,l1,l2;
   float c1,c2,tx,ty,t1,t2,u1,u2,z, sc;

   /* Calculate the number of points */
   nn = 1;
   for (i=0;i<m;i++)
      nn *= 2;

   /* Do the bit reversal */
   i2 = nn >> 1;
   j = 0;
   for (i=0;i<nn-1;i++) {
      if (i < j) {
         tx = x[i];
         ty = y[i];
         x[i] = x[j];
         y[i] = y[j];
         x[j] = tx;
         y[j] = ty;
      }
      k = i2;
      while (k <= j) {
         j -= k;
         k >>= 1;
      }
      j += k;
   }

   /* Compute the FFT */
   c1 = -1.0;
   c2 = 0.0;
   l2 = 1;
   for (l=0;l<m;l++) {
      l1 = l2;
      l2 <<= 1;
      u1 = 1.0;
      u2 = 0.0;
      for (j=0;j<l1;j++) {
         for (i=j;i<nn;i+=l2) {
            i1 = i + l1;
            t1 = u1 * x[i1] - u2 * y[i1];
            t2 = u1 * y[i1] + u2 * x[i1];
            x[i1] = x[i] - t1;
            y[i1] = y[i] - t2;
            x[i] += t1;
            y[i] += t2;
         }
         z =  u1 * c1 - u2 * c2;
         u2 = u1 * c2 + u2 * c1;
         u1 = z;
      }
      c2 = sqrt((1.0 - c1) * 0.5);
      if (dir == 1)
         c2 = -c2;
      c1 = sqrt((1.0 + c1) * 0.5);
   }

   /* Scaling for forward transform */
   if (dir == 1) {
      sc = 1/(float)nn;
      for (i=0;i<nn;i++) {
         x[i] *= sc;
         y[i] *= sc;
      }
   }
}

/* some crucial constants needed for the SSE 1D FFT: */

#define shaders_asm
#include "x86_asm.h"
#undef shaders_asm

/*-------------------------------------------------------------------------
   Perform a 2D FFT inplace given a complex 2D array
   The direction dir, 1 for forward, -1 for reverse
   The size of the array (nx,ny)
   Return false if there are memory problems or
      the dimensions are not powers of 2
*/
void fft_2D_complex(complex c[MAX_FFT_SIZE][MAX_FFT_SIZE], int dir, int fft_size)
{
   int i, j, log_fft_size, usse, loopsize;
   SSE_ALIGN(float re[4*MAX_FFT_SIZE]);
   SSE_ALIGN(float im[4*MAX_FFT_SIZE]);


   log_fft_size = power_of_2(fft_size);
   if (log_fft_size==-1) return;

   usse = (cpu.sse && fft_size>=4);
   //usse=0;
   loopsize = fft_size / (usse?4:1);

   for (j=0;j<loopsize;j++) {
      if (usse) float_copy_ij_i(re, im, c[0]+j*4, fft_size);
      	else
      		for (i=0;i<fft_size;i++) {
         		re[i] = c[i][j].re;
         		im[i] = c[i][j].im;
      		}
	if (usse)
		fft_1D_complex_sse(dir, log_fft_size, re, im);
	else
      		fft_1D_complex    (dir, log_fft_size, re, im);
      if (usse) float_copy_i_ij(re, im, c[0]+j*4, fft_size);
      	else
      		for (i=0;i<fft_size;i++) {
         		c[i][j].re = re[i];
         		c[i][j].im = im[i];
      		}
   }

   for (i=0;i<loopsize;i++) {
     if (usse) float_copy_ij_j(re, im, c[i*4], fft_size);
      else
      for (j=0;j<fft_size;j++) {
         re[j] = c[i][j].re;
         im[j] = c[i][j].im;
      }
     if (usse)
      	fft_1D_complex_sse(dir, log_fft_size, re, im);
	else
      	fft_1D_complex(dir,log_fft_size,re,im);
     if (usse) float_copy_j_ij(re, im, c[i*4], fft_size);
       else
      for (j=0;j<fft_size;j++) {
         c[i][j].re = re[j];
         c[i][j].im = im[j];
      }
   }
}

#define sqr(x) ((x)*(x))

void create_fft_filter(int fft_size)
{
	int i, j;
	float *f, fsd2mh, rfsd2mhpsqrt2;

	fft_filter = (float *) malloc(fft_size * fft_size * sizeof(float));
	if (!fft_filter) return;
	f = fft_filter;
	fsd2mh = fft_size/2 - 0.5;
	rfsd2mhpsqrt2 = 1 / (fsd2mh * 1.4142135623731);
	for (j=0;j<fft_size;j++)
		for (i=0;i<fft_size;i++, f++)
			*f = 4*(1.0-exp(log(sqrt(sqr(fsd2mh - j) + sqr(fsd2mh - i))*rfsd2mhpsqrt2)*6));
}

void apply_fft_filter_x86(complex *dest, float *filter, int fft_size)
{
	if (!filter) return;
	fft_size = fft_size * fft_size;
	do {
		dest->re *= (*filter);
		dest->im *= (*filter);
		dest++; filter++;
	}while (--fft_size);
}

#define apply_fft_filter_asm
#include "x86_asm.h"
#undef  apply_fft_filter_asm

static inline float clamp_float(float f)
{
	if (f>255.0) f = 255.0;
	if (f < 0.0) f =   0.0;
	return f;
}

static inline int clamp_int(int f)
{
	if (f > 255) f = 255;
	if (f <   0) f = 0;
	return f;
}

#define fix(x) (clamp_int((x)))

struct FloatArr {
	complex in[3][MAX_FFT_SIZE][MAX_FFT_SIZE];
};

static Allocator<FloatArr> allocator(ALLOCATOR_MALLOC_FREE);

// returns 1 on succes, zero otherwise
int shader_FFT_Filter(Uint32 *src, Uint32 *dst, int resx, int resy)
{
	int i, j, x, y, fft_size;
	volatile int k;
	prof_enter(PROF_SHADER1);
	if (resx>1024 || resy>1024) return 0;
	x = resx>resy?resx:resy;
	y = 0;
	if (power_of_2(x)==-1) { // if the largest of the two dimensions is not a power of 2, extend to the closest larger power of 2
		y++;
		// get the next power of 2
		while (x!=1) { x/=2; y++; }
		}
	fft_size = x<<y;
	FloatArr *fa = allocator[0];

	if (fft_size == MAX_FFT_SIZE) memset(fa->in, 0, sizeof(fa->in));
		else{
			for (j=0;j<3;j++) {
				for (i=0;i<fft_size;i++) if (i>=resy)
					memset(fa->in[j][i], 0, sizeof(complex)*fft_size);
					else
					memset(fa->in[j][i]+resx, 0, sizeof(complex)*(fft_size-resx));
				}
		}

	for (j=0;j<resy;j++) {
		for (i=0;i<resx;i++) {
			for (k = 0; k < 3; k++) {
				fa->in[k][j][i].re = (float) ((src[i + j*resx]>>(8*k)) & 0xff);
				fa->in[k][j][i].im = 0.0;
			}
		}
	}

	memset(dst, 0, resx*resy*4);

	if (!fft_filter) create_fft_filter(fft_size);
		else {free(fft_filter); create_fft_filter(fft_size);}

	for (k = 0; k < 3; ++k) {
		//k+=2;k%=3;
		//printf("%d\n", k);
		fft_2D_complex(fa->in[k], 1, fft_size);
		if (cpu.sse) apply_fft_filter_sse(fa->in[k][0], fft_filter, fft_size);
			else	 apply_fft_filter_x86(fa->in[k][0], fft_filter, fft_size);
		fft_2D_complex(fa->in[k], -1, fft_size);
		//k++;
		//k%=3;
		}
	prof_leave(PROF_SHADER1);
	prof_enter(PROF_SHADER2);
	for (k = 0; k < 3; k++)
		for (j = 0; j < resy; j++)
			for (i = 0; i < resx; i++) {
				//do_add(dst+i+j*resx,fix(fa->in[0][j][i].re),fix(fa->in[1][j][i].re),fix(fa->in[2][j][i].re));
				dst[i+j*resx] += fix((int)fa->in[k][j][i].re)<<(8*k);
			}
	prof_leave(PROF_SHADER2);
	return 1;
}

static inline void clamp3(int *a, int *b, int *c)
{
	*a = (*a)<0?0:(*a)>255?255:(*a);
	*b = (*b)<0?0:(*b)>255?255:(*b);
	*c = (*c)<0?0:(*c)>255?255:(*c);
}

void shader_convolution(Uint32 *src, Uint32 *dest, int resx, int resy, ConvolveMatrix *M)
{
	int a, r, g, b, i, j, x, y;
	int si, ei, sj, ej, s, log2s;

	// cope with edges
	for (y=0;y<M->n/2;y++) {
		sj = M->n/2 - y;
		for (x=0;x<resx;x++) {
			si = M->n/2-x; if (si<0) si = 0;
			ei = resx - x + M->n/2; if (ei > M->n) ei = M->n;
			s = r = g = b = 0;
			for (j=sj; j<M->n;j++)
				for (i=si;i<ei;i++) {
					a = src[resx*(y+j-M->n/2) + x+i-M->n/2];
					b += M->coeff[j][i]*(a&0xff); a>>=8;
					g += M->coeff[j][i]*(a&0xff); a>>=8;
					r += M->coeff[j][i]*(a&0xff);
					s += M->coeff[j][i];
				}
			if (((ei-si) % 2) ^ ((M->n-sj) % 2)) {
				a = src[resx*y + x];
				b += a&0xff; a>>=8;
				g += a&0xff; a>>=8;
				r += a&0xff;
			}
			if (s) {b/=s;g/=s;r/=s;}
			clamp3(&b, &g, &r);
			dest[x + y*resx] = b | (g<<8) | (r<<16);
		}
	}

	for (y=resy-M->n/2;y<resy;y++) {
		ej = resy - y + M->n/2;
		for (x=0;x<resx;x++) {
			si = M->n/2-x; if (si<0) si = 0;
			ei = resx - x + M->n/2; if (ei > M->n) ei = M->n;
			s = r = g = b = 0;
			for (j=0; j<ej;j++)
				for (i=si;i<ei;i++) {
					a = src[resx*(y+j-M->n/2) + x+i-M->n/2];
					b += M->coeff[j][i]*(a&0xff); a>>=8;
					g += M->coeff[j][i]*(a&0xff); a>>=8;
					r += M->coeff[j][i]*(a&0xff);
					s += M->coeff[j][i];
				}
			if (((ei-si) % 2) ^ (ej % 2)) {
				a = src[resx*y + x];
				b += a&0xff; a>>=8;
				g += a&0xff; a>>=8;
				r += a&0xff;
			}
			if (s) {b/=s;g/=s;r/=s;}
			clamp3(&b, &g, &r);
			dest[x + y*resx] = b | (g<<8) | (r<<16);
		}
	}

	for (x=0; x < M->n/2 ; x++) {
		si = M->n/2 - x;
		for (y=M->n/2;y<resy-M->n/2;y++) {
			sj = M->n/2-y; if (sj<0) sj = 0;
			ej = resy - y + M->n/2; if (ej > M->n) ej = M->n;
			s = r = g = b = 0;
			for (j=sj; j<ej;j++)
				for (i=si;i<M->n;i++) {
					a = src[resx*(y+j-M->n/2) + x+i-M->n/2];
					b += M->coeff[j][i]*(a&0xff); a>>=8;
					g += M->coeff[j][i]*(a&0xff); a>>=8;
					r += M->coeff[j][i]*(a&0xff);
					s += M->coeff[j][i];
				}
			if (((ej-sj) % 2) ^ ((M->n-si) % 2)) {
				a = src[resx*y + x];
				b += a&0xff; a>>=8;
				g += a&0xff; a>>=8;
				r += a&0xff;
			}
			if (s) {b/=s;g/=s;r/=s;}
			clamp3(&b, &g, &r);
			dest[x + y*resx] = b | (g<<8) | (r<<16);
		}
	}
	for (x=resx - M->n/2; x < resx ; x++) {
		ei = resx + M->n/2 - x;
		for (y=M->n/2;y<resy-M->n/2;y++) {
			sj = M->n/2-y; if (sj<0) sj = 0;
			ej = resy - y + M->n/2; if (ej > M->n) ej = M->n;
			s = r = g = b = 0;
			for (j=sj; j<ej;j++)
				for (i=0;i<ei;i++) {
					a = src[resx*(y+j-M->n/2) + x+i-M->n/2];
					b += M->coeff[j][i]*(a&0xff); a>>=8;
					g += M->coeff[j][i]*(a&0xff); a>>=8;
					r += M->coeff[j][i]*(a&0xff);
					s += M->coeff[j][i];
				}
			if (((ej-sj) % 2) ^ (ei % 2)) {
				a = src[resx*y + x];
				b += a&0xff; a>>=8;
				g += a&0xff; a>>=8;
				r += a&0xff;
			}
			if (s) {b/=s;g/=s;r/=s;}
			clamp3(&b, &g, &r);
			dest[x + y*resx] = b | (g<<8) | (r<<16);
		}
	}
/* do the whole thing: */
	s = 0;
	for (i=0;i<M->n;i++)
		for (j=0;j<M->n;j++)
			s+=M->coeff[i][j];
			/*	The idea here is simple:
				if s is zero, don't correct RGB values. If it is a power of two, use shifts instead of divides
			*/
	//printf("%d %d\n", s, power_of_2(s));
	if (cpu.mmx && (s==0||power_of_2(s)>=0)) {
		convolve_mmx_w_shifts(src, dest, resx, resy, M, power_of_2(s)); // see effectsasm for details
		return;
	}
	if ( s==0 ) {
	for (y=M->n/2;y<resy-M->n/2;y++)
		for (x=M->n/2;x<resx-M->n/2;x++) {
			r=g=b=0;
			for (j=0;j<M->n;j++)
				for (i=0;i<M->n;i++) {
					a = src[resx*(y+j-M->n/2) + x+i-M->n/2];
					b += M->coeff[j][i]*((int)(a&0xff)); a>>=8;
					g += M->coeff[j][i]*((int)(a&0xff)); a>>=8;
					r += M->coeff[j][i]*((int)(a&0xff));
				}
			clamp3(&b, &g, &r);
			dest[x + y*resx] = b | (g<<8) | (r<<16);
		}
	} else {if (power_of_2(s)>=0) {
		log2s = power_of_2(s);
	for (y=M->n/2;y<resy-M->n/2;y++)
		for (x=M->n/2;x<resx-M->n/2;x++) {
			r=g=b=0;
			for (j=0;j<M->n;j++)
				for (i=0;i<M->n;i++) {
					a = src[resx*(y+j-M->n/2) + x+i-M->n/2];
					b += M->coeff[j][i]*((int)(a&0xff)); a>>=8;
					g += M->coeff[j][i]*((int)(a&0xff)); a>>=8;
					r += M->coeff[j][i]*((int)(a&0xff));
				}
			if (b<0) b = 0; if (g<0) g = 0; if (r<0) r = 0;
			b>>=log2s; g>>=log2s; r>>=log2s;
			if (b>255) b = 255; if (g>255) g = 255; if (r>255) r = 255;
			dest[x + y*resx] = b | (g<<8) | (r<<16);
		}

	} else {
	//if (cpu.mmx) { convolve_mmx_w_div(src, dest, resx, resy, M, s); return; }
	for (y=M->n/2;y<resy-M->n/2;y++)
		for (x=M->n/2;x<resx-M->n/2;x++) {
			r=g=b=0;
			for (j=0;j<M->n;j++)
				for (i=0;i<M->n;i++) {
					a = src[resx*(y+j-M->n/2) + x+i-M->n/2];
					b += M->coeff[j][i]*((int)(a&0xff)); a>>=8;
					g += M->coeff[j][i]*((int)(a&0xff)); a>>=8;
					r += M->coeff[j][i]*((int)(a&0xff));
				}
			b/=s;g/=s;r/=s;
			clamp3(&b, &g, &r);
			dest[x + y*resx] = b | (g<<8) | (r<<16);
		}
	}
	}
}


void shader_edge_glow(Uint32 *src, Uint32 *dest, int resx, int resy)
{
	prof_enter(PROF_SHADER1);
	shader_convolution(src, shader_tmp , resx, resy, &pshop_ma3x_5);
	prof_leave(PROF_SHADER1);
	if (pshop_ma3x_5.shiftrequired)
		shader_gamma_shl(shader_tmp, shader_tmp, resx, resy, pshop_ma3x_5.shiftrequired);
	prof_enter(PROF_SHADER2);
	shader_convolution(shader_tmp, dest, resx, resy, &blur_ma3x_3);
	//shader_gamma_shl(dest, dest, resx, resy, 2);
	//memcpy(dest, shader_tmp, resx*resy*4);
	prof_leave(PROF_SHADER2);
}

void shader_blur(Uint32 *src, Uint32 *dest, int resx, int resy, int blur_size)
{
	Uint32 *tgt;
	prof_enter(PROF_SHADER1);
	if (src!=dest) tgt=dest;
		else tgt = shader_tmp;
	switch (blur_size){
		case 3: shader_convolution(src, tgt, resx, resy, &blur_ma3x_3); break;
		case 7: shader_convolution(src, tgt, resx, resy, &blur_ma3x_7); break;
		default:shader_convolution(src, tgt, resx, resy, &blur_ma3x_5);
	}
	prof_leave(PROF_SHADER1);
	prof_enter(PROF_SHADER2);
	if (src==dest) memcpy(dest, shader_tmp, resx*resy*4);
	prof_leave(PROF_SHADER2);
}

void shader_inversion(Uint32 *src, Uint32 *dest, int resx, int resy, double radius)
{
	prof_enter(PROF_SHADER1);
	double cx, cy, xp, yp, jsq, f, f1;
	Uint32 *tgt;
	int i, j, xpi, ypi;
	cx = 0.5 * (resx-1); cy = 0.5 * (resy-1);

	tgt = ((src == dest)?shader_tmp:dest);
	radius *= radius;
	for (j=0;j<resy;j++) {
		jsq = sqr(j-cy);
		for (i=0;i<resx;i++) {
			xp = radius*(i-cx) / (double) (jsq+sqr(i-cx));
			yp = radius*(j-cy) / (double) (jsq+sqr(i-cx));
			if (fabs(xp)>cx || fabs(yp)>cy) {
				f = cx/fabs(xp);
				f1= cy/fabs(yp);
				if (f1 < f) f = f1;
				xp *= f; yp *= f;
			}
			xpi = roundint(xp+cx) ; ypi=roundint(yp+cy);

			*(tgt++) = ((xpi>=0 && xpi < resx && ypi >= 0 && ypi < resy)?(src[xpi + ypi * resx]):0);
		}
	}
	prof_leave(PROF_SHADER1);
	prof_enter(PROF_SHADER2);
	if (src == dest) memcpy(dest, shader_tmp, resx*resy*4);
	prof_leave(PROF_SHADER2);
}

void shader_spill(Uint8 *src, Uint8 * dest, int resx, int resy, float coeff)
{
	coeff = 1 - coeff;
	if (cpu.mmx) {
		shader_spill_mmx(src, dest, resx, resy, coeff);
		return;
	}
	unsigned I = (unsigned) (65535 * coeff);
	unsigned Im1 = 65535 - I;
	for (int row = 0; row < resy; row++) {
		Uint8 *sr = src  + row * resx;
		Uint8 *dr = dest + row * resx;
		for (int x = 1; x < resx; x++) {
			dr[x] = (Uint8) ((sr[x] * I + dr[x-1] * Im1)>>16);
		}
		for (int x = resx - 2; x >= 0; x--) {
			dr[x] = ((dr[x] * I + dr[x+1] * Im1)>>16);
		}
	}
	for (int col = 0; col < resx; col++) {
		Uint8 * p = dest + col;
		for (int y = 1; y < resy; y ++ ){
			p[y * resx] = ((p[y * resx] * I + p[(y-1) * resx] * Im1) >> 16);
		}
		for (int y = resy - 2; y >= 0; y--) {
			p[y * resx] = ((p[y * resx] * I + p[(y+1) * resx] * Im1) >> 16);
		}
	}
}


void shader_fbmerge(Uint32 *dest, Uint8 * src, int resx, int resy, float intensity, Uint32 glow_color)
{
	if (cpu.mmx2) {
		shader_fbmerge_mmx2(dest, src, resx, resy, intensity, glow_color);
		return;
	}
	for (int i = 0; i < resx * resy; i++) {
		int mult1 = (int) ((src[i] << 8)*intensity);
		dest[i] = multiplycolor(dest[i], 65535 - mult1) + multiplycolor(glow_color, mult1);
	}
}

void shader_prepare_for_glow(const char *which, const Uint32 *prefb, Uint8 * glowbuff, int resx, int resy)
{
	static bool included[MAX_OBJECTS];
	memset(included, 0, allobjects.size() * sizeof(bool));
	unsigned match_id;
	OBTYPE om = OB_UNDEFINED;
	if (0 == strcmp(which, "all"))
		om = OB_ALL;
	else {
		
		if (1 == sscanf(which, "sphere:%u", &match_id)) {
			om = OB_SPHERE;
		}
		if (1 == sscanf(which, "mesh:%u", &match_id)) {
			om = OB_TRIANGLE;
		}
		if (om == OB_UNDEFINED)
			return;
	}
	match_id += om;
	for (int i = 0; i < allobjects.size(); i++) {
		OBTYPE type = allobjects[i]->get_type();
		if (om == OB_ALL || (om == type && allobjects[i]->tag == match_id)) {
			included[i] = true;
		}
	}
	int sz = resx * resy;
	for (int i = 0; i < sz; i++) {
		int t = prefb[i] & 0xffff;
		if (t && included[t-1])
			glowbuff[i] = 0xff;
		else
			glowbuff[i] = 0;
	}
}

void shader_object_glow(Uint32 *fb, Uint8 * glowbuff, Uint32 glow_color, int resx, int resy, float effect_intensity)
{
	prof_enter(PROF_SHADER1);
	shader_spill(glowbuff, glowbuff, resx, resy, 0.80);
	prof_leave(PROF_SHADER1);
	prof_enter(PROF_SHADER2);
	shader_fbmerge(fb, glowbuff, resx, resy, effect_intensity, glow_color);
	prof_leave(PROF_SHADER2);
}

static int sqrt_tab[65536];

void shader_sobel(Uint32 *src, Uint32 *dest, int resx, int resy, int thread_idx, int threads_count)
{
	const int h_kernel[9] = 
	{
		+1, +2, +1,
		 0,  0,  0,
		-1, -2, -1,
	};
	const int v_kernel[9] = 
	{
		+1, +0, -1,
		+2, +0, -2,
		+1, +0, -1,
	};
	Uint32 *mydest;
	if (src == dest) {
		mydest = shader_tmp;
	} else {
		mydest = dest;
	}
	if (cpu.sse) {
		shader_sobel_sse(src, mydest, resx, resy, h_kernel, v_kernel, thread_idx, threads_count);
		if (src == dest) 
			mt_fb_memcpy(dest, shader_tmp, resx, resy, thread_idx, threads_count);
		return;
	}

	if (sqrt_tab[65535] == 0) {
		for (int i = 0; i < 65536; i++)
			sqrt_tab[i] = (int) sqrt((float)i);
	}
	prof_enter(PROF_SHADER1);
	int p = 0;
	for (int j = thread_idx; j < resy; j += threads_count) {
		for (int i = 0; i < resx; i++, p++) {
			mydest[p] = 0;
			if (i == 0 || i == resx - 1 || j == 0 || j == resy - 1) {
				 continue;
			}
			for (int k = 0; k < 24; k += 8) {
				int h_value = 0, v_value = 0;
				for (int yi = 0; yi < 3; yi += 2) {
					for (int xi = 0; xi < 3; xi++) {
						int t = 0xff&(src[p+xi-1+(yi-1)*resx]>>k);
						h_value += t * h_kernel[yi*3+xi];
					}
				}
				for (int yi = 0; yi < 3; yi ++) {
					for (int xi = 0; xi < 3; xi+=2) {
						int t = 0xff&(src[p+xi-1+(yi-1)*resx]>>k);
						v_value += t * v_kernel[yi*3+xi];
					}
				}
				int orle = (h_value*h_value + v_value*v_value);
				if (orle > 65535) orle = 255;
				else orle = sqrt_tab[orle];
				mydest[p] |= orle << k;
			}
		}
	}
	prof_leave(PROF_SHADER1);
	prof_enter(PROF_SHADER2);
	if (src == dest) 
		mt_fb_memcpy(dest, shader_tmp, resx, resy, thread_idx, threads_count);
	prof_leave(PROF_SHADER2);
}


void shader_shutters(Uint32 *fb, Uint32 col, int resx, int resy, float amount)
{
	int p = 0;
	const int effsize = 32;
	int f = (int) ((2*effsize) * amount);
	for (int j = 0; j < resy; j++) {
		for (int i = 0; i < resx; i++,p++) {
			if ((i&(effsize-1))+(j&(effsize-1)) < f)
				fb[p] = (fb[p]>>2) & 0x3f3f3f;
		}
	}
}

static inline unsigned limit255(unsigned x) { return (x >= 256) ? 255 : x; }

void shader_clippedgamma(Uint32 *fb, int resx, int resy, double multiplier)
{
	unsigned m = (unsigned)(multiplier * 65535u);

	for (int j = 0; j < resy; j++) {
		Uint32* fbl = &fb[j*resx];
		for (int i = 0; i < resx; i++)
			fbl[i] = 
			/* B */ (limit255(((fbl[i] & 0xff) * m) >> 16))              + 
			/* G */ (limit255((((fbl[i] >> 8) & 0xff) * m) >> 16) << 8 ) +
			/* R */ (limit255((((fbl[i] >>16) & 0xff) * m) >> 16) << 16) ;
	}
}

void shader_gamma(Uint32 *fb, int resx, int resy, float multiplier, int thread_idx, int threads_count)
{
	unsigned m = (unsigned)(multiplier * 255u);

	for (int j = thread_idx; j < resy; j += threads_count) {
		Uint32* fbl = &fb[j*resx];
		for (int i = 0; i < resx; i++)
			fbl[i] = (((fbl[i] & 0xff)*m)>>8) + 
				((((fbl[i] & 0xff00)*m)>>8) & 0xff00) + 
				((((fbl[i] & 0xff0000)*m)>>8) & 0xff0000);
	}
}

void shader_gamma_cpy(Uint32 *src, Uint32 *dest, int resx, int resy, float multiplier, int thread_idx, int threads_count)
{
	unsigned m = (unsigned)(multiplier * 255u);

	for (int j = thread_idx; j < resy; j += threads_count) {
		Uint32* srcl = &src[j*resx];
		Uint32* dstl = &dest[j*resx];
		for (int i = 0; i < resx; i++)
			dstl[i] =       (((srcl[i] & 0xff)*m)>>8) + 
					((((srcl[i] & 0xff00)*m)>>8) & 0xff00) + 
					((((srcl[i] & 0xff0000)*m)>>8) & 0xff0000);
	}
}


void shader_outro_effect(Uint32 *fb)
{
#ifdef ACTUALLYDISPLAY	
	if (get_ticks() % 3 == 0) {
		shader_sobel(fb, fb, xres(), yres());
		shader_gamma_shr(fb, fb, xres(), yres(), 1);
		return;
	}
	int i, j, q=0;

	for (j = 0;j < yres(); j++)
		for (i=0; i< xres(); i++, q++)
			fb[q] = desaturate(multiplycolor(fb[q], 50000 + rand()%20000), 0.8);
#endif	
}



void shaders_close(void)
{
	if (fft_filter) free(fft_filter);
}
