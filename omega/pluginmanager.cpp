/***************************************************************************
 *   Copyright (C) 2006 by Veselin Georgiev                                *
 *   anrieff@mgail.com (convert to gmail)                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
***************************************************************************/
#include <stdio.h>
#include <vector>
#include <algorithm>
#ifdef XWIN32
#include <windows.h>
#include <stdlib.h>
#else
#include <glob.h>
#include <sys/time.h>
#include <dlfcn.h>
#endif
#include "pluginmanager.h"

using namespace std;


/**
 * @class  FindData
 * @brief  Generic class for searching directories for files
 * @date   2006-06-12
 * @author Veselin Georgiev
 *
 * The class is built upon FindFirstFile()/FindNextFile() under Win32
 * and upon glob() on *nix. It creates a list of files, matching
 * the given pattern.
 *
 * When searching in a distant path (e.g. "../sources/ *.so"), the returned
 * results are prefixed with that path (e.g. "../sources/foo.so", "../sources/bar.so", etc)
*/ 
class FindData {
	char **_names;
	int _count;
	int _iterator;
	
	void _construct();
	void _destruct();
	bool _populate(const char *path);
public:
	FindData();
	~FindData();
	
	/// Match the given path and wildcard for files (actually, it calls search())
	FindData(const char *path);
	
	/// Match the given path and wildcard for files
	/// @return true on success, false on failure
	bool search(const char *path);
	
	/// True if no files were found
	bool empty(void) const;
	
	/// The number of the files found
	int size(void) const;
	
	/// Returns the next matching file; or NULL if there are no more
	const char *next(void);
	
	/// Resets the current file iterator to the beginning
	void reset(void);
	
	/// Return a matching file by index [0 .. size() - 1]
	const char *operator[] (int index) const;
};

/**
 * @class FindData
 */
void FindData::_construct()
{
	_names = NULL;
	_count = 0;
	_iterator = 0;
}

#ifdef _WIN32
static int cmp_charp(const void *a, const void *b)
{
	const char *x = *(const char **) a, *y = *(const char **) b;
	return strcmp(x, y);
}
#endif

bool FindData::_populate(const char* path)
{
#ifdef _WIN32
//	win32's find does not return the full path name, so extract the root first
	int rootLen = strlen(path)-1;
	while (rootLen >= 0 && path[rootLen] != '/' && path[rootLen] != '\\') rootLen--;
	rootLen++;
//

	vector<char*> all;
	WIN32_FIND_DATA findData;
	HANDLE fd = FindFirstFile(path, &findData);
	if (fd == INVALID_HANDLE_VALUE) return false;
	do {
		if (findData.cFileName[0] == '.' &&
		(findData.cFileName[1] == 0 || findData.cFileName[1] == '.')) continue;
		int fileLen=strlen(findData.cFileName);
		int outlen = 1 + fileLen + rootLen;
		char *s = new char[outlen];
		if (!s) continue;
		vutils_memcpy(s, path, rootLen);
		vutils_memcpy(s+rootLen, findData.cFileName, fileLen+1);
		all += s;
	} while ( 0 != FindNextFile(fd, &findData));
	FindClose(fd);
	qsort(&all[0], all.size(), sizeof(char*), cmp_charp);
	//
	_names = new char*[all.size()];
	for (unsigned i = 0; i < all.size(); i++)
		_names[i] = all[i];
	_count = (int) all.size();
#else
	glob_t pg;
	if (0 != glob(path, GLOB_MARK|GLOB_TILDE, NULL, &pg)) return false;
	_names = new char*[pg.gl_pathc];
	_count = pg.gl_pathc;
	for (int i = 0; i < _count; i++) {
		_names[i] = new char[1 + strlen(pg.gl_pathv[i])];
		strcpy(_names[i], pg.gl_pathv[i]);
	}
	globfree(&pg);
#endif
	_iterator = 0;
	return true;
}

void FindData::_destruct(void)
{
	if (_count) {
		for (int i = 0; i < _count; i++)
			delete [] _names[i];
		delete [] _names;
		
	}
	_names = NULL; _count = 0;
}

FindData::FindData()
{
	_construct();
}

FindData::~FindData()
{
	_destruct();
}

FindData::FindData(const char *path)
{
	_construct();
	_populate(path);
}

bool FindData::search(const char *path)
{
	return _populate(path);
}

bool FindData::empty(void) const
{
	return _count == 0;
}

int FindData::size(void) const
{
	return _count;
}

const char* FindData::operator [](int index) const
{
	if (index < 0 || index >= _count || _names == NULL) return NULL;
	return _names[index];
}

void FindData::reset(void)
{
	_iterator = 0;
}

const char* FindData::next(void) 
{
	if (_iterator >= _count || _names == NULL) return NULL;
	return _names[_iterator++];
}

/* ------------------------------------------------------------------------ */

typedef Plugin* (*ConstructFun) (void);
typedef void (*DestructFun) (Plugin *);

struct DllModule {
#ifdef _WIN32
	HMODULE hmod;
#else
	void *hmod;
#endif
	ConstructFun construct_fn;
	DestructFun destruct_fn;
	Plugin* theplugin;
};

static int plug_idx;
static vector<DllModule> plugs;
static int curplugin = 0;

static bool plug_cmp(const DllModule &a, const DllModule &b)
{
	return a.theplugin->get_info().priority < b.theplugin->get_info().priority;
}

bool load_plugins(void)
{
#ifdef _WIN32
	FindData fd("plugins\\*.dll");
#else
	FindData fd("plugins/*.so");
#endif
	for (int i = 0; i < fd.size(); i++) {
		DllModule mod;
#ifdef _WIN32
		mod.hmod = LoadLibrary(fd[i]);
		if (!mod.hmod) continue;
		mod.construct_fn = (ConstructFun) GetProcAddress(mod.hmod, "new_class_instance");
		mod.destruct_fn  = (DestructFun)  GetProcAddress(mod.hmod, "delete_class_instance");
		
#else
		mod.hmod = dlopen(fd[i], RTLD_NOW);
		if (!mod.hmod) continue;
		mod.construct_fn = (ConstructFun) dlsym(mod.hmod, "new_class_instance");
		mod.destruct_fn  = (DestructFun)  dlsym(mod.hmod, "delete_class_instance");
#endif
		if (!mod.construct_fn || !mod.destruct_fn) continue;
		mod.theplugin = mod.construct_fn();
		if (!mod.theplugin) continue;
		plugs.push_back(mod);
	}
	sort(plugs.begin(), plugs.end(), plug_cmp);
}

void free_plugins(void)
{
	for (unsigned i = 0; i < plugs.size(); i++) {
		DllModule& mod = plugs[i];
		mod.destruct_fn(mod.theplugin);
#ifdef _WIN32
		FreeLibrary(mod.hmod);
#else
		dlclose(mod.hmod);
#endif
	}
}

/// returns the number of available plugins
int get_plugin_count(void)
{
	return (int) plugs.size();
}

/// gets the plugin at the given index (zero-based)
Plugin* get_plugin(int idx)
{
	int n = get_plugin_count();
	if (idx < 0 || idx >= n) return NULL;
	return plugs[idx].theplugin;
}

/// gets the currently selected plugin
Plugin* get_current_plugin(void)
{
	return plugs[curplugin].theplugin;
}

/// sets the plugin with the given idx to be current
void set_plugin(int idx)
{
	int n = get_plugin_count();
	if (idx < 0 || idx >= n) return;
	curplugin = idx;
}

