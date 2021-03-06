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

#include <wx/wx.h>
#include <stdio.h>
#include <string.h>

#define SSE2_MASK  0x04000000

#ifdef __GNUC__
#	ifdef __x86_64__
bool have_SSE2(void) 
{
	return true; // all amd64-capable CPUs have SSE2
}
#	else
bool have_SSE2(void) 
{
	int CPUZ=0, CPUEXT=0, ECS=0;
 	__asm __volatile (
	
		 "			push	%%ebx\n"
		 "          mov     $1,    %%eax \n"
		 "          cpuid \n"
		 "			pop		%%ebx\n"
		 "			push	%%ebx\n"
		 "          mov     %%edx,  %0\n"
		 "			mov     $0x80000000, %%eax\n"
		 "          cpuid\n"
		 "			pop		%%ebx\n"
		 "			push	%%ebx\n"
		 "          cmp     $0x80000001, %%eax\n"
		 "          jb      not_supported\n"

		 "          mov     $0x80000001, %%eax\n"
		 "          cpuid\n"
		 "			pop		%%ebx\n"
		 "          mov     %%edx,       %1\n"
		 "          movl     $1,          %2\n"
		 "          jmp     ende\n"

		 "          not_supported:\n"
		 "          movl     $0,          %2\n"
		 "          ende:\n"
	:"=m"(CPUZ), "=m"(CPUEXT), "=m"(ECS)
	:
	:"eax", "ecx", "edx"
		  );

	return (CPUZ&SSE2_MASK?true:false);
}
#endif // __x86_64__
#else
#ifdef _MSC_VER
bool have_SSE2(void)
{
	int CPUZ=0, CPUEXT=0, ECS=0;

	__asm {

 		mov		eax,		1
 		cpuid
 		mov		CPUZ,		edx
 		mov		eax,		0x80000000
 		cpuid
 		cmp		eax,		0x80000001
 		jb		not_supported

 		mov		eax,		0x80000001
 		cpuid
 		mov		CPUEXT,		edx
 		mov		ECS,		1
 		jmp		ende

	not_supported:
 		mov		ECS,		0
	ende:

	}

	return (CPUZ&SSE2_MASK?true:false);
}
#else
#error unsupported compiler
#endif //_MSC_VER
#endif //__GNUC__

wxString get_version(void)
{
	FILE *f;
	f = fopen("_VERSION_.txt", "rt");
	if (!f) return "<unknown>";
	char line[100];
	fgets(line, 99, f);
	fclose(f);
	if(strlen(line) && line[strlen(line)-1]=='\n') line[strlen(line)-1] = 0;
	return line;
}
