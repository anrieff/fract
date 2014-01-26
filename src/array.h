/***************************************************************************
 *   Copyright (C) 2004 by Veselin Georgiev                                *
 *   vesko@ViruS                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *-------------------------------------------------------------------------*
 * Implements random access resizeable array, just like STL's vector       *
 ***************************************************************************/

#ifndef __ARRAY_H__
#define __ARRAY_H__

#include <stdlib.h>

#define _initcap 64

template <typename T>
class Array {
	int _cap, _size;
	T *data;
public:
	Array () {
		_init();
	}
	~Array() {
		_destroy();
	}
	Array (const Array<T> & r)
	{
		_kopy(r);
	}
	Array & operator = (const Array<T> & r)
	{
		if (this != &r) {
			_destroy();
			_kopy(r);
		}
		return *this;
	}
	inline const T& operator [] (const int idx) const
	{
		return data[idx];
	}
	inline T& operator [] (const int idx) 
	{
		return data[idx];
	}
	inline void add(const T& t)
	{
		_newel();
		data[_size-1] = t;
	}
	inline void operator += (const T& t)
	{
		add(t);
	}
	inline int size() const
	{
		return _size;
	}
	void reverse(void)
	{
		int i = 0, j = _size - 1;
		while (i < j) {
			T t = data[i];
			data[i] = data[j];
			data[j] = t;
			i++, j--;
		}
	}
	void append(const Array<T> & r)
	{
		for (int i = 0; i < r._size; i++)
			add(r.data[i]);
	}
	inline void clear(void) 
	{
		_size = 0;
		if (_cap > _initcap)
		{
			_cap = _initcap;
			delete [] data;
			data = new T[_cap];
		}
	}
	void erase(int idx)
	{
		if (idx < 0 || idx >= _size) return;
		--_size;
		for (int i = idx; i < _size; i++)
			data[i] = data[i+1];
	}
private:
	void _init() {
		_cap = _initcap;
		_size = 0;
		data = new T[_cap];
	}
	void _destroy() {
		if (data)
			delete [] data;
		data = NULL;
	}
	void _kopy(const Array<T>& r)
	{
		_cap = r._cap;
		_size = r._size;
		data = new T[_cap];
		for (int i = 0; i < _size; i++)
			data[i] = r.data[i];
	}
	void _newel(void)
	{
		_size ++;
		if (_size > _cap) {
			_cap = 2* _cap;
			T * newdata = new T[_cap];
			for (int i = 0; i < _size - 1; i++)
				newdata[i] = data[i];
			delete [] data;
			data = newdata;
		}
	}
};

#endif
