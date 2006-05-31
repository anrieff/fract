/***************************************************************************
 *   Copyright (C) 2004 by Veselin Georgiev                                *
 *   vesko@ViruS                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *-------------------------------------------------------------------------*
 *   Describes the result file format                                      *
 ***************************************************************************/

#ifndef __RESULTFMT_H__
#define __RESULTFMT_H__

#include "md5.h"

#define MAX_COMMENT 768

typedef unsigned char BYTE;

#ifdef _MSC_VER
#include <pshpack1.h>
#endif
struct result_file {

	/* # build info */
	char version[32];
	char OS[16];
	char compiler[16];

	/* # machine */
	int cpu_count;
	char cpu_type[32];
	int cpu_mhz;
	char chipset[16];
	BYTE mach_padding[8];

	/* # results */
	int res_x, res_y;
	float overall_fps;
	float scene[5];
	BYTE results_padding[32];

	/* # user info */
	char username[32];
	char country[16];
	BYTE md5sum[16];

	/* # comment */
	char comment[MAX_COMMENT];

	/* **methods** */
	void calculate_md5_sum(void *dest)
	{
		int size = (int) ((BYTE*)&(this->md5sum) - (BYTE*)this);
		MD5Hasher hash;
		hash.add_data(this, size);
		hash.result(dest);
	}	
	bool compare_md5_sum(const void *in_md5sum)
	{
		BYTE*in = (BYTE*) in_md5sum;
		for (int i =0 ; i < 16; i++)
			if (in[i] != md5sum[i])
				return false;
		return true;
	}
#ifdef _MSC_VER
};
#include <poppack.h>
#else
} __attribute__((packed));
#endif

#endif // __RESULTFMT_H__
