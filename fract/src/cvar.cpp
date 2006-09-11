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

/**
 * @class CVar
 */
 
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
