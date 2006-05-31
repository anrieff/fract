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
typedef struct cmdinfo{
char *data;
char *value;
cmdinfo *next;
};

void initcmdline(int argc, char * argv[]);
void OptionAdd(char *opt);
int OptionExists(char *opt);
char *OptionValueString(char *opt);
int OptionValueInt(char *opt);
float OptionValueFloat(char *opt);
void DisplayUsage(void);
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
