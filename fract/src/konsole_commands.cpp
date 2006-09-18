/***************************************************************************
 *   Copyright (C) 2006 by Veselin Georgiev                                *
 *   anrieff@mailg.com (convert to gmail)                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifdef ACTUALLYDISPLAY
#include <SDL/SDL.h>
#endif

#include <string.h>
#include "cvar.h"
#include "cvars.h"
#include "konsole_commands.h"
#include "konsole.h"
#include "cpu.h"
#include "threads.h"
#include "string_array.h"
#ifdef _WIN32
#include <malloc.h>
#endif

#define QUICK_HELP (argc > 1 && !strcmp(QUICKHELP_STRING, argv[1]))

const char *cmd_quickhelp[] = {
	"exit|quits the program",
	"help|displays help about commands or cvars",
	"cpu|displays info about the CPU",
	"cmdlist|lists all commands",
	"cvarlist|list all cvars",
	"list|list all cvars and commands",
	"fancy|changes console background",
	"title|changes window title (in windowed mode)",
	"inc|increments an integer cvar",
	"mul|multiplies real cvar by some number",
	"toggle|toggles an bool variable",
	
	
	""
};

static const char * get_quickhelp(const char *cmd)
{
	int n = strlen(cmd);
	for (unsigned i = 0; cmd_quickhelp[i][0]; i++) {
		if (!strncmp(cmd, cmd_quickhelp[i], n) && cmd_quickhelp[i][n] == '|')
			return (cmd_quickhelp[i] + n + 1);
	}
	return "(no help available)";
}

int cmd_exit(int argc, char **argv)
{
	konsole.exit();
	return 0;
}

int cmd_help(int argc, char **argv)
{
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
	
	
	for (int i = 0; i < arr.size(); i++) {
		bool iscmd = arr.get_traits(i) == NULL;
		
		konsole.set_color(iscmd ? 0xffffcc : 0xccccff);
		konsole.write("%s", arr.get_string(i));
		konsole.set_default_color();
		for (int j = 0; j < maxlen - (int) strlen(arr.get_string(i)); j++)
			konsole.write(" ");
		konsole.write(" - ");
		if (iscmd) {
			konsole.write("%s\n", get_quickhelp(arr.get_string(i)));
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
	list_things(true, false, argc > 1 ? argv[1] : "");
	return 0;
}

int cmd_cvarlist(int argc, char **argv)
{
	list_things(false, true, argc > 1 ? argv[1] : "");
	return 0;
}

int cmd_list(int argc, char **argv) 
{
	list_things(true, true, argc > 1 ? argv[1] : "");
	return 0;
}

int cmd_fancy(int argc, char **argv)
{
	konsole.fancy();
	return 0;
}

int cmd_title(int argc, char **argv)
{
#ifdef ACTUALLYDISPLAY
	if (argc < 2) {
		char *t, *i;
		SDL_WM_GetCaption(&t, &i);
		konsole.write("Current title is \"%s\"\n", t);
	} else {
		char t[200] = {0};
		for (int i = 1; i < argc; i++)
		{
			if (i > 1) strcat(t, " ");
			strcat(t, argv[i]);
		}
		SDL_WM_SetCaption(t, "");
	}
#endif
	return 0;
}

int cmd_inc(int argc, char **argv)
{
	if (argc != 2) {
		konsole.write("Usage: inc <cvar>\n");
		return 1;
	}
	CVar *cvar = find_cvar_by_name(argv[1]);
	if (!cvar) {
		konsole.write("inc: no such cvar: `%s'\n", argv[1]);
		return 2;
	}
	switch (cvar->type) {
		case CVar::TYPE_INT: cvar->value_int[0]++; break;
		case CVar::TYPE_BOOL: cvar->value_bool[0] = !cvar->value_bool[0]; break;
		case CVar::TYPE_DOUBLE: cvar->value_double[0]++; break;
		default: return 3;
	}
	return 0;
}

int cmd_mul(int argc, char **argv)
{
	if (argc != 3) {
		konsole.write("Usage: mul <cvar> <multiplier>\n");
		return 1;
	}
	double m;
	if (1 != sscanf(argv[2], "%lf", &m)) {
		konsole.write("mul: bad argument: `%s'\n", argv[2]);
		return 2;
	}
	CVar *cvar = find_cvar_by_name(argv[1]);
	if (!cvar) {
		konsole.write("mul: no such cvar: `%s'\n", argv[1]);
		return 2;
	}
	switch (cvar->type) {
		case CVar::TYPE_INT: cvar->value_int[0] = (int) (cvar->value_int[0] * m); break;
		case CVar::TYPE_BOOL: konsole.write("mul: invalid variable type!\n"); return 3;
		case CVar::TYPE_DOUBLE: cvar->value_double[0] *= m; break;
		default: return 4;
	}
	return 0;
}

int cmd_toggle(int argc, char **argv)
{
	if (argc != 2) {
		konsole.write("Usage: toggle <cvar>\n");
		return 1;
	}
	CVar *cvar = find_cvar_by_name(argv[1]);
	if (!cvar) {
		konsole.write("toggle: no such cvar: `%s'\n", argv[1]);
		return 2;
	}
	if (cvar->type != CVar::TYPE_BOOL) {
		konsole.write("toggle: invalid variable type!\n");
		return 3;
	}
	cvar->value_bool[0] = !cvar->value_bool[0];
	return 0;
}
