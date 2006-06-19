/***************************************************************************
 *   Copyright (C) 2006 by Veselin Georgiev                                *
 *   anrieff@mailg.com (convert to gmail)                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// all OS stuff
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// system dependent stuff
#if defined linux
#include <sys/sysinfo.h>
#include <unistd.h>
#else
#ifdef _WIN32
#include <windows.h>
#endif
#endif

#ifdef __APPLE__
#include <unistd.h>
#include <mach/clock_types.h>
#include <mach/clock.h>
#include <mach/mach.h>
#endif

// my headers
#include "asmconfig.h"
#include "cmdline.h"
#include "cpu.h"
#include "profile.h"
#include "threads.h"


// GLOBAL VARIABLEs
CPU cpu;


//these are required for the windoze version of the CPU feature detection
// routine, since we can't use that cute /proc/cpuinfo ... :(
#define MMX_MASK  0x00800000
#define SSE_MASK  0x02000000
#define SSE2_MASK  0x04000000
#define MMX2_MASK 0x00400000



static void run_rgbhacks1(int & hassse, int & hasmmx, int & hasmmx2, int & hassse2) 
#ifdef __GNUC__
__attribute__((noinline))
#else
#endif
;

static void get_vendor_string(char *name) 
#ifdef __GNUC__
		__attribute__((noinline))
#else
#endif
;


void CPU::init()
{
	/**************************************
	 Get the processor count
	**************************************/
#ifdef linux
	//this is how we get the number of CPUs under Linux
	count = sysconf(_SC_NPROCESSORS_ONLN);
#else
#ifdef _MSC_VER
	// and this is under Win32
	SYSTEM_INFO system_info;
	GetSystemInfo(&system_info);
	count = system_info.dwNumberOfProcessors;
#else
#ifdef __APPLE__
	// ... and yet this is under Mac OS X
	kern_return_t kr;
	host_basic_info_data_t basic_info;
	host_info_t info = (host_info_t)&basic_info;
	host_flavor_t flavor = HOST_BASIC_INFO;
	mach_msg_type_number_t count = HOST_BASIC_INFO_COUNT;
	kr = host_info(mach_host_self(), flavor, info, &count);
	if (kr != KERN_SUCCESS) count = 1;
	count = basic_info.avail_cpus;
#else
#warning Getting the processor count under MinGW32 is not implemented!
	count = 1;
#endif // __APPLE__
#endif // _MSC_VER
#endif // linux

	/****************************************
	 Seek for processor features
	****************************************/
#ifdef USE_ASSEMBLY
	int hassse, hasmmx, hasmmx2, hassse2;
	run_rgbhacks1(hassse, hasmmx, hasmmx2, hassse2);
	if (hasmmx) mmx = true;
	if (hasmmx2) mmx2 = true;
	if (hassse) sse = true;
	if (hassse2) sse2 = true;
#endif // USE_ASSEMBLY

	/****************************************
	 Determine the amount of memory
	****************************************/
	memory_size = 0;
#ifdef linux
	struct sysinfo si;
	sysinfo(&si);
	memory_size = si.totalram * si.mem_unit / 1024;
#endif // no implementations for other OSes, sorry
	
	/****************************************
	 Get the CPU vendor string
	****************************************/
	memset(vendor_id, 0, sizeof(vendor_id));
#ifdef USE_ASSEMBLY
	get_vendor_string(vendor_id);
#endif

	/****************************************
	 Cope with user-settable options
	****************************************/
	if (option_exists("--has-mmx" )) mmx  = true;
	if (option_exists("--has-sse" )) sse  = true;
	if (option_exists("--has-mmx2")) mmx2 = true;
	if (option_exists("--has-sse2")) sse2 = true;
	if (option_exists("--no-mmx"  )) mmx  = false;
	if (option_exists("--no-mmx2" )) mmx2 = false;
	if (option_exists("--no-sse"  )) sse  = false;
	if (option_exists("--no-sse2" )) sse2 = false;
	if (option_exists("--no-mem"  )) memory_size = 0;
	if (option_exists("--force-mem")) memory_size = MEMORY_REQUIREMENT * 1024;

}

bool CPU::is_intel(void) const
{
	return 0 == strcmp(vendor_id, "GenuineIntel");
}

unsigned CPU::speed(void)
{
	if (!_speed_inited) {
		_speed_inited = true;
		_im_speed = prof_get_cpu_speed();
	}
	return _im_speed;
}

static void 
run_rgbhacks1(int & hassse, int & hasmmx, int & hasmmx2, int & hassse2)
{
	#define rgbhacks1
	#include "x86_asm.h"
	#undef rgbhacks1
}

#define rgbhacks2
#include "x86_asm.h"
#undef rgbhacks2

void set_cpus(int new_count)
{
	if (new_count < 1) {
		printf("Invalid processor count: %d\n", new_count);
		new_count = 1;
	} else if (new_count > MAX_CPU_COUNT) {
		printf("Sorry. Maximum allowable CPU count is %d\n", MAX_CPU_COUNT);
		new_count = MAX_CPU_COUNT;
	}
	cpu.count = new_count;
}

void print_cpu_info(void)
{
	printf("CPU count = %d\n", cpu.count);
	printf("CPU speed = %d MHz\n", cpu.speed());
	printf("Memory size = %d KB\n", cpu.memory_size);
	printf("CPU features:\n");
	printf("\tMMX  = %s\n", cpu.mmx  ? "present" : "absent");
	printf("\tMMX2 = %s\n", cpu.mmx2 ? "present" : "absent");
	printf("\tSSE  = %s\n", cpu.sse  ? "present" : "absent");
	printf("\tSSE2 = %s\n", cpu.sse2 ? "present" : "absent");
	printf("CPU vendor string = `%s'\n", cpu.vendor_id);
}
