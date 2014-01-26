/***************************************************************************
 *   Copyright (C) 2004 by Veselin Georgiev                                *
 *   vesko@ViruS                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef __CMDLINE_H__
#define __CMDLINE_H__
struct cmdinfo{
	char *data;
	char *value;
	cmdinfo *next;
};

void initcmdline(int argc, char * argv[]);
void option_add(const char *opt);
int option_exists(const char *opt);
char *option_value_string(const char *opt);
int option_value_int(const char *opt);
float option_value_float(const char *opt);
void display_usage(void);
void cmdline_close(void);

class ArgumentIterator {
	cmdinfo *iter;
public:
	ArgumentIterator();
	bool ok() const;
	const char *value() const;
	void operator ++ ();
};

#endif
