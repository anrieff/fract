/***************************************************************************
 *   Copyright (C) 2005 by Veselin Georgiev                                *
 *   vesko@ChaosGroup.com                                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <string.h>
 
/**
 * @File	cpuid.cpp
 * @Author	Veselin Georgiev
 * @Date	2006-08-10
 * @Brief	Identifies CPU type using CPUID
 * 
 * This is forged off with the help from the Camel project 
 * (www.codeproject.com/system/camel.asp), but since no single line from
 * it is copied, you might consider it my own work.
 *
 * If you wish to use this piece of code in your own project (free or 
 * commercial), please write me (anrieff@mgail.com (convert to gmail)) and
 * just ask me.
*/ 

/**
 * @brief Executes CPUID
 * @param val_eax - The value of eax register prior to executing CPUID
 * @param res - The result, stored as follows:
 *        res[0] == eax
 *        res[1] == ebx
 *        res[2] == ecx
 *        res[3] == edx
*/
static void CPUID(int val_eax, int res[])
{
#ifdef __GNUC__
	__asm __volatile(
		"	push	%%eax\n"
		"	push	%%ebx\n"
		"	push	%%ecx\n"
		"	push	%%edx\n"
		"	push	%%edi\n"
		"	mov	%0,	%%eax\n"
		"	mov	%1,	%%edi\n"
		
		"	cpuid\n"
		
		"	mov	%%eax,	(%%edi)\n"
		"	mov	%%ebx,	4(%%edi)\n"
		"	mov	%%ecx,	8(%%edi)\n"
		"	mov	%%edx,	12(%%edi)\n"
		"	pop	%%edi\n"
		"	pop	%%edx\n"
		"	pop	%%ecx\n"
		"	pop	%%ebx\n"
		"	pop	%%eax\n"
		:
		:"m"(val_eax), "m"(res)
		:"memory"
	);
#else
	__asm {
		push	eax
		push	ebx
		push	ecx
		push	edx
		push	edi
		mov	eax,	val_eax
		mov	edi,	res
		
		cpuid
		
		mov	[edi],	eax
		mov	[edi+4],	ebx
		mov	[edi+8],	ecx
		mov	[edi+12],	edx
		
		pop	edi
		pop	edx
		pop	ecx
		pop	ebx
		pop	eax
	}
#endif
}

// Include the database of CPU IDs, a.k.a. the matchtable
#include "cpuid_database.h"

static int score(const MatchEntry& m, int f, int mod, int s, int em, int ef)
{
	int res = 0;
	if (m.family		== f  ) res +=2;
	if (m.model		== mod) res +=2;
	if (m.stepping		== s  ) res +=2;
	if (m.ext_family	== ef ) res +=2;
	if (m.ext_model		== em ) res +=2;
	
	return res;
}

enum {
	VENDOR_INTEL,
	VENDOR_AMD,
	VENDOR_UNKNOWN
};

static int cpu_vendor(void)
{
	int res[4];
	CPUID(0, res);
	char vendor_id[13];
	memcpy(&vendor_id[0], &res[1], 4);
	memcpy(&vendor_id[4], &res[3], 4);
	memcpy(&vendor_id[8], &res[2], 4);
	vendor_id[12] = 0;
	if (!strcmp(vendor_id, "GenuineIntel")) return VENDOR_INTEL;
	if (!strcmp(vendor_id, "AuthenticAMD")) return VENDOR_AMD;
	return VENDOR_UNKNOWN;
}

const char *identify_cpu(void)
{
	int num_entries;
	const MatchEntry *cpudb = NULL;
	switch (cpu_vendor()) {
		case VENDOR_INTEL: 
			cpudb = cpudb_intel;
			num_entries = (int) sizeof(cpudb_intel) / sizeof(cpudb_intel[0]);
			break;
		case VENDOR_AMD:
			cpudb = cpudb_amd;
			num_entries = (int) sizeof(cpudb_amd) / sizeof(cpudb_amd[0]);
			break;
		default:
			return "Unknown CPU";
	}
	int res1[4];
	CPUID(1, res1);
	
	int family, model, stepping;
	int ext_family, ext_model;
	
	int r = res1[0];
	stepping    = r & 0xf; r >>= 4;
	model       = r & 0xf; r >>= 4;
	family      = r & 0xf; r >>= 8;
	ext_model   = r & 0xf; r >>= 4;
	ext_family  = r & 0xff;
	
	// we have what we need, try matching now.
	int bestscore = -1;
	int bestindex = 0;
	
	
	for (int i = 0; i < num_entries; i++) {
		int t = score(cpudb[i], family, model, stepping, ext_model, ext_family);
		if (t > bestscore) {
			bestscore = t;
			bestindex = i;
		}
	}
	
	return cpudb[bestindex].name;
}
