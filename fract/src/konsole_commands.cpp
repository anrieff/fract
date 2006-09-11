/***************************************************************************
 *   Copyright (C) 2006 by Veselin Georgiev                                *
 *   anrieff@mailg.com (convert to gmail)                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/


#include <string.h>
#include "konsole_commands.h"
#include "konsole.h"
#include "cpu.h"

int cmd_help(int argc, char **argv)
{
	if (!strcmp(QUICKHELP_STRING, argv[1])) {
		konsole.write("dispays help about commands or cvars\n");
		return 0;
	}
	konsole.write("Like every good 3D thing, fract has a console\n");
	konsole.write("It is used to execute commands or change\n");
	konsole.write("console variables (cvars)\n");
	konsole.write("Typing the cvar name alone gives its value\n");
	konsole.write("Cvar, followed by some value assigns it to the cvar\n");
	konsole.write("To get a full list of cvars, type `cvarlist'\n");
	konsole.write("Commands are more complicated. Type `help <command>'\n");
	konsole.write("to get more information\n");
	return 0;
}

int cmd_cpu(int argc, char **argv)
{
	if (!strcmp(QUICKHELP_STRING, argv[1])) {
		konsole.write("displays info about the CPU\n");
		return 0;
	}
	konsole.write("CPU Info:\n");
	konsole.write("   MMX: %s\n", cpu.mmx?"present":"absent");
	konsole.write("   MMX2: %s\n", cpu.mmx2?"present":"absent");
	konsole.write("   SSE: %s\n", cpu.sse?"present":"absent");
	konsole.write("   SSE2: %s\n", cpu.sse2?"present":"absent");
	konsole.write("   Speed: %d MHz\n", cpu.speed());
	konsole.write("   Vendor id: %s\n", cpu.vendor_id);
	return 0;
}

int cmd_testarg(int argc, char **argv)
{
	if (!strcmp(QUICKHELP_STRING, argv[1])) {
		konsole.write("tests console capabilities\n");
		return 0;
	}
	konsole.write("%d arguments:\n", argc);
	for (int i = 0; i < argc; i++) 
		konsole.write("arg[%d] = `%s'\n", i, argv[i]);
	return 0;
}

int cmd_cmdlist(int argc, char **argv)
{
	if (!strcmp(QUICKHELP_STRING, argv[1])) {
		konsole.write("lists all commands\n");
		return 0;
	}
	const char *lastc = "";
	int n = cmdcount();
	int maxcmdlen = 0;
	
	for (int i = 0; i < n; i++)
		if ((int) strlen(allcommands[i].cmdname) > maxcmdlen)
			maxcmdlen = strlen(allcommands[i].cmdname);
	char *exec = (char *) alloca(5 + maxcmdlen);
	while (1) {
		const char *mincmd = NULL;
		for (int i = 0; i < n; i++) {
			if (strcmp(allcommands[i].cmdname, lastc) > 0 ) {
				if (!mincmd) mincmd = allcommands[i].cmdname;
				else if (strcmp(allcommands[i].cmdname, mincmd)< 0)
					mincmd = allcommands[i].cmdname;
			}
		}
		if (!mincmd) break;
		lastc = mincmd;
		konsole.set_color(0xffffcc);
		konsole.write("%s", mincmd);
		konsole.set_default_color();
		for (int j = 0; j < maxcmdlen - (int) strlen(mincmd); j++)
			konsole.write(" ");
		konsole.write(" - ");
		strcpy(exec, mincmd);
		strcat(exec, " ");
		strcat(exec, QUICKHELP_STRING);
		konsole.execute(exec);
	}
	return 0;
}
