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


/*
** d? = think "desktop"
** s? = think "server" (multiprocessor)
** M? = think "mobile"
** D? = think "dual core"
**
** ?P = think Pentium
** ?X = think Xeon
** ?M = think Xeon MP
** ?C = think Celeron
** ?c = think Core
** ?A = think Athlon
** ?D = think Duron
** ?S = think Sempron
** ?O = think Opteron
** ?T = think Turion
*/
enum code_t {
       /* ==========Intel==========  ============AMD============    Transmeta */
   NS, /*        Not-specified                                                */
   UN, /*        Unknown                                                      */
   dP, /*        Pentium                                                      */
   MP, /* Mobile Pentium                                                      */
   sX, /*        Xeon                                                         */
   sI, /*        Xeon (Irwindale)                                             */
   s7, /*        Xeon (Series 7000)                                           */
   sM, /*        Xeon MP                                                      */
   sP, /*        Xeon MP (Potomac)                                            */
   MM, /* Mobile Pentium M                                                    */
   dC, /*        Celeron                                                      */
   MC, /* Mobile Celeron                                                      */
   nC, /* not    Celeron                                                      */
   dc, /*        Core Solo                                                    */
   Dc, /*        Core Duo                                                     */
   Da, /*        Core Duo (Allendale)                                         */
   Qc, /*        Kentsfield (Quad-core CPU)                                   */
   Wc, /*        More than quad-core CPU                                      */
   dO, /*                                      Opteron                        */
   d8, /*                                      Opteron (8xx)                  */
   dX, /*                                      Athlon XP                      */
   dt, /*                                      Athlon XP (Thorton)            */
   MX, /*                            mobile    Athlon XP-M                    */
   ML, /*                            mobile    Athlon XP-M (LV)               */
   dA, /*                                      Athlon(64)                     */
   dm, /*                                      Athlon(64) (Manchester)        */
   sA, /*                                      Athlon MP                      */
   MA, /*                            mobile    Athlon(64)                     */
   dF, /*                                      Athlon 64 FX                   */
   dD, /*                                      Duron                          */
   sD, /*        Pentium D                     Duron MP                       */
   MD, /*                            mobile    Duron                          */
   dS, /*                                      Sempron                        */
   MS, /*                            mobile    Sempron                        */
   DO, /*                            Dual Core Opteron                        */
   D8, /*                            Dual Core Opteron (8xx)                  */
   MT, /*                            mobile    Turion                         */
   t2, /*                                                           TMx200    */
   t4, /*                                                           TMx400    */
   t5, /*                                                           TMx500    */
   t6, /*                                                           TMx600    */
   t8, /*                                                           TMx800    */
};

 
struct MatchEntry {
	int family, model, stepping, ext_family, ext_model;
	code_t code;
	char name[32];
};

const MatchEntry cpudb_intel[] = {
	{ -1, -1, -1, -1, -1, NS, "Unknown Intel CPU" },
	
	// i486
	{ 4, -1, -1, -1, -1, NS, "Unknown i486" },
	{ 4,  0, -1, -1, -1, NS, "i486 DX-25/33" },
	{ 4,  1, -1, -1, -1, NS, "i486 DX-50" },
	{ 4,  2, -1, -1, -1, NS, "i486 SX" },
	{ 4,  3, -1, -1, -1, NS, "i486 DX2" },
	{ 4,  4, -1, -1, -1, NS, "i486 SL" },
	{ 4,  5, -1, -1, -1, NS, "i486 SX2" },
	{ 4,  7, -1, -1, -1, NS, "i486 DX2 WriteBack" },
	{ 4,  8, -1, -1, -1, NS, "i486 DX4" },
	{ 4,  9, -1, -1, -1, NS, "i486 DX4 WriteBack" },
	
	/* All Pentia: */
	// Pentium 1
	{ 5, -1, -1, -1, -1, NS, "Unknown Pentium" },
	{ 5,  0, -1, -1, -1, NS, "Pentium A-Step" },
	{ 5,  1, -1, -1, -1, NS, "Pentium 1 (0.8u)" },
	{ 5,  2, -1, -1, -1, NS, "Pentium 1 (0.35u)" },
	{ 5,  3, -1, -1, -1, NS, "Pentium OverDrive" },
	{ 5,  4, -1, -1, -1, NS, "Pentium 1 (0.35u)" },
	{ 5,  7, -1, -1, -1, NS, "Pentium 1 (0.35u)" },
	{ 5,  8, -1, -1, -1, NS, "Pentium MMX (0.25u)" },
	
	// Pentium 2 / 3 / M / Conroe / whatsnext - all P6 based.
	{ 6, -1, -1, -1, -1, NS, "Unknown P6" },
	{ 6,  0, -1, -1, -1, NS, "Pentium Pro" },
	{ 6,  1, -1, -1, -1, NS, "Pentium Pro" },
	{ 6,  3, -1, -1, -1, NS, "Pentium II (Klamath)" },
	{ 6,  5, -1, -1, -1, NS, "Pentium II (Deschutes)" },
	{ 6,  6, -1, -1, -1, NS, "Pentium II (Dixon)" },
	
	{ 6,  3, -1, -1, -1, sX, "P-II Xeon" },
	{ 6,  5, -1, -1, -1, sX, "P-II Xeon" },
	{ 6,  6, -1, -1, -1, sX, "P-II Xeon" },
		
	{ 6,  5, -1, -1, -1, dC, "P-II Celeron (no L2)" },
	{ 6,  6, -1, -1, -1, dC, "P-II Celeron (128K)" },
	
	// ////////////////////////////////////////////////// //
	
	{ 6,  7, -1, -1, -1, NS, "Pentium III (Katmai)" },
	{ 6,  8, -1, -1, -1, NS, "Pentium III (Coppermine)" },
	{ 6, 10, -1, -1, -1, NS, "Pentium III (Coppermine)" },
	{ 6, 11, -1, -1, -1, NS, "Pentium III (Tualatin)" },
	
	{ 6,  7, -1, -1, -1, sX, "P-III Xeon" },
	{ 6,  8, -1, -1, -1, sX, "P-III Xeon" },
	{ 6, 10, -1, -1, -1, sX, "P-III Xeon" },
	{ 6, 11, -1, -1, -1, sX, "P-III Xeon" },
	
	{ 6,  7, -1, -1, -1, dC, "P-III Celeron" },
	{ 6,  8, -1, -1, -1, dC, "P-III Celeron" },
	{ 6, 10, -1, -1, -1, dC, "P-III Celeron" },
	{ 6, 11, -1, -1, -1, dC, "P-III Celeron" },
	
	// ////////////////////////////////////////////////// //
	
	{ 6,  9, -1, -1, -1, NS, "Unknown Pentium M" },
	{ 6,  9, -1, -1, -1, NS, "Unknown Pentium M" },
	{ 6,  9, -1, -1, -1, dP, "Pentium M (Banias)" },
	{ 6,  9, -1, -1, -1, dC, "Celeron M" },
	{ 6, 13, -1, -1, -1, dP, "Pentium M (Dothan)" },
	{ 6, 13, -1, -1, -1, dC, "Celeron M" },
	
	// ////////////////////////////////////////////////// //
	
	{ 6, 14, -1, -1, -1, NS, "Unknown Yonah" },
	{ 6, 14, -1, -1, -1, dc, "Yonah (Core Solo)" },
	{ 6, 14, -1, -1, -1, Dc, "Yonah (Core Duo)" },
	{ 6, 14, -1, -1, -1, sX, "Xeon LV" },
	{ 6, 14, -1, -1, -1, dC, "Yonah (Core Solo)" },
	
	{ 6, 15, -1, -1, -1, NS, "Unknown Core 2" },
	{ 6, 15, -1, -1, -1, Dc, "Conroe (Core 2 Duo)" },
	{ 6, 15, -1, -1, -1, Qc, "Kentsfield" },
	{ 6, 15, -1, -1, -1, Wc, "More than quad-core" },
	{ 6, 15, -1, -1, -1, Da, "Allendale (Core 2 Duo)" },
	
	{ 6, 16, -1, -1, -1, NS, "Unknown Core ?" }, // future ones
	{ 6, 17, -1, -1, -1, NS, "Unknown Core ?" }, // future ones
	{ 6, 16, -1, -1, -1, Wc, "More than quad-core" }, // future ones
	{ 6, 17, -1, -1, -1, Wc, "More than quad-core" }, // future ones
	
	// Itaniums
	{  7, -1, -1, -1, -1, NS, "Itanium" },
	{ 15, -1, -1,  1, -1, NS, "Itanium 2" },
	
	// Netburst based (Pentium 4 and later)
	// classic P4s
	{ 15, -1, -1, 0, -1, NS, "Unknown Pentium 4" },
	{ 15, -1, -1, 0, -1, dC, "Unknown P-4 Celeron" },
	{ 15, -1, -1, 0, -1, sX, "Unknown Xeon" },
	
	{ 15,  0, -1, 0, -1, NS, "Pentium 4 (Willamette)" },
	{ 15,  1, -1, 0, -1, NS, "Pentium 4 (Willamette)" },
	{ 15,  2, -1, 0, -1, NS, "Pentium 4 (Northwood)" },
	{ 15,  3, -1, 0, -1, NS, "Pentium 4 (Prescott)" },
	{ 15,  4, -1, 0, -1, NS, "Pentium 4 (Prescott)" },
	
	// server CPUs
	{ 15,  0, -1, 0, -1, sX, "Xeon (Foster)" },
	{ 15,  1, -1, 0, -1, sX, "Xeon (Foster)" },
	{ 15,  2, -1, 0, -1, sX, "Xeon (Prestonia)" },
	{ 15,  2, -1, 0, -1, sM, "Xeon (Gallatin)" },
	{ 15,  3, -1, 0, -1, sX, "Xeon (Nocona)" },
	{ 15,  4, -1, 0, -1, sX, "Xeon (Nocona)" },
	{ 15,  4, -1, 0, -1, sI, "Xeon (Irwindale)" },
	{ 15,  4, -1, 0, -1, sM, "Xeon (Cranford)" },
	{ 15,  4, -1, 0, -1, sP, "Xeon (Potomac)" },
	{ 15,  6, -1, 0, -1, sX, "Xeon 5000" },
	
	// Pentium Ds
	{ 15,  4,  4, 0, -1, NS, "Pentium D" },
	{ 15,  4, -1, 0, -1, dD, "Pentium D" },
	{ 15,  4,  7, 0, -1, NS, "Pentium D" },
	{ 15,  6, -1, 0, -1, dD, "Pentium D" },

	// Celeron and Celeron Ds
	{ 15,  1, -1, 0, -1, dC, "P-4 Celeron (128K)" }, 
	{ 15,  2, -1, 0, -1, dC, "P-4 Celeron (128K)" },
	{ 15,  3, -1, 0, -1, dC, "Celeron D" },
	{ 15,  4, -1, 0, -1, dC, "Celeron D" },
	{ 15,  6, -1, 0, -1, dC, "Celeron D" },
	
	
};

const MatchEntry cpudb_amd[] = {
	{ -1, -1, -1, -1, -1, NS, "Unknown AMD CPU" },
	
	// 486 and the likes
	{  4, -1, -1, -1, -1, NS, "Unknown AMD 486" },
	{  4,  3, -1, -1, -1, NS, "AMD 486DX2" },
	{  4,  7, -1, -1, -1, NS, "AMD 486DX2WB" },
	{  4,  8, -1, -1, -1, NS, "AMD 486DX4" },
	{  4,  9, -1, -1, -1, NS, "AMD 486DX4WB" },
	
	// Pentia clones
	{  5, -1, -1, -1, -1, NS, "Unknown AMD 586" },
	{  5,  0, -1, -1, -1, NS, "K5" },
	{  5,  1, -1, -1, -1, NS, "K5" },
	{  5,  2, -1, -1, -1, NS, "K5" },
	{  5,  3, -1, -1, -1, NS, "K5" },
	
	// The K6
	{  5,  6, -1, -1, -1, NS, "K6" },
	{  5,  7, -1, -1, -1, NS, "K6" },
	
	{  5,  8, -1, -1, -1, NS, "K6-2" },
	{  5,  9, -1, -1, -1, NS, "K6-III" },
	{  5, 13, -1, -1, -1, NS, "K6-2+" },
	
	// Athlon et al.
	{  6,  1, -1, -1, -1, NS, "Athlon (Slot-A)" },
	{  6,  2, -1, -1, -1, NS, "Athlon (Slot-A)" },
	{  6,  3, -1, -1, -1, NS, "Duron (Spitfire)" },
	{  6,  4, -1, -1, -1, NS, "Athlon (ThunderBird)" },
	
	{  6,  6, -1, -1, -1, NS, "Unknown Athlon" },
	{  6,  6, -1, -1, -1, dA, "Athlon (Palomino)" },
	{  6,  6, -1, -1, -1, sA, "Athlon MP (Palomino)" },
	{  6,  6, -1, -1, -1, dD, "Duron (Palomino)" },
	{  6,  6, -1, -1, -1, dX, "Athlon XP" },
	
	{  6,  7, -1, -1, -1, NS, "Unknown Athlon XP" },
	{  6,  7, -1, -1, -1, dD, "Duron (Morgan)" },
	
	{  6,  8, -1, -1, -1, NS, "Athlon XP" },
	{  6,  8, -1, -1, -1, dA, "Athlon XP" },
	{  6,  8, -1, -1, -1, dX, "Athlon XP" },
	{  6,  8, -1, -1, -1, dD, "Duron (Applebred)" },
	{  6,  8, -1, -1, -1, dS, "Sempron (Thoroughbred)" },
	{  6,  8, -1, -1, -1, sA, "Athlon MP (Thoroughbred)" },
	{  6,  8, -1, -1, -1, MX, "Mobile Athlon (Thorough.)" },
	{  6,  8, -1, -1, -1, ML, "Mobile Athlon (Thorough.)" },
	
	{  6, 10, -1, -1, -1, NS, "Athlon XP (Barton)" },
	{  6, 10, -1, -1, -1, dA, "Athlon XP (Barton)" },
	{  6, 10, -1, -1, -1, dX, "Athlon XP (Barton)" },
	{  6, 10, -1, -1, -1, dS, "Sempron (Barton)" },
	{  6, 10, -1, -1, -1, dt, "Athlon XP" }, 
	{  6, 10, -1, -1, -1, sA, "Athlon MP (Barton)" },
	{  6, 10, -1, -1, -1, MX, "Mobile Athlon (Barton)" },
	{  6, 10, -1, -1, -1, ML, "Mobile Athlon (Barton)" },
	// ^^ Actually, Thorton, but it's equivallent to Thoroughbred
	
	// K8 Arch
	{ 15, -1, -1,  0, -1, NS, "Unknown K8" },
	{ 15, -1, -1,  1, -1, NS, "Unknown K9" },
	
	{ 15, -1, -1,  0,  0, NS, "" },
	{ 15,  0, -1,  0,  0, NS, "" },
	{ 15,  0, -1,  0,  0, NS, "" },
	{ 15,  0, -1,  0,  0, NS, "" },
	{ 15,  0, -1,  0,  0, NS, "" },
	{ 15,  0, -1,  0,  0, NS, "" },
	{ 15,  0, -1,  0,  0, NS, "" },
	{ 15,  0, -1,  0,  0, NS, "" },
	{ 15,  0, -1,  0,  0, NS, "" },
	{ 15,  0, -1,  0,  0, NS, "" },
	{ 15,  0, -1,  0,  0, NS, "" },
	{ 15,  0, -1,  0,  0, NS, "" },
	{ 15,  0, -1,  0,  0, NS, "" },
	{ 15,  0, -1,  0,  0, NS, "" },
	{ 15,  0, -1,  0,  0, NS, "" },
	{ 15,  0, -1,  0,  0, NS, "" },
	{ 15,  0, -1,  0,  0, NS, "" },
	{ 15,  0, -1,  0,  0, NS, "" },
	{ 15,  0, -1,  0,  0, NS, "" },
	{ 15,  0, -1,  0,  0, NS, "" },
	{ 15,  0, -1,  0,  0, NS, "" },
	{ 15,  0, -1,  0,  0, NS, "" },
	{ 15,  0, -1,  0,  0, NS, "" },
	{ 15,  0, -1,  0,  0, NS, "" },
	{ 15,  0, -1,  0,  0, NS, "" },
	{ 15,  0, -1,  0,  0, NS, "" },
	{ 15,  0, -1,  0,  0, NS, "" },
	
};
