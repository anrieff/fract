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
 *                                                                         *
 ***************************************************************************/

#include <stdio.h>
#include <string.h>
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

/*
 * All the code below is dedicated to measuring CPU speed, in Mhz, for the
 * official fract benchmarking.
 *
 * prof_get_cpu_speed() is the master function, which measures CPU speed by
 * some method. The simplest method, implemented in ver 1.07a and before, is
 * to wait some fixed amount of time and calculate the CPU MHz based on the
 * delta of RDTSC's.
 *
 * This method was frequently fooled by power saving managers utilizing
 * CPU scaling in newer chips. So in 1.07b, triple checked version was used.
 *
 * However, even this could be eventually fooled, so in 1.08a a new approach
 * is taken: two measurements of time ticks and CPU HZ ticks are made
 * while the bench actually renders. This ensures the CPUs are under stress
 * and thus the frequency would be set to the highest available setting.
 * Moreover, we are just counting the Hz the CPU has given to the bench,
 * so efficiency charts are as accurate as possible.
 *
 * In order to avoid CPU scaling managers as much as we can, we make the first
 * measurement just 3.5 seconds after the rendering has begun (to ensure
 * the CPU has `spun up' and the frequency increased). The second measurement
 * is made at the end of the bench.
 *
 * If prof_get_cpu_speed() decides the data gathered this way would be
 * inacurate, the old method is used.
 *
 * Measurements are made by calling the mark_cpu_rdtsc() method.
 */

struct CPUHzStruct {
	Uint32 ticks;
	long long cpu_cycles;
};

static CPUHzStruct cpu_ticks[2];
static int cpu_ticks_count = 0;

void mark_cpu_rdtsc(void)
{
	if (cpu_ticks_count == 2)
		--cpu_ticks_count;
	cpu_ticks[cpu_ticks_count].ticks = get_ticks();
	cpu_ticks[cpu_ticks_count].cpu_cycles = prof_rdtsc();
	cpu_ticks_count++;
}

int get_marked_cpu_speed(void)
{
	// Not enough time points? bail off
	if (cpu_ticks_count < 2) return -1;
	long long timedelta = (long long) cpu_ticks[1].ticks - (long long) cpu_ticks[0].ticks;
	// time points too close in time? bail off
	if (timedelta < CPU_CALIBRATE_TIME) return -1;
	long long hzdelta = cpu_ticks[1].cpu_cycles - cpu_ticks[0].cpu_cycles;
	long long result = timedelta / hzdelta / 1000;
	// some common sense checks for correctness
	if (result < 1 || result > 99999) return -1;
	// result looks good, use it...
	return (int) result;
}


// gets the CPU speed using RDTSC.
int prof_get_cpu_speed_once(void)
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

/*
** prof_get_cpu_speed() - gets the CPU speed, in MHz, by various methods.
**
** If official benchmarking completed, we rely on rendering code to call
** our mark_cpu_rdtsc() function twice, which would give us two time/hz points
** to accurately calculate CPU speed. If we don't have it, use the old method
**
** It is triple checked, in order to ensure result correctness
**
** The algorithm is as follows: 
** 1. prof_get_cpu_speed_once() is called three times unconditionally;
** 2. Two results are selected: the minimally different by absolute value (assumed to be "correct");
** 3. The mean of the selected values is returned as a final result.
*/
int prof_get_cpu_speed(void)
{
	int r = get_marked_cpu_speed();
	if (r != -1) return r;
	//
	int res[3];
	for (int i = 0; i < 3; i++)
		res[i] = prof_get_cpu_speed_once();
	
	int answer = 0, bestabs = 0x7fffffff;
	for (int i = 0; i < 3; i++) {
		for (int j = i + 1; j < 3; j++) {
			int diff = res[i]-res[j]; if (diff < 0) diff = -diff;
			if (diff < bestabs) {
				bestabs = diff;
				answer = (res[i] + res[j]) / 2;
			}
		}
	}
	return answer;
}
