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

void set_cpus(int new_count, char* error_msg = NULL);
void print_cpu_info(void);

#endif // __CPUID_H__
