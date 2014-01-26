/***************************************************************************
 *   Copyright (C) 2006 by Veselin Georgiev                                *
 *   anrieff@mailg.com (convert to gmail)                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *-------------------------------------------------------------------------*
 * Implements sortable and minimalistic string array, where strings are of *
 * the char* type                                                          *
 ***************************************************************************/

#ifndef __STRING_ARRAY_H__
#define __STRING_ARRAY_H__
 
#include "array.h"

class StringArray {
	struct StringInfo {
		char * s;
		void* traits;
		StringInfo() {}
		StringInfo(char * o, void* t): s(o), traits(t) {}
		bool operator < (const StringInfo&) const;
	};
	Array<StringInfo> _info;
	char *_data;
	int _ml;
	int _cap;
	int _n;
public:
	StringArray();
	~StringArray();
	
	void add(const char *, void* traits = NULL);
	void sort(void);
	void filter(const char *prefix);
	int get_max_length() const;
	int size() const;
	const char *get_string(int index);
	const void * get_traits(int index);
};


#endif // __STRING_ARRAY_H__
