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

// bigarray is 64 MB large; this defines how much memory (in MB) will be (at least) required in order to use bigarray...
// 100 Megabytes seems like a good value to me (will be ok for people with 128 MB and will piss off people with 80 or 96 MB,
// who could use the bigarray on a properly tuned machine; it's a rather uncommon configuration and those people surely 
// know how to recompile...)
// NOTE FOR WINDOWS USERS: in Windows, bigarr utilization is disabled no matter how much memory do you have.
//			   fract can be forced to use it with --use-mem switch
#define MEMORY_REQUIREMENT 100


struct CPU {
	int count;
	bool mmx;
	bool mmx2;
	bool sse;
	bool sse2;
	int memory_size; // in KB
	char vendor_id[13];
	
	void init();
	unsigned speed(void); // in MHz
	bool is_intel(void) const;
private:
	bool _speed_inited;
	unsigned _im_speed;
};

extern CPU cpu;

void set_cpus(int new_count);
void print_cpu_info(void);

#endif // __CPUID_H__
