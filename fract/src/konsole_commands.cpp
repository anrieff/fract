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
#include "cvar.h"
#include "cvars.h"
#include "konsole_commands.h"
#include "konsole.h"
#include "cpu.h"
#include "threads.h"
#include "string_array.h"

int cmd_exit(int argc, char **argv)
{
	if (!strcmp(QUICKHELP_STRING, argv[1])) {
		konsole.write("quits the program\n");
		return 0;
	}
	konsole.exit();
	return 0;
}

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
	konsole.write("   Detected # of CPUs: %d\n", get_processor_count());
	konsole.write("   Used CPUs: %d\n", cpu.count);
	konsole.write("   MMX: %s\n", cpu.mmx?"present":"absent");
	konsole.write("   MMX2: %s\n", cpu.mmx2?"present":"absent");
	konsole.write("   SSE: %s\n", cpu.sse?"present":"absent");
	konsole.write("   SSE2: %s\n", cpu.sse2?"present":"absent");
	konsole.write("   Speed: %d MHz\n", cpu.speed());
	konsole.write("   Vendor id: %s\n", cpu.vendor_id);
	return 0;
}

static void list_things(bool list_commands, bool list_cvars, const char *filter)
{
	StringArray arr;
	
	if (list_commands) {
		int n = cmdcount();
		for (int i = 0; i < n; i++)
			arr.add(allcommands[i].cmdname, NULL);
	}
	if (list_cvars) {
		for (CVar *cvar = cvars_start(); cvar; cvar = cvars_iter()) {
			arr.add(cvar->name, cvar);
		}
	}
	if (filter[0]) arr.filter(filter);
	if (!arr.size()) {
		konsole.write("(no matches)\n");
		return;
	}
	int maxlen = arr.get_max_length();
	arr.sort();
	
	char *exec = (char *) alloca(5 + maxlen);
	
	for (int i = 0; i < arr.size(); i++) {
		bool iscmd = arr.get_traits(i) == NULL;
		
		konsole.set_color(iscmd ? 0xffffcc : 0xccccff);
		konsole.write("%s", arr.get_string(i));
		konsole.set_default_color();
		for (int j = 0; j < maxlen - (int) strlen(arr.get_string(i)); j++)
			konsole.write(" ");
		konsole.write(" - ");
		if (iscmd) {
			strcpy(exec, arr.get_string(i));
			strcat(exec, " ");
			strcat(exec, QUICKHELP_STRING);
			konsole.execute(exec);
		} else {
			const CVar *cvar = reinterpret_cast<const CVar *>(arr.get_traits(i));
			konsole.set_color(0x777777);
			konsole.write("(%4s)", cvar->get_type_name());
			konsole.set_default_color();
			konsole.write("  - %s\n", cvar->description);
		}
		
	}
}

int cmd_cmdlist(int argc, char **argv)
{
	if (!strcmp(QUICKHELP_STRING, argv[1])) {
		konsole.write("lists all commands\n");
		return 0;
	}
	list_things(true, false, argc > 1 ? argv[1] : "");
	return 0;
}

int cmd_cvarlist(int argc, char **argv)
{
	if (!strcmp(QUICKHELP_STRING, argv[1])) {
		konsole.write("lists all cvars\n");
		return 0;
	}
	list_things(false, true, argc > 1 ? argv[1] : "");
	return 0;
}

int cmd_list(int argc, char **argv) 
{
	if (!strcmp(QUICKHELP_STRING, argv[1])) {
		konsole.write("lists all cvars and commands\n");
		return 0;
	}
	list_things(true, true, argc > 1 ? argv[1] : "");
	return 0;
}

int cmd_fancy(int argc, char **argv)
{
	if (!strcmp(QUICKHELP_STRING, argv[1])) {
		konsole.write("makes console background look fancy\n");
		return 0;
	}
	konsole.fancy();
	return 0;
}
