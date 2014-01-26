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

#include <stdio.h>
#include <string.h>
#include "cpuid.h"

static void show_help(void)
{
	printf("showcpu -- identifies your CPU.\n\n");
	printf("Usage: showcpu [--full]\n");
	printf("\t--full   - Display CPU Brand String (if supported)\n\n");
}

static void basic_info(void)
{
	printf("%s\n", identify_cpu());
}

static void full_info(void)
{
	const char *brand = cpu_brand_string();
	printf("CPU brand string is ");
	if (brand) printf("\"%s\".\n", brand);
		else printf("not supported.\n");
	printf("Cache size is ");
	int cache = get_cache_size();
	if (cache == -1) printf("unknown.\n"); 
		else printf("%dKB.\n", cache);
	printf("CPU-specific code is %d\n", get_code());
}

int main(int argc, char **argv)
{
	switch (argc) {
		case 1: basic_info(); break;
		case 2: 
		{
			if (!strcmp(argv[1], "--help")) {
				show_help();
				break;
			}
			if (!strcmp(argv[1], "--full")) {
				basic_info();
				full_info();
				break;
			}
		}
		default:
			printf("Invalid argument(s)!\n");
			show_help();
			break;
	}
	return 0;
}
