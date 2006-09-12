/***************************************************************************
 *   Copyright (C) 2006 by Veselin Georgiev                                *
 *   anrieff@mailg.com (convert to gmail)                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "string_array.h"
#include "common.h"
#include <string.h>

const int DEFAULT_ALLOC_SIZE = 10000;

bool StringArray::StringInfo::operator <(const StringArray::StringInfo & r) const
{
	return strcmp(s, r.s) < 0;
}

StringArray::StringArray()
{
	_data = NULL;
	_ml = 0;
	_cap = 0;
	_n = 0;
}

StringArray::~StringArray()
{
	if (_data) delete [] _data;
	_data = NULL;
}

void StringArray::add(const char *s, void* traits)
{
	int length = 1 + strlen(s);
	int bytes_needed = _n + length;
	
	if (bytes_needed > _cap) {
		if (0 == _cap) {
			_cap = DEFAULT_ALLOC_SIZE;
			_data = new char[_cap];
		} else {
			char *t = new char[2*_cap];
			for (int i = 0; i < _n; i++)
				t[i] = _data[i];
			delete [] _data;
			_data = t;
			_cap *= 2;
		}
	}
	memcpy(_data + _n, s, length);
	_info.add(StringInfo(_data + _n, traits));
	_n += length;
	
	--length;
	if (length > _ml) _ml = length;
}

void StringArray::filter(const char *prefix)
{
	Array<StringInfo> newinfo;
	int newmaxlen = 0;
	for (int i = 0; i < _info.size(); i++)
		if (0 == strncmp(prefix, _info[i].s, strlen(prefix))) {
			newinfo.add(_info[i]);
			if ((int) strlen(_info[i].s) > newmaxlen)
				newmaxlen = strlen(_info[i].s);
		}
	_info = newinfo;
	_ml = newmaxlen;
}

void StringArray::sort(void)
{
	sort_inplace(&_info[0], _info.size());
}

int StringArray::get_max_length() const
{
	return _ml;
}

int StringArray::size() const
{
	return _info.size();
}

const char *StringArray::get_string(int index)
{
	return _info[index].s;
}

const void *StringArray::get_traits(int index)
{
	return _info[index].traits;
}

