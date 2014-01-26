/***************************************************************************
 *   Copyright (C) 2006 by Veselin Georgiev                                *
 *   anrieff@mailg.com (convert to gmail)                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <stdio.h>
#include <string.h>
#include "cvar.h"
#include "cvars.h"
#include "array.h"
#include "light.h"

/**
 * @class CVar
 */
 
char CVar::temp_value_storage[128];
 
CVar::CVar(){}

CVar::CVar(const char *name, int *ptr, const char *description)
{
	strcpy(this->name, name);
	strcpy(this->description, description);
	
	type = TYPE_INT;
	value_int = ptr;
}

CVar::CVar(const char *name, bool *ptr, const char *description)
{
	strcpy(this->name, name);
	strcpy(this->description, description);
	
	type = TYPE_BOOL;
	value_bool = ptr;
}

CVar::CVar(const char *name, double *ptr, const char *description)
{
	strcpy(this->name, name);
	strcpy(this->description, description);
	
	type = TYPE_DOUBLE;
	value_double = ptr;
}

const char *CVar::to_string(void)
{
	switch (type) {
		case TYPE_INT   : sprintf(temp_value_storage, "%d", *value_int); break;
		case TYPE_BOOL  : sprintf(temp_value_storage, "%d", (*value_bool)?1:0); break;
		case TYPE_DOUBLE: sprintf(temp_value_storage, "%.4lf", *value_double); break;
	}
	return temp_value_storage;
}

bool CVar::set_value(const char *new_value)
{
	switch (type) {
		case TYPE_INT: 
		{
			int x, r;
			r = sscanf(new_value, "%d", &x);
			if (r == 1) {
				*value_int = x;
				check_triggers();
				return true;
			}
			return false;
		}
		case TYPE_BOOL  :
		{
			int x;
			if (1 != sscanf(new_value, "%d", &x)) return false;
			if (x < 0 || x > 1) return false;
			*value_bool = x == 1;
			check_triggers();
			return true;
		}
		case TYPE_DOUBLE: 
		{
			int r; double x;
			r = sscanf(new_value, "%lf", &x);
			if (r == 1) {
				*value_double = x;
				check_triggers();
				return true;
			}
			return false;
		}
		default:
			return false;
	}
}

Array<CVar> allcvars;

void cvars_init(void)
{
	#define IMPLEMENTATION
	#include "cvarlist.h"
	#undef IMPLEMENTATION
}

static int cvars_iterator;
CVar * cvars_start(void)
{
	cvars_iterator = 0;
	return cvars_iter();
}

CVar * cvars_iter(void)
{
	if (cvars_iterator >= allcvars.size()) return NULL;
	return &allcvars[cvars_iterator++];
}

CVar * find_cvar_by_name(const char *name)
{
	for (CVar * cvar = cvars_start(); cvar; cvar = cvars_iter()) {
		if (0 == strcmp(name, cvar->name)) return cvar;
	}
	return NULL;
}

const char *CVar::get_type_name(void) const
{
	switch (type) {
		case TYPE_INT: return "INT";
		case TYPE_BOOL: return "BOOL";
		case TYPE_DOUBLE: return "REAL";
		default: return "UNKNOWN";
	}
}

void CVar::check_triggers(void)
{
	static bool entered = false;
	if (entered) return;
	entered = true;
	if (!strcmp(name, "lmsize")) {
		CVars::shadow_algo = 2;
		light.rebuild_lightmaps();
	}
	
	if (!strcmp(name, "shadow_algo")) {
		(*value_int) %= 3;
		light.mode = (ShadowMode) CVars::shadow_algo;
		if (light.points[0].size == 0) {
			CVars::lmsize = 128;
			light.rebuild_lightmaps();
		}
	}
	
	entered = false;
}
