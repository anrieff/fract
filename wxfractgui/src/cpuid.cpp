/***************************************************************************
 *   Copyright (C) 2006 by Veselin Georgiev                                *
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
 
/*
** Copyright 2003,2004,2005,2006 by Todd Allen.  All Rights Reserved.
** Permission to use, copy, modify, distribute, and sell this software and
** its documentation for any purpose is hereby granted without fee, provided
** that the above copyright notice appear in all copies and that both the
** copyright notice and this permission notice appear in supporting
** documentation.
**
** No representations are made about the suitability of this software for any
** purpose.  It is provided ``as is'' without express or implied warranty,
** including but not limited to the warranties of merchantability, fitness
** for a particular purpose, and noninfringement.  In no event shall Todd
** Allen be liable for any claim, damages, or other liability, whether in
** action of contract, tort, or otherwise, arising from, out of, or in
** connection with this software.
*/

#include <stdio.h>
#include <string.h>
 
/**
 * @File	cpuid.cpp
 * @Author	Veselin Georgiev
 * @Date	2006-08-10
 * @Brief	Identifies CPU type using CPUID
 * 
 * This is forged off with the help from the Camel project and the cpuid
 * program (http://www.etallen.com/cpuid.html)
 *
 * If you wish to use this piece of code in your own project (free or 
 * commercial), please write me (anrieff@mgail.com (convert to gmail)) and
 * just ask me.
*/ 

// Include the database of CPU IDs, a.k.a. the matchtable
#include "cpuid_database.h"


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
#	ifdef __x86_64__
	__asm __volatile(
		"	push	%%rbx\n"
		"	push	%%rcx\n"
		"	push	%%rdx\n"
		"	push	%%rdi\n"
		
		"	mov	%1,	%%rdi\n"
		
		"	xor	%%ebx,	%%ebx\n"
		"	xor	%%ecx,	%%ecx\n"
		"	xor	%%edx,	%%edx\n"
		
		"	cpuid\n"
		
		"	movl	%%eax,	(%%rdi)\n"
		"	movl	%%ebx,	4(%%rdi)\n"
		"	movl	%%ecx,	8(%%rdi)\n"
		"	movl	%%edx,	12(%%rdi)\n"
		"	pop	%%rdi\n"
		"	pop	%%rdx\n"
		"	pop	%%rcx\n"
		"	pop	%%rbx\n"
		:
		:"a"(val_eax), "rdi"(res)
		:"memory"
	);
#	else
	__asm __volatile(
		"	push	%%eax\n"
		"	push	%%ebx\n"
		"	push	%%ecx\n"
		"	push	%%edx\n"
		"	push	%%edi\n"
		"	mov	%0,	%%eax\n"
		"	mov	%1,	%%edi\n"
		
		"	xor	%%ebx,	%%ebx\n"
		"	xor	%%ecx,	%%ecx\n"
		"	xor	%%edx,	%%edx\n"
		
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
#	endif // __x86_64__
#else
	__asm {
		push	eax
		push	ebx
		push	ecx
		push	edx
		push	edi
		mov	eax,	val_eax
		mov	edi,	res
		xor	ebx,	ebx
		xor	ecx,	ecx
		xor	edx,	edx
		
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

enum {
	VENDOR_INTEL,
	VENDOR_AMD,
	VENDOR_UNKNOWN
};



// This hack is intended for testing. If the executable finds a file, named `simulator.txt'
// it reads various CPU info from it and pretends it's running on the specified
// cpu, faking CPUID calls, etc.
bool simulator = false;
struct SimulatorInfo {
	int num_cores, logical_cpus, family, model, stepping, ext_family, ext_model;
	int vendor;
	int cache_size;
	char brand[49];
} sinfo;


/*
 * Simulator file fmt:
 *
 * <vendor> (0 = intel, 1 = amd)
 * <family> <model> <stepping> <ext_family> <ext_model>
 * <num_cores> <logical_cpus>
 * <cache_size_in_KB>
 * <cpu_brand_string>
*/
static bool simulator_readinfo(const char *fn)
{
	FILE *f = fopen(fn, "rt");
	if (!f) return false;
	bool good = false;
	if (1 != fscanf(f, "%d", &sinfo.vendor)) goto FAIL;
	if (5 != fscanf(f, "%d%d%d%d%d", &sinfo.family, &sinfo.model, &sinfo.stepping,
		&sinfo.ext_family, &sinfo.ext_model)) goto FAIL;
	if (2 != fscanf(f, "%d%d", &sinfo.num_cores, &sinfo.logical_cpus)) goto FAIL;
	if (1 != fscanf(f, "%d", &sinfo.cache_size)) goto FAIL;
	if (!fgets(sinfo.brand, 49, f)) goto FAIL;
	if (!fgets(sinfo.brand, 49, f)) goto FAIL;
	if (!strlen(sinfo.brand)) goto FAIL;
	sinfo.brand[strlen(sinfo.brand)-1] = 0;
	good = true;

FAIL:
	fclose(f);
	return good;
}

const char *cpu_brand_string(void)
{
	static char reso[64];
	int i;
	
	int xres[4];
	
	CPUID(0x80000000, xres);
	if ((unsigned) xres[0] < 0x80000004) return NULL;
	
	for (i = 0; i < 3; i++) {
		CPUID(0x80000002 + i, xres);
		for (int j = 0; j < 4; j++)
			memcpy(&reso[i*16+j*4], &xres[j], 4);
	}
	reso[48] = 0;
	i = 0;
	while (reso[i] && reso[i] == ' ') i++;
	return &reso[i];
}

static void fill_cache_info(bool *results)
{
	for (int i = 0; i < 256; i++) results[i] = false;
	int r[4];
	
	CPUID(0, r);
	if (r[0] < 2) return;
	
	CPUID(2, r);
	int cycles_left = r[0] & 0xff;

	for (;cycles_left; --cycles_left) {
		for (int i = 0; i < 4; i++) if (0 == (0x80000000 & r[i])) {
			for (int j = 0; j < 4; j++) {
				if (i + j > 0) results[r[i]&0xff] = true;
				r[i] >>= 8;
			}
		}
		if (cycles_left > 1) CPUID(2, r);
	}
}

static int score(const MatchEntry& m, int f, int mod, int s, int em, int ef, code_t cod)
{
	int res = 0;
	if (m.family		== f  ) res +=2;
	if (m.model		== mod) res +=2;
	if (m.stepping		== s  ) res +=2;
	if (m.ext_family	== ef ) res +=2;
	if (m.ext_model		== em ) res +=2;
	if (m.code		== cod) res +=2;
	
	return res;
}

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

static code_t get_cpu_code_phase1(int vendor)
{
	const char *brand = cpu_brand_string();
	if (simulator)
		brand = sinfo.brand;
	if (brand == NULL) return UN;
	switch (vendor) {
		case VENDOR_INTEL:
		{
			if (strstr(brand, "Mobile") != NULL) {
				if (strstr(brand, "Celeron") != NULL) {
					return MC;
				} else if (strstr(brand, "Pentium") != NULL) {
					return MP;
				}
			} else {
				if (strstr(brand, "Xeon MP") != NULL
				|| strstr(brand, "Xeon(TM) MP") != NULL) {
					return sM;
				} else if (strstr(brand, "Xeon") != NULL) {
					return sX;
				} else if (strstr(brand, "Celeron") != NULL) {
					return dC;
				} else if (strstr(brand, "Pentium(R) M") != NULL) {
					return MM;
				} else if (strstr(brand, "Pentium(R) D") != NULL) {
					return dD;
				} else if (strstr(brand, "Pentium") != NULL) {
					return dP;
				} else if (strstr(brand, "Genuine Intel(R) CPU") != NULL
					|| strstr(brand, "Intel(R) Core(TM)2") != NULL) {
					return dc;
				}
			}
			return UN;
		}
		case VENDOR_AMD:
		{
			if (strstr(brand, "Opteron")) {
				if (strstr(brand, "Dual Core")) return OptiD;
				return OptiS;
			}
			if (strstr(brand, "Athlon(tm) 64 FX")) return A64FX;
			if (strstr(brand, "Athlon(tm) FX")) return AFX;
			if (strstr(brand, "Athlon(tm) 64 FX")) return A64FX;
			if (strstr(brand, "Athlon(tm) 64")) {
				if (strstr(brand, "Dual Core")) return AX2_0;
				return A64_0;
			}
			if (strstr(brand, "Turion(tm)")) return T64_0;
			if (strstr(brand, "Sempron(tm)")) return S64_0;
			
			if (strstr(brand, "mobile") || strstr(brand, "Mobile")) {
				if (strstr(brand, "Athlon(tm) XP-M (LV)")) return ML;
				if (strstr(brand, "Athlon(tm) XP")) return MX;
				if (strstr(brand, "Athlon")) return MA;
				if (strstr(brand, "Duron")) return MD;
			} else {
				if (strstr(brand, "Athlon(tm) XP")) return dX;
				if (strstr(brand, "Athlon(tm) MP")) return sA;
				if (strstr(brand, "Duron")) return dD;
				if (strstr(brand, "Athlon")) return dA;
			}
			
			return UN;
		}
		default: return UN;
	}
}

static bool fms_matches(int check_family, int check_model, int check_stepping)
{
		int res[4];
		CPUID(1, res);
		int r = res[0];
		int stepping    = r & 0xf; r >>= 4;
		int model       = r & 0xf; r >>= 4;
		int family      = r & 0xf; r >>= 8;
		if (check_family != family) return false;
		if (check_model != -1 && check_model != model) return false;
		if (check_stepping != -1 && check_stepping != stepping) return false;
		return true;
}

static bool is_xeon_mp(void)
{
	// Explicit check for processor; used for the corner case 0x49:
	// on Xeon MP (family 0xf, model 0x6) -- cache-id 0x49 means: 4MB L3
	// on other CPUs (conroe et al) -- cache-id 0x49 means: 4MB L2
	return fms_matches(0xf, 0x6, -1);
}

int get_cache_size(void)
{
	int vendor = cpu_vendor();
	if (vendor == VENDOR_INTEL) {
		bool info[256];
		fill_cache_info(info);
		if (info[0x39] || info[0x3b] || info[0x41] || info[0x79]) return 128;
		if (info[0x3a]) return 192;
		if (info[0x3c] || info[0x42] || info[0x7a] || info[0x7e] || info[0x82]) return 256;
		if (info[0x3d]) return 384;
		if (info[0x3e] || info[0x43] || info[0x7b] || info[0x7f] || info[0x83] || info[0x86]) return 512;
		if (info[0x44] || info[0x78] || info[0x7c] || info[0x84] || info[0x87]) return 1024;
		if (info[0x45] || info[0x7d] || info[0x85] || info[0x88]) return 2048;
		if (info[0x49] && !is_xeon_mp()) return 4096;
		if (info[0x4e]) return 6144;
		if (info[0x40])	return 0;
	}
	if (vendor == VENDOR_AMD) {
		int r[4];
		CPUID(0x80000000, r);
		if ((unsigned) r[0] >= 0x80000006) {
			CPUID(0x80000006, r);
			return (r[2] >> 16) & 0xffff;
		}
	}
	return -1;
}

static bool has_L3_cache(void)
{
	bool info[256];
	fill_cache_info(info);
	int all[] = { 0x22, 0x23, 0x25, 0x29, 0x46, 0x47, 0x4a, 0x4b, 0x4c, 0x4d, 0x88, 0x89, 0x8a, 0x8d };
	for (unsigned i = 0; i < sizeof(all)/sizeof(all[0]); i++)
		if (info[all[i]]) return true;
	if (all[0x49] && is_xeon_mp()) return true;
	return false;
}

static code_t get_cpu_code_phase2(int vendor, code_t code)
{
	int cores = 1, hyperthreads = 1;
	int logical_cpus = -1, num_cores = -1;
	
	int r[4];
	int max_cpuid, max_ext_cpuid;
	CPUID(0, r);
	max_cpuid = r[0];
	CPUID(0x80000000, r);
	max_ext_cpuid = r[0];
	if (max_cpuid >= 1) {
		CPUID(1, r);
		logical_cpus = (r[1] >> 16) & 0xff;
		if (vendor == VENDOR_INTEL && max_cpuid >= 4) {
			CPUID(4, r);
			num_cores = 1 + ((r[0] >> 26) && 0x3f);
		} else if (vendor == VENDOR_AMD && (unsigned) max_ext_cpuid >= 0x80000008) {
			CPUID(0x80000008, r);
			num_cores = 1 + (r[2] & 0xff);
		}
	}
	if (simulator) {
		num_cores = sinfo.num_cores;
		logical_cpus = sinfo.logical_cpus;
	}
	if (num_cores != -1 && logical_cpus != -1) {
		if (num_cores > 1) {
			if (logical_cpus == num_cores) {
				cores        = num_cores;
				hyperthreads = 1;
			} else {
				cores        = num_cores;
				hyperthreads = logical_cpus / num_cores;
			}
		} else {
			cores        = 1;
			hyperthreads = (logical_cpus >= 2 ? logical_cpus : 2);
		}
	}
	
	int cache_size = get_cache_size();
	if (simulator)
		cache_size = sinfo.cache_size;
	
	// Specialize for intel:
	if (code == dc) {
		switch (cores) {
			case 1: break;
			case 2: code = Dc; break;
			case 4: code = Qc; break;
			default: code = Wc; break;
		}
	}

	// Specialize other Intels by cache:
	if (code == sX && has_L3_cache()) code = sI; // recognize Nocona<->Irwindale
	if (code == sM && has_L3_cache()) code = sP; // recognize Cranford<->Potomac
	if (code == Dc && cache_size == 2048) code = Da; // recognize Conroe<->Allendale
	
	// Specialize AMDs:
	if (code == dA && cache_size == 512) code = dm; // recognize Manchester E6<->Toledo
	if (code == dX && cache_size == 512) code = dB; // recognize Barton<->Thorton
	if (code == A64_0 && cache_size > 512) code = A64_1; // recognize A64s with > 512K cache
	if (code == S64_0 && cache_size > 128) code = S64_1; // recognize A64 semprons with > 128K cache
	if (code == T64_0 && cache_size > 512) code = T64_1; // recognize Turions with > 512K cache
	if (code == AX2_0 && cache_size > 512) code = AX2_1; // recognize A64 X2s with > 512K cache
	
	return code;
}

static code_t get_cpu_code(int vendor)
{
	code_t temp = get_cpu_code_phase1(vendor);
	return get_cpu_code_phase2(vendor, temp);
}

const char *identify_cpu(void)
{
	int num_entries;
	const MatchEntry *cpudb = NULL;
	simulator = simulator_readinfo("simulator.txt");

	int vendor = cpu_vendor();
	if (simulator) 
		vendor = sinfo.vendor;

	switch (vendor) {
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
	code_t code;
	
	int r = res1[0];
	stepping    = r & 0xf; r >>= 4;
	model       = r & 0xf; r >>= 4;
	family      = r & 0xf; r >>= 8;
	ext_model   = r & 0xf; r >>= 4;
	ext_family  = r & 0xff;
	//
	if (simulator) {
		family     = sinfo.family;
		model      = sinfo.model;
		stepping   = sinfo.stepping;
		ext_family = sinfo.ext_family;
		ext_model  = sinfo.ext_model;
	}
	//
	code = get_cpu_code(vendor);
	
	// we have what we need, try matching now.
	int bestscore = -1;
	int bestindex = 0;
	
	
	for (int i = 0; i < num_entries; i++) {
		int t = score(cpudb[i], family, model, stepping, ext_model, ext_family, code);
		if (t > bestscore) {
			bestscore = t;
			bestindex = i;
		}
	}
	return cpudb[bestindex].name;
}

int get_code(void)
{
	int vendor = cpu_vendor();
	if (vendor == VENDOR_INTEL || vendor == VENDOR_AMD)
		return (int) get_cpu_code(vendor);
	return -2;
}
