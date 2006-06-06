/***************************************************************************
 *   Copyright (C) 2004 by Veselin Georgiev                                *
 *   vesko@ViruS                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef __CPUID_H__
#define __CPUID_H__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmdline.h"

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


//these are required for the windoze version of the CPU feature detection
// routine, since we can't use that cute /proc/cpuinfo ... :(
#define MMX_MASK  0x00800000
#define SSE_MASK  0x02000000
#define SSE2_MASK  0x04000000
#define MMX2_MASK 0x00400000
// the contents of the EBX register after a CPUID call with eax=0.
#define INTEL_EBX 0x756e6547 // 'Genu' in intel order

// bigarray is 64 MB large; this defines how much memory (in MB) will be (at least) required in order to use bigarray...
// 100 Megabytes seems like a good value to me (will be ok for people with 128 MB and will piss off people with 80 or 96 MB, who could
// use the bigarray on a properly tuned machine; it's a rather uncommon configuration and those people surely know how to recompile...)
// NOTE FOR WINDOWS USERS: in Windows, bigarr utilization is disabled no matter how much memory do you have.
//			   fract can be forced to use it with --use-mem switch
#define MEMORY_REQUIREMENT 100

static void run_rgbhacks1(int & hassse, int & hasmmx, int & hasmmx2, int & hassse2) 
#ifdef __GNUC__
__attribute__((noinline))
#else
#endif
;

class SystemInfo {
	char flags[256];
	int cpucount;
	int memory;
	bool f_largemem;
	bool flags_initialized;

void addfeature(char *new_feature) {
		strcat(flags, " ");
		strcat(flags, new_feature);
	}

void removefeature(char *feature) {
		char *s;
		if (!( s = strstr(flags, feature))) return;
		while (' '!=(*s) && 0!=(*s)) {
			*s='.';
			s++;
		}
	}
	public:
	SystemInfo() {
		flags[0] = 0;
		memory = -1;
		flags_initialized = false;
#ifdef linux
		//this is how we get the number of CPUs under Linux
		cpucount = sysconf(_SC_NPROCESSORS_ONLN);
#else
#ifdef _MSC_VER
		// and this is under Win32
		SYSTEM_INFO system_info;
		GetSystemInfo(&system_info);
		cpucount = system_info.dwNumberOfProcessors;
#else
#ifdef __APPLE__
        kern_return_t kr;
        host_basic_info_data_t basic_info;
        host_info_t info = (host_info_t)&basic_info;
        host_flavor_t flavor = HOST_BASIC_INFO;
        mach_msg_type_number_t count = HOST_BASIC_INFO_COUNT;
        kr = host_info(mach_host_self(), flavor, info, &count);
        if (kr != KERN_SUCCESS) cpucount = 1;
        cpucount = basic_info.avail_cpus;
#else
#warning Getting the processor count under MinGW32 is not implemented!
		cpucount = 1;
#endif
#endif
#endif
	}
	/// checks if the specified feature is supported
	/// @param feature is of the form "mmx" "tsc" etc
	/// @see linux /proc/cpuinfo for feature names
	bool supports(char *feature) {
		if (!flags_initialized) { // perform initialization
			flags_initialized = true;
#ifdef USE_ASSEMBLY
#ifdef linux
			FILE *f;
			if ( ( f = fopen("/proc/cpuinfo", "rt")) == NULL ) {
				printf("cannot open /proc/cpuinfo!\n");
			} else {
				char buff[256];
				while (fgets(buff, 256, f)) {
					if (!strncmp("flags", buff, 5)) {
						strcpy(flags, buff);
						break;
					}
				}
				fclose(f);
			}
#else
#endif
			int hassse, hasmmx, hasmmx2, hassse2;
			run_rgbhacks1(hassse, hasmmx, hasmmx2, hassse2);
			if (hasmmx) strcat(flags, "mmx");
			if (hasmmx2) strcat(flags, "mmx2");
			if (hassse) strcat(flags, "sse");
			if (hassse2) strcat(flags, "sse2");

#else
// no assembly will be used. No need to check for supported features
		flags[0]=0;
#endif
			if (OptionExists("--has-mmx")) addfeature("mmx");
			if (OptionExists("--has-sse")) addfeature("sse");
			if (OptionExists("--has-mmx2")) addfeature("mmx2");
			if (OptionExists("--has-sse2")) addfeature("sse2");
			if (OptionExists("--no-mmx")) removefeature("mmx");
			if (OptionExists("--no-mmx2")) removefeature("mmx2");
			if (OptionExists("--no-sse")) removefeature("sse");
			if (OptionExists("--no-sse2")) removefeature("sse2");
		}
		return (strstr(flags, feature)!=NULL);
	}

	// returns the amount of memory in KB
	int memsize() {
		if (-1 == memory) {
#ifdef linux
			struct sysinfo si;
			sysinfo(&si);
			memory = si.totalram * si.mem_unit / 1024;
#else
			printf("feature not implemented: getting amount of memory under windows\n");
#endif
			if (OptionExists("--force-mem")) {
			// sufficient for bigarr
				memory = MEMORY_REQUIREMENT*1024;
			}
			if (OptionExists("--no-mem")) {
			// insufficient for bigarr:
				memory = (MEMORY_REQUIREMENT-1)*1024;
			}
			f_largemem = (memory / 1024 >= MEMORY_REQUIREMENT);
		}
		return memory;
	}
	/// checks if we have enough memory to use bigarray
	/// @relates rgb2yuv.cpp
	bool largemem() {
		memsize();
		return f_largemem;
	}

	void set_largemem(bool new_setting) {
		f_largemem = new_setting;
	}
	
	/// returns the number of CPUs in the system
	int cpu_count() {
		return cpucount;
	}
	
	/// forces the number of CPUs to the given value
	void set_cpu_count(int newcount) {
		cpucount = newcount;
	}
	
// this includes a routine that checks for intel CPU
// It turns out that there's difference between the SSE
// implementations between AMD and Intel CPUs and the checksums
// from the generated image are different

/// int is_intel(void)
/// @returns 0 if not intel cpu, nonzero otherwise
#ifdef USE_ASSEMBLY
#define rgbhacks2
#include "x86_asm.h"
#undef rgbhacks2
#else
/// can't determine
int is_intel(void) {return 0;}
#endif
};


#ifndef RGB2YUV_CPP
extern SystemInfo SysInfo;
#endif

static void 
run_rgbhacks1(int & hassse, int & hasmmx, int & hasmmx2, int & hassse2)
{
	#define rgbhacks1
	#include "x86_asm.h"
	#undef rgbhacks1
}


#endif // __CPUID_H__
