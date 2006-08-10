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
	
	// Pentia
	{ 5, -1, -1, -1, -1, "Unknown Pentium" },
	{ 5,  0, -1, -1, -1, "Pentium A-Step" },
	{ 5,  1, -1, -1, -1, "Pentium 1 Classic" },
	{ 5,  2, -1, -1, -1, "Pentium " },
	{ 5,  3, -1, -1, -1, "Pentium " },
	{ 5,  4, -1, -1, -1, "Pentium " },
	{ 5,  7, -1, -1, -1, "Pentium " },
	{ 5,  8, -1, -1, -1, "Pentium " },

};

const MatchEntry cpudb_amd[] = {
	{ -1, -1, -1, -1, -1, "Unknown AMD CPU" },
};
