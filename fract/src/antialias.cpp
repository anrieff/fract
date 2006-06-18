/***************************************************************************
 *   Copyright (C) 2004 by Veselin Georgiev                                *
 *   vesko@ViruS                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *-------------------------------------------------------------------------*
 *  Copes with antialiasing modes, does transforms using MMX if possible   *
 ***************************************************************************/
#include <map>
using namespace std;

#include <stdio.h>
#include <string.h>
#include "MyGlobal.h"
#include "MyTypes.h"
#include "antialias.h"
#include "cmdline.h"
#include "gfx.h"
#include "fract.h"
#include "render.h"

int fsaa_mode;
char fsaa_name[64] = "none";
double last_fsaa_change=-90;
extern int mmx_enabled;
extern int sse_enabled;
extern int defaultconfig;
extern int RowMin[], RowMax[];

Uint8 fb_copy[RES_MAXX * RES_MAXY];

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** **/
/*  Static FSAA Data - the FSAA Kernels used for adaptive AA in Fract:  **/
/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** **/

// simple unweighted 4x unaligned grid pattern
static const fsaa_kernel aaa_kernel_4 = {
        {
                { 0.125, 0.125 },
                { 0.625, 0.250 },
                { 0.000, 0.625 },
                { 0.500, 0.750 },
        },
        {
                0.2500, 0.2500, 0.2500, 0.2500,
        },
        4
};

// High-quality 16x kernel, based on the above "simple 4x" grid
// with some transforms and scalings and with added weights

// Courtesy of FELLIX
static const fsaa_kernel aaa_kernel_16 = {
        {
                { 0.000, 0.625 },
                { 0.125, 0.125 },
                { 0.375, 0.000 },
                { 0.875, 0.125 },
                { 1.000, 0.375 },
                { 0.875, 0.875 },
                { 0.625, 1.000 },
                { 0.125, 0.875 },
                { 0.625, 0.250 },
                { 0.500, 0.750 },
                { 0.250, 0.375 },
                { 0.750, 0.500 },
                { 0.375, 0.750 },
                { 0.500, 0.250 },
                { 0.250, 0.500 },
                { 0.750, 0.625 },
        },
        {
                0.07575758, 0.07575758, 0.07575758, 0.07575758,
                0.07575758, 0.07575758, 0.07575758, 0.07575758,
                0.04924242, 0.04924242, 0.04924242, 0.04924242,
                0.04924242, 0.04924242, 0.04924242, 0.04924242,
        },
        16
};

// simple weighted Quincunx kernel, looks pretty good
static const fsaa_kernel aaa_kernel_quincunx = {
        {
                { 0.375, 0.375 },
                { 0.125, 0.125 },
                { 0.125, 0.625 },
                { 0.625, 0.625 },
                { 0.625, 0.125 },
        },
        {
                0.2500, 0.1875, 0.1875, 0.1875,
                0.1875,
        },
        5
};

// Another kernel by fellix; unweighted, 10-sample
// Looks beautiful
static const fsaa_kernel aaa_kernel_10 = {
        {
                { 0.250, 0.125 },
                { 0.125, 0.500 },
                { 0.500, 0.625 },
                { 0.625, 0.250 },
                { 0.375, 0.375 },
                { 0.750, 0.875 },
                { 0.375, 0.750 },
                { 0.500, 0.375 },
                { 0.875, 0.500 },
                { 0.625, 0.625 },
        },
        {
                0.1000, 0.1000, 0.1000, 0.1000,
                0.1000, 0.1000, 0.1000, 0.1000,
                0.1000, 0.1000,
        },
        10
};

// called if the selected resolution is too high for antialiasing
// for example, the upscaled renderable is too large to fit in the framebuffers
void aa_mode_fallback(void)
{
	fsaa_mode = FSAA_MODE_ADAPT_4;
	printf("set_fsaa_mode: Can't set requested mode : insufficient framebuffer size for operation!\n");
}

// tries to switch to the given antialiasing mode if possible
void set_fsaa_mode(int newMode)
{
	last_fsaa_change = bTime();
	switch (newMode) {
		case FSAA_MODE_4X_LO_Q:
			if (xres()*2<=RES_MAXX && yres()*2<=RES_MAXX) {
				fsaa_mode = FSAA_MODE_4X_LO_Q;
				strcpy(fsaa_name, "BfAA_4x_lo");
			} else {
				aa_mode_fallback();
			}
			break;
		case FSAA_MODE_4X_HI_Q:
			if (xres()*2<=RES_MAXX && yres()*2<=RES_MAXX) {
				fsaa_mode = FSAA_MODE_4X_HI_Q;
				strcpy(fsaa_name, "BfAA_4x_hi");
			} else {
				aa_mode_fallback();
			}
			break;
		case FSAA_MODE_ADAPT_4:
			fsaa_mode = FSAA_MODE_ADAPT_4;
			strcpy(fsaa_name, "AAA_4x");
			break;
		case FSAA_MODE_ADAPT_QUINCUNX:
			fsaa_mode = FSAA_MODE_ADAPT_QUINCUNX;
			strcpy(fsaa_name, "AAA_Quincunx");
			break;
		case FSAA_MODE_ADAPT_10:
			fsaa_mode = FSAA_MODE_ADAPT_10;
			strcpy(fsaa_name, "AAA_fellix_10");
			break;
		case FSAA_MODE_ADAPT_16:
			fsaa_mode = FSAA_MODE_ADAPT_16;
			strcpy(fsaa_name, "AAA_fellix_16");
			break;
		default:
			fsaa_mode = FSAA_MODE_NONE;
			strcpy(fsaa_name, "none");
	}

	gfx_update_2nds();
}

#define antialias_asm
#include "x86_asm.h"
#undef antialias_asm

// the integer C function is a low fidelity one; just like antialias_4x_mmx2_lo_fi

static void antialias_4x_p5(Uint32 *fb)
{
	int i, j, xr, yr, z;
	xr = xres();
	yr = yres();
	for (j=0,z=0;j<yr;j++)
		for (i=0;i<xr;i++)
			fb[z++] =
				((fb[(j*2  )*(xr*2)+i*2  ] >> 2) & 0x3f3f3f3f) +
				((fb[(j*2  )*(xr*2)+i*2+1] >> 2) & 0x3f3f3f3f) +
				((fb[(j*2+1)*(xr*2)+i*2  ] >> 2) & 0x3f3f3f3f) +
				((fb[(j*2+1)*(xr*2)+i*2+1] >> 2) & 0x3f3f3f3f) ;
}

static void antialias_4x(Uint32 *fb)
{
	if ( sse_enabled ) {
		if (fsaa_mode == FSAA_MODE_4X_LO_Q)
			antialias_4x_mmx2_lo_fi(fb);
			else
			antialias_4x_mmx_hi_fi(fb);
	}
		else
		antialias_4x_p5(fb);
}

static void antialias_uint8(Uint8 *fb)
{
	int xr = xres(), yr = yres();
	int z = 0, i, j;
	for (i = 0; i < yr; i++)
		for (j = 0; j < xr; j++)
			fb[z++] = fb[i * xr * 4 + j * 2];
}

/* Master API function, selects an antialiasing function, depending on the FSAA mode */
void antialias(Uint32 *fb)
{
	switch (fsaa_mode) {
		case FSAA_MODE_4X_LO_Q:
		case FSAA_MODE_4X_HI_Q: antialias_4x(fb); break;
	}
}

void antialias(Uint8 *fb)
{
	switch (fsaa_mode) {
		case FSAA_MODE_4X_LO_Q:
		case FSAA_MODE_4X_HI_Q: antialias_uint8(fb); break;
	}
}


int  xsize_render(int x)
{
	switch (fsaa_mode) {
		case FSAA_MODE_4X_LO_Q:
		case FSAA_MODE_4X_HI_Q:
			return (2*x); break;
		default: return x;
	}
}

int  ysize_render(int y)
{
	switch (fsaa_mode) {
		case FSAA_MODE_4X_LO_Q:
		case FSAA_MODE_4X_HI_Q: return (2*y); break;
		default: return y;
	}
}

/*
	Check command line for --fsaa option.
	return 0 if unknown fsaa option is given
*/
void check_fsaa_param(void)
{
	if (!option_exists("--fsaa")) return;
	if (!strcmp(option_value_string("--fsaa"),"none")) {
		set_fsaa_mode(FSAA_MODE_NONE);
		defaultconfig = 0;
		return;
	}
	if (!strcmp(option_value_string("--fsaa"),"4xlo-fi")) {
		set_fsaa_mode(FSAA_MODE_4X_LO_Q);
		defaultconfig = 0;
		return;
	}
	if (!strcmp(option_value_string("--fsaa"),"4xhi-fi")) {
		set_fsaa_mode(FSAA_MODE_4X_HI_Q);
		defaultconfig = 0;
		return;
	}
	if (!strcmp(option_value_string("--fsaa"),"4xAAA")) {
		set_fsaa_mode(FSAA_MODE_ADAPT_4);
		defaultconfig = 0;
		return;
	}
	if (!strcmp(option_value_string("--fsaa"),"5xAAA")) {
		set_fsaa_mode(FSAA_MODE_ADAPT_QUINCUNX);
		// this is the default	
		return;
	}
	if (!strcmp(option_value_string("--fsaa"),"10xAAA")) {
		set_fsaa_mode(FSAA_MODE_ADAPT_10);
		defaultconfig = 0;
		return;
	}
	if (!strcmp(option_value_string("--fsaa"),"16xAAA")) {
		set_fsaa_mode(FSAA_MODE_ADAPT_16);
		defaultconfig = 0;
		return;
	}
	printf("antialias.cpp: unknown fullscreen antialiasing mode.\n");
	printf("Valid choices are:\n\n");
	printf("\t`none' (default),\n\t`4xlo-fi',\n\t`4xhi-fi',\n\t`4xAAA',\n\t`5xAAA',\n\t`10xAAA',\n\t`16xAAA'\n");
}

bool is_adaptive_fsaa(void)
{
	return (fsaa_mode >= FSAA_MODE_ADAPT_4 && fsaa_mode <= FSAA_MODE_ADAPT_16);
}

void prepare_fsaa_context(raycasting_ctx * ctx, const Vector & column_inc, const Vector & row_inc)
{
	const fsaa_kernel *kern = NULL;
	switch (fsaa_mode) {
		case FSAA_MODE_ADAPT_4: kern = &aaa_kernel_4; break;
		case FSAA_MODE_ADAPT_QUINCUNX: kern = &aaa_kernel_quincunx; break;
		case FSAA_MODE_ADAPT_10: kern = &aaa_kernel_10; break;
		case FSAA_MODE_ADAPT_16: kern = &aaa_kernel_16; break;
	}
	if (!kern) {
		ctx->count = 0;
		return ;
	}
	ctx->count = kern->count;
	for (int i = 0; i < kern->count; i++) {
		ctx->weights[i] = kern->weights[i];
		ctx->sample[i] = column_inc * kern->coords[i][0] + row_inc * kern->coords[i][1];
	}
}

unsigned maxsize = 0;
fsaa_info_entry *fsaa_info;

static void prepare_fsaa_info_entry(fsaa_set_entry sentry, fsaa_info_entry& ientry)
{
	ientry.size = 0;
	for (int j = 0; j < 2; j++) {
		for (int i = 0; i < ED_KERNEL_SIZE; i++) {
			if (sentry.a[i] & 0xffff) {
				bool found = false;
				Uint32 newentry = sentry.a[i] & 0xffff;
				for (int j = 0; j < ientry.size; j++) {
					if (ientry.ids[j] == newentry) {
						found = true;
						break;
					}
				}
				if (!found) {
					ientry.ids[ientry.size++] = newentry;
				}
			}
			sentry.a[i] >>= 16;
		}
	}
	ientry.sort();
}

bool fsaa_set_entry::jagged() const
{
	bool jagflag = false;
	for (int i = 1; i < ED_KERNEL_SIZE; i++) {
		if (a[i] != a[0]) {
			jagflag = true;
			break;
		}
	}
	if (!jagflag) return false;
	for (int i = 0; i < ED_KERNEL_SIZE; i++) {
		int id = 0xffff & a[i];
		if (id && allobjects[id-1]->get_type() == OB_SPHERE)
			return false;
	}

	return true;
}

void fsaa_set_entry::sort()
{
	// sort them, since the order doesn't matter
	for (int i = 0; i < ED_KERNEL_SIZE - 1; i++) {
		for (int j = i + 1; j < ED_KERNEL_SIZE; j++) {
			if (a[i] > a[j]) {
				Uint32 t = a[i];
				a[i] = a[j];
				a[j] = t;
			}
		}
	}
}

void fsaa_info_entry::sort(void)
{
	for (int i = 0; i < size; i++) {
		for (int j = i + 1; j < size; j++) {
			if (ids[i] < ids[j]) {
				int t = ids[i];
				ids[i] = ids[j];
				ids[j] = t;
			}
		}
	}
}

void antibuffer_init(Uint32 fb[], int xr, int yr)
{
	HashMap<fsaa_set_entry, unsigned> m;
	unsigned c = 0;
	unsigned p = xr + 1;
	Uint32 rows[2][RES_MAXX];
	Uint32 *prev, *current;
	for (int i = 0; i < xr; i++)
		rows[0][i] = fb[i];
	prev = rows[0];
	current = rows[1];
	for (int j = 1; j < yr - 1; j++, p += 2) {
		memcpy(current, fb+(j * xr), xr * 4);
		for (int i = 1; i < xr - 1; i++, p++) {
			fsaa_set_entry sentry(&prev[i], &current[i], &fb[p + xr], xr);
			if (sentry.jagged()) {
				sentry.sort();
				HashMap<fsaa_set_entry, unsigned>::iterator *it = m.find(sentry);
				if (it) {
					fb[p] = 0x80000000 + it->second;
				} else {
					fb[p] = 0x80000000 + c;
					m.insert(sentry, c);
					c++;
				}
				if (i < RowMin[j]) RowMin[j] = i;
				if (i > RowMax[j]) RowMax[j] = i;
			}
		}
		Uint32 *t = current;
		current = prev;
		prev = t;
	}
	if (fsaa_info) {
		delete [] fsaa_info;
		fsaa_info = NULL;
	}
	if (c) {
		fsaa_info = new fsaa_info_entry[c];
		m.iter_start();
		HashMap<fsaa_set_entry, unsigned>::iterator *i;
		while (NULL != (i = m.iter())) {
			prepare_fsaa_info_entry(i->first, fsaa_info[i->second]);
		}
	}
	if (c > maxsize) maxsize = c;
}


void antialias_close(void)
{
	//printf("antialias_close(): maxsize = %d\n", maxsize);
	if (fsaa_info) {
		delete [] fsaa_info;
		fsaa_info = NULL;
	}
}
