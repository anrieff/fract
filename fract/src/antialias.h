/***************************************************************************
 *   Copyright (C) 2004 by Veselin Georgiev                                *
 *   vesko@ViruS                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "MyTypes.h"
#include "vector3.h"

#define FSAA_MODE_NONE                  0
#define FSAA_MODE_4X_LO_Q               2
#define FSAA_MODE_4X_HI_Q               3
#define FSAA_MODE_ADAPT_4             100
#define FSAA_MODE_ADAPT_QUINCUNX      101
#define FSAA_MODE_ADAPT_10            102
#define FSAA_MODE_ADAPT_16            103

// the mode that will be set in the beginning, if possible
#define FSAA_STARTING_MODE FSAA_MODE_ADAPT_QUINCUNX
//#define FSAA_STARTING_MODE FSAA_MODE_NONE

#define FSAA_MAX_SAMPLES         16
#define ED_KERNEL_SIZE            5

struct fsaa_kernel {
	double coords[FSAA_MAX_SAMPLES][2];
	float weights[FSAA_MAX_SAMPLES];
	int count;
};

extern const fsaa_kernel aaa_kernel_4, aaa_kernel_16, aaa_kernel_quincunx, aaa_kernel_10;

struct raycasting_ctx {
	Vector sample[FSAA_MAX_SAMPLES];
	float weights[FSAA_MAX_SAMPLES];
	int count;
};

struct fsaa_bucket {
	int id;
	float color[3];
	float weight;
	float opacity;
};

struct fsaa_set_entry {
	Uint32 a[ED_KERNEL_SIZE];
	fsaa_set_entry (Uint32 *prev, Uint32* current, Uint32 *next, int xsize) 
	{
		a[0] = prev[0];
		a[1] = current[-1];
		a[2] = current[ 0];
		a[3] = current[+1];
		a[4] = next[0];
		//
	}
	inline Uint32 get_hash_code() const
	{
		Uint32 code = 0;
		for (int i = 0; i < ED_KERNEL_SIZE; i++) {
			code = ((code << 2) ^ ((code >> 1) * 37337) + 0x16253bac) ^ a[i];
		}
		return code;
	}
	inline bool operator == (const fsaa_set_entry& r) const
	{
		for (int i = 0; i < ED_KERNEL_SIZE; i++) 
			if (a[i] != r.a[i]) return false;
		return true;
	}
	bool jagged() const;
	void sort();
};

struct fsaa_info_entry {
	int size;
	Uint32 ids[10];
	fsaa_info_entry() {}
	void sort(void);
};


extern int fsaa_mode;
extern char fsaa_name[];
extern double last_fsaa_change;
extern Uint8 fb_copy[];
extern fsaa_info_entry *fsaa_info;
extern bool g_scpuabi;


void set_fsaa_mode(int newMode);
void antialias(Uint32 *fb);
void antialias(Uint8 *fb);
int  xsize_render(int x);
int  ysize_render(int y);
void check_fsaa_param(void);
bool is_adaptive_fsaa(void);
void prepare_fsaa_context(raycasting_ctx *, const Vector & column_inc, const Vector & row_inc);
void antibuffer_init(Uint32 fb[], int xr, int yr);

void antialias_close(void);
