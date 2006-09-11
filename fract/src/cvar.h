/***************************************************************************
 *   Copyright (C) 2006 by Veselin Georgiev                                *
 *   anrieff@mailg.com (convert to gmail)                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef __CVAR_H__
#define __CVAR_H__

struct CVar {
	union {
		int    *value_int;
		bool   *value_bool;
		double *value_double;
	};
	enum CVarType {
		TYPE_INT,
		TYPE_BOOL,
		TYPE_DOUBLE,
	} type;
	char name[32], description[120];
	static char temp_value_storage[128];
	
	CVar();
	CVar(const char *name, int    *, const char *description);
	CVar(const char *name, bool   *, const char *description);
	CVar(const char *name, double *, const char *description);
	
	const char *to_string(void);
	bool set_value(const char *new_value);
};

void cvars_init(void);
CVar *find_cvar_by_name(const char *name);
CVar *cvars_start(void);
CVar *cvars_iter(void);

#endif // __CVAR_H__
