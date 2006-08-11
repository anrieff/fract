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

 
struct MatchEntry {
	int family, model, stepping, ext_family, ext_model;
	char *name;
};

const MatchEntry cpudb_intel[] = {
	{ -1, -1, -1, -1, -1, "Unknown Intel CPU" },
	
	// i486
	{ 4, -1, -1, -1, -1, "Unknown i486" },
	{ 4,  0, -1, -1, -1, "i486 DX-25/33" },
	{ 4,  1, -1, -1, -1, "i486 DX-50" },
	{ 4,  2, -1, -1, -1, "i486 SX" },
	{ 4,  3, -1, -1, -1, "i486 DX2" },
	{ 4,  4, -1, -1, -1, "i486 SL" },
	{ 4,  5, -1, -1, -1, "i486 SX2" },
	{ 4,  7, -1, -1, -1, "i486 DX2 WriteBack" },
	{ 4,  8, -1, -1, -1, "i486 DX4" },
	{ 4,  9, -1, -1, -1, "i486 DX4 WriteBack" },
	
	/* All Pentia: */
	// Pentium 1
	{ 5, -1, -1, -1, -1, "Unknown Pentium" },
	{ 5,  0, -1, -1, -1, "Pentium A-Step" },
	{ 5,  1, -1, -1, -1, "Pentium 1 (0.8u)" },
	{ 5,  2, -1, -1, -1, "Pentium 1 (0.35u)" },
	{ 5,  3, -1, -1, -1, "Pentium OverDrive" },
	{ 5,  4, -1, -1, -1, "Pentium 1 (0.35u)" },
	{ 5,  7, -1, -1, -1, "Pentium 1 (0.35u)" },
	{ 5,  8, -1, -1, -1, "Pentium MMX (0.25u)" },
	
	// Pentium 2 / 3 / M / Conroe / whatsnext - all P6 based.
	{ 6, -1, -1, -1, -1, "Unknown P6" },
	{ 6,  0, -1, -1, -1, "Pentium 2 A-step" },
	{ 6,  1, -1, -1, -1, "Pentium 2 (Klamath)" },
	{ 6,  3, -1, -1, -1, "Pentium 2" },
	{ 6,  5, -1, -1, -1, "Pentium 2 (Deschutes)" },
	{ 6,  6, -1, -1, -1, "Pentium 2 (Dixon)" },
	
	{ 6,  7, -1, -1, -1, "Pentium 3 (Katmai)" },
	{ 6,  8, -1, -1, -1, "Pentium 3 (Coppermine)" },
	{ 6, 10, -1, -1, -1, "Pentium 3 (Coppermine)" },
	{ 6, 11, -1, -1, -1, "Pentium 3 (Tualatin)" },
};

const MatchEntry cpudb_amd[] = {
	{ -1, -1, -1, -1, -1, "Unknown AMD CPU" },
};
