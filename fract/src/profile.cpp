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

// gets the CPU speed using RDTSC. Defined on all systems
int prof_get_cpu_speed_fallback(void)
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


#ifdef _WIN32
#define GETCPUSPEED_DEFINED
#include <windows.h>
// gets the CPU speed using windows registry
int prof_get_cpu_speed(void)
{
	HKEY key;
	long err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
				"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
				0,
				KEY_READ,
				&key);
	if (err != ERROR_SUCCESS) return prof_get_cpu_speed_fallback();
	DWORD res = _MAX_PATH, bufsize = _MAX_PATH;
	RegQueryValueEx(key, "~MHz", NULL, NULL, (LPBYTE) &res, &bufsize);
	return (int) res;
}
#endif

#ifdef linux
#define GETCPUSPEED_DEFINED
// gets the CPU speed using /proc/cpuinfo
int prof_get_cpu_speed(void)
{
	FILE *f = fopen("/proc/cpuinfo", "rt");
	if (!f) return prof_get_cpu_speed_fallback();
	char buff[256];
	while (fgets(buff, 256, f)) {
		if (strncasecmp(buff, "cpu mhz", 7) == 0) {
			int i = 0;
			while (buff[i] && (buff[i] < '0' || buff[i] > '9')) i++;
			int res;
			if (!buff[i]) res = prof_get_cpu_speed_fallback();
			else {
				sscanf(&buff[i], "%d", &res);
			}
			fclose(f);
			return res;
		}
	}
	fclose(f);
	return prof_get_cpu_speed_fallback();
}
#endif


#ifndef GETCPUSPEED_DEFINED
// This is only for non-linux and non-windows systems:
int prof_get_cpu_speed(void)
{
	return prof_get_cpu_speed_fallback();
}
#endif
