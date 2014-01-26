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

#ifndef __CPUID_H__
#define __CPUID_H__

/**
 * identify_cpu
 * @brief  Returns some (basic) CPU identifier, e.g. "Pentium III (Katmai)"
 * NOTE: Always returns some valid string, even if CPU is not known
 *       (in such cases it will be, e.g. "Unknown Intel CPU")
*/ 
const char *identify_cpu(void);

/**
 * cpu_brand_string
 * @brief  Returns CPU brand string (CPUID FNs 0x8000000[234]) (if supported)
 * @returns If the operation is supported, a pointer to the CPU brand string.
 *          If it is not, the function returns NULL.
*/ 
const char *cpu_brand_string(void);

/**
 * get_cache_size
 * @brief returns the L2 cache size in kilobytes
 * @returns the amount of cache (if available) or -1 for unknown
*/ 
int get_cache_size(void);

int get_code(void);

#endif // __CPUID_H__
