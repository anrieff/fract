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

const char *cpu_list[] = {
		"-----Intel-----",
		"Pentium",
		"Pentium MMX",
		"Pentium Pro (256K)",
		"Pentium Pro (512K)",
		"Pentium Pro (1024K)",
		"Pentium II",
		"P-II Celeron (no L2)",
		"P-II Celeron (128K)",
		"P-II Xeon",
		"Pentium III (Katmai)",
		"Pentium III (Coppermine)",
		"Pentium III-S",
		"P-III Celeron",
		"P-III Xeon",
		"Pentium 4 (Willamate)",
		"P-4 Celeron (128K)" ,
		"Pentium 4 (Northwood)",
		"Pentium 4 (Prescott)",
		"Pentium 6xx",
		"Pentium D",
		"Celeron D" ,
		"Pentium M (Banias)" ,
		"Pentium M (Dothan)",
		"Celeron M",
		"Xeon (Foster)",
		"Xeon (Prestonia)",
		"Xeon (Gallatin)",
		"Xeon (Nocona)",
		
		
		"---AMD---",
		"K5",
		"K6",
		"K6-2",
		"K6-III",
		"K6-2+",
		"K6-III+",
		"Athlon (Slot-A)",
		"Athlon (ThunderBird)",
		"Duron (Spitfire)",
		"Duron (Morgan)",
		"Athlon XP",
		"Athlon XP (Barton)",
		"Sempron (Thoroughbred)",
		"Opteron",
		"Opteron (Dual Core)",
		"Athlon FX",
		"Athlon 64 (512K)",
		"Athlon 64 (1024K)",
		"A64 Sempron (256K)",
		"A64 Sempron (128K)",
		"Turion 64 (512K)",
		"Turion 64 (1024K",
		"Athlon 64 X2 (512K)",
		"Athlon 64 X2 (1024K)",
		
		"-----Cyrix-----",
		"Cx5x86",
		"6x86",
	
		"-----VIA-----",
		"C3",
		"C7",
	
		"-----NexGen-----",
		"Nx586",
	
		"-----Transmeta-----",
		"Crusoe",
		"Efficeon",
	
	
	// END OF LIST ****	
	
		"-----Unknown-----",
		""
	
};


const char default_cpu[] = "-----Unknown-----";

static int cpu_list_size(void)
{
	int i = 0;
	while (cpu_list[i][0]) i++;
	return i;
}
