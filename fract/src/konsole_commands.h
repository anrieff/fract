/***************************************************************************
 *   Copyright (C) 2006 by Veselin Georgiev                                *
 *   anrieff@mailg.com (convert to gmail)                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef __CONSOLE_COMMANDS_H__
#define __CONSOLE_COMMANDS_H__

#define QUICKHELP_STRING "-qh"

struct CommandStruct {
	char *cmdname;
	int (*exec_fn)(int argc, char **argv);
};

extern CommandStruct allcommands[];
int cmdcount(void);

int cmd_exit(int argc, char **argv);
int cmd_help(int argc, char **argv);
int cmd_cpu(int argc, char **argv);
int cmd_cmdlist(int argc, char **argv);
int cmd_cvarlist(int argc, char **argv);
int cmd_list(int argc, char **argv);
int cmd_fancy(int argc, char **argv);
int cmd_title(int argc, char **argv);
int cmd_inc(int argc, char **argv);
int cmd_mul(int argc, char **argv);
int cmd_toggle(int argc, char **argv);
int cmd_bind(int argc, char **argv);
int cmd_unbindall(int argc, char **argv);
int cmd_where(int argc, char **argv);


#endif // __CONSOLE_COMMANDS_H__
