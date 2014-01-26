/***************************************************************************
 *   Copyright (C) 2006 by Veselin Georgiev                                *
 *   anrieff@mailg.com (convert to gmail)                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/
 
#ifndef __KONSOLE_H__
#define __KONSOLE_H__

#include "MyTypes.h"
#include "bitmap.h"
#include "gfx.h"
#include "array.h"

struct KonsoleChar {
	int color;
	char ch;
	
	void neutral(void)
	{
		color = 0;
		ch = 0;
	}
};

struct HistoryEntry {
	char * line;
	int pos;
	HistoryEntry *next, *prev;
	HistoryEntry(const char *line, int pos);
	~HistoryEntry();
};

struct KeyBinding {
	char key[8];
	char binding[200];
	KeyBinding(){}
	KeyBinding(const char *, const char *);
	static bool key_exists(const char *);
};

class Alias {
	void destroy(void);
	void _kopy(const Alias&);
public:
	char name[32];
	char *statement;
	Alias();
	~Alias();
	Alias(const Alias &);
	Alias& operator = (const Alias &);
};

class Konsole {
	int lines, cols;
	int xr, yr;
	int current_color;
	int fancy_level;
	KonsoleChar *data;
	char *buffer; int buffpos;
	Uint32 *konsole_background;
	int cur_x, cur_y;
	bool is_on;
	bool _exit;
	double stroketime;
	Font *font;
	HistoryEntry *history, *selected_history;
	KeyBinding *keys;
	Array<Alias> aliases;
	int keys_size;
	char *photoframe_callback;
	int photoframe_callback_nframes;
	
	void advance(void);
	void putchar(char c);
	void scroll(void);
	void remember_from_history(void);
	void history_add(const char *command, int);
	void history_prev(void);
	void history_next(void);
public:
	Konsole();
	~Konsole();
	
	void exit(void);
	bool wants_exit();
	void fancy(void);
	void init(int xres, int yres, Font *font);
	void toggle();
	void show(bool reallyshow = true);
	bool visible() const;
	void render(void *screen, Uint32* framebuff);
	bool handle_keycode(int keycode, bool shift);
	void write(const char *format, ...);
	void puts(const char *s);
	void set_color(int new_color);
	void set_default_color(void);
	int execute(const char *cmd);
	void add_key(const KeyBinding &);
	KeyBinding *get_keys(void);
	Alias *find_alias_by_name(const char * name);
	void add_alias(const Alias&);
	int get_keys_count(void) const;
	void keys_unbind_all(void);
	bool key_bound(int code, bool shift);
	void dump(const char* fname);
	void set_photoframe_callback(int num_frames, char *callback);
	void photo_frame_done(void);
};

extern Konsole konsole;

#endif // __KONSOLE_H__
