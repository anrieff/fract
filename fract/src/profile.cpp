/***************************************************************************
 *   Copyright (C) 2004 by Veselin Georgiev                                *
 *   vesko@ViruS                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *-------------------------------------------------------------------------*
 *   Provides profiling routines                                           *
 *								           *
 ***************************************************************************/

#include <stdio.h>
#include "MyTypes.h"
#include "profile.h"
#include "fract.h"

#include "idnames.h"

long long prof_data[MAX_PROF_ENTRIES];
long long prof_mark[MAX_PROF_ENTRIES];

#define profile_asm
#include "x86_asm.h"
#undef profile_asm

#ifdef MAKE_PROFILING
void prof_enter(unsigned ID)
{
	prof_mark[ID] = prof_rdtsc();
}

void prof_leave(unsigned ID)
{
	prof_data[ID] += prof_rdtsc() - prof_mark[ID];
}

void prof_init(void)
{
	int i;
	for (i=0;i<MAX_PROF_ENTRIES;i++) prof_data[i] = 0L;
}
#endif // MAKE_PROFILING

void prof_statistics(void)
{
#ifndef MAKE_PROFILING
	printf("Profiling statistcs not available: profiling code not enabled.\nRecompile with MAKE_PROFILING to use this feature\n");
	return;
#endif
	int i, j, f;
	long long sall, st;

	printf("Profile Statistics:\n\n");

	sall = 0;
	for (i=0;i<256;i+=16) sall+=prof_data[i];
	for (i=0;i<256;i+=16) if (prof_data[i]) {
		printf(" [%6.2lf%%] %s\n", 100.0 * ((double) prof_data[i]/(double) sall), getidname(i));
		st = 0;
		f = 0;
		for (j=1;j<16;j++)
			if (prof_data[i+j]) {
				f = 1;
				st += prof_data[i+j];
				printf("    +---[%6.2lf%%] %s\n", 100.0*((double) prof_data[i+j]/
								(double) prof_data[i]), getidname(i+j));
			}
		if (f) printf("    `---[%6.2lf%%] other\n",((1.0 - (double)st/(double) prof_data[i])*100.0));
		printf("    |\n");
	}
}


// gets the CPU speed using RDTSC.
int get_cpu_speed(void)
{
	long long start, end;
	Uint32 clk0, clk;
	int res;

	clk0 = get_ticks();
	while ((clk=get_ticks()) == clk0);
	start = prof_rdtsc();
	clk0 = clk;
	while ((clk=get_ticks()) - clk0 < CPU_CALIBRATE_TIME) ;
	end = prof_rdtsc();

	res = (end - start) / ((long long)(clk - clk0));
	res /= 1000;
	if (res > 99999) res = 99999;
	if (res < 0    ) res =     0;
	return res;
}
