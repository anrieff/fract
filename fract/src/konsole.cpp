/***************************************************************************
 *   Copyright (C) 2006 by Veselin Georgiev                                *
 *   anrieff@mailg.com (convert to gmail)                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *-------------------------------------------------------------------------*
 * Displays and manages the fract console                                  *
 ***************************************************************************/
 
#include "MyGlobal.h"
#include "konsole.h"
#include "konsole_commands.h"
#include "cvar.h"
#include "fract.h"
#include "string_array.h"
#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <ctype.h>
#ifdef _WIN32
#include <malloc.h>
#endif

const int KONSOLE_DEFAULT_COLOR = 0xcccccc;
const float KONSOLE_ALPHA = 0.75f;
const double POPUP_TIME = 0.5; // in seconds

CommandStruct allcommands[] = {
	{ "exit", cmd_exit },
	{ "help", cmd_help },
	{ "cpu", cmd_cpu },
	{ "cmdlist", cmd_cmdlist },
	{ "cvarlist", cmd_cvarlist },
	{ "list", cmd_list },
	{ "fancy", cmd_fancy },
};

int cmdcount(void)
{
	return (int) (sizeof(allcommands)/sizeof(allcommands[0]));
}

HistoryEntry::HistoryEntry(const char *theline, int thepos)
{
	line = new char[1 + strlen(theline)];
	pos = thepos;
	strcpy(line, theline);
	next = prev = NULL;
}

HistoryEntry::~HistoryEntry()
{
	if (prev) delete prev;
	delete line;
}

Konsole::Konsole()
{
	selected_history = history = NULL;
	buffer = NULL;
	data = NULL;
	konsole_background = NULL;
	is_on = false;
	font = NULL;
	stroketime = -1000.0;
	_exit = false;
	fancy_level = 0;
}

Konsole::~Konsole()
{
	is_on = false;
	if (data) delete [] data;
	if (buffer) delete [] buffer;
	if (konsole_background) delete [] konsole_background;
	data = NULL;
	konsole_background = NULL;
	buffer = NULL;
}

void Konsole::init(int xres, int yres, Font *font_in)
{
	
	current_color = KONSOLE_DEFAULT_COLOR;
	font = font_in;
	xr = xres;
	yr = yres;
	int fsx = font->w_int();
	int fsy = font->h();
	lines = yres / 2 / fsy;
	cols = xres / fsx;
	buffer = new char [cols + 1];
	memset(buffer, 0, sizeof(buffer[0]) * (cols+1));
	buffpos = 0;
	data = new KonsoleChar[cols * lines];
	for (int i = 0; i < cols * lines; i++)
		data[i].neutral();
	cur_x = cur_y = 0;
	
	write("Welcome to fract's console!\nType `help' if you feel lost here\n\n");
	
	// create the console background:
	konsole_background = new Uint32[xr * (yr/2)];
	// render background:
	for (int j = 0; j < yr/2; j++) {
		for (int i = 0; i < xr; i++) {
			int r = rand() % 0x20;
			int g = rand() % 0x30;
			int b = rand() % 0x40;
			konsole_background[ j * xr + i ] = (r << 16) | (g << 8) | b;
		}
	}
	const int filter[3][3] = {
		{ 1, 2, 1 },
		{ 2, 4, 2 },
		{ 1, 2, 1 }
	};
	Uint32 *t = new Uint32[xr * (yr/2) ];
	for (int j = 0; j < yr/2; j++) {
		for (int i = 0; i < xr; i++) {
			int r = 0, g = 0, b = 0, a = 0;
			for (int xi = 0; xi < 3; xi ++) {
				for (int yi = 0; yi < 3; yi++) {
					int m = filter[xi][yi];
					if (i + xi >= 0 && i + xi < xr && j + yi >= 0 && j + yi < yr/2) {
						Uint32 x = konsole_background[ (j + yi) * xr + i + xi ];
						a += m;
						r += m * (0xff & (x >> 16));
						g += m * (0xff & (x >> 8 ));
						b += m * (0xff & x);
					}
				}
			}
			t[j * xr + i] = ((r / a) << 16) | ((g / a) << 8) | (b / a);
		}
	}
	delete [] konsole_background;
	konsole_background = t;
}

void Konsole::fancy(void)
{
	if (fancy_level == 0) {
		int xp = 0;
		for (int j = 0; j < yr/2; j++) {
			float y = (float) j / xr;
			for (int i = 0; i < xr; i++, xp++) {
				float x = (float) i / xr;
				float dist = sqrtf(sqr(x - 0.5f) + sqr(y - 0.25f));
				konsole_background[xp] = multiplycolorf(konsole_background[xp], fabs(sin(dist*50.0f)));
			}
		}	
	}
	if (fancy_level == 1) {
		for (int j = 0; j < yr / 2 ; j++) {
			if (j / 3 % 2) {
				for (int i = 0; i < xr; i++)
					konsole_background[xr * j + i] = 0;
			}
		}
	}
	if (fancy_level == 2) {
		memset(konsole_background, 0, xr * (yr / 2) * 4);
	}
	++fancy_level;
}

void Konsole::scroll(void)
{
	for (int i = 0; i < lines-1; i++)
		for (int j = 0; j < cols; j++)
			data[i * cols + j] = data[(i+1) * cols + j];
	for (int j = 0; j < cols; j++)
		data[(lines-1)*cols + j].neutral();
}

void Konsole::puts(const char *s)
{
	for (int i = 0; s[i]; i++) putchar(s[i]);
}

void Konsole::putchar(char c)
{
	if (c >= 32) {
		int idx = cur_x + cur_y * cols;
		data[idx].color = current_color;
		data[idx].ch = c;
		advance();
	} else {
		switch (c) {
			case '\n': 
			{
				cur_x = 0;
				cur_y++;
				if (cur_y >= lines) {
					--cur_y;
					scroll();
				}
				break;
			}
			default:
			{
				putchar('?');
				break;
			}
		}
	}
}

void Konsole::advance(void)
{
	cur_x ++;
	if (cur_x >= cols) {
		cur_x = 0;
		cur_y++;
		if (cur_y >= lines) {
			--cur_y;
			scroll();
		}
	}
}

void Konsole::write(const char *buf, ...)
{
	va_list arg;
	static char s[1024];
	va_start(arg, buf);
#ifdef _MSC_VER
	_vsnprintf(s, 1024, buf, arg); // M$ sucks with those _names :)
#else
	vsnprintf(s, 1024, buf, arg);
#endif
	va_end(arg);

	puts(s);
}

void Konsole::toggle()
{
	is_on = !is_on;
	stroketime = bTime();
#ifdef ACTUALLYDISPLAY
	if (is_on) {
		SDL_EnableKeyRepeat(500, 30);
	} else {
		SDL_EnableKeyRepeat(0, 0); // this is meant to disable key repeat
	}
#endif
}

void Konsole::show(bool reallyshow)
{
	if (reallyshow != is_on) 
		toggle();
}

bool Konsole::visible() const
{
	return is_on || (bTime() - stroketime <= POPUP_TIME);
}

void Konsole::render(void* screen, Uint32 *fb)
{
	if (!is_on && bTime() - stroketime > POPUP_TIME) return;
#ifdef ACTUALLYDISPLAY
	char *temp = (char *) alloca(cols+2);
	SDL_Surface *p = (SDL_Surface*) screen;
	
	// calculate how much lines of the console must be shown. They are yr/2
	// when the console is fully opened, but might be less if it is in 
	// a pop-up state.
	int yspan;
	if (bTime() - stroketime <= POPUP_TIME) {
		double x = (bTime() - stroketime) / POPUP_TIME;
		if (is_on) 
			yspan = (int) (yr / 2 * sqrt(x));
		else
			yspan = (int) (yr / 2 * sqrt(1.0 - x));
	} else yspan = yr / 2;
	int yoffset = yr/2 - yspan;
	
	// put the background
	int xp = 0;
	for (int j = 0; j < yspan; j++)
		for (int i = 0; i < xr; i++, xp++) {
			fb[xp] = blend(
				konsole_background[xp + yoffset * xr],
				fb[xp],
				KONSOLE_ALPHA);
		}
	
	// clean up the current cursor line
	for (int i = 0; i < cols; i++)
		data[cols * cur_y + i].neutral();
	cur_x = 0;
	write("~> ");
	write("%s", buffer);
	cur_x = 3 + buffpos;
	
	//push the text:
	for (int j = 0; j < lines; j++) if (data[j*cols].ch) {
		int i=0, k=0, cc=data[j*cols].color, start = 0;
		while (1) {
			while (i < cols && data[j * cols + i].color == cc && data[j * cols + i].ch) {
				temp[k++] = data[j * cols + i].ch;
				i++;
			}
			temp[k] = 0;
			if (j * font->h() - yoffset >= 0)
				font->printxy(p, fb, start * (font->w_int()), j * font->h() - yoffset, cc, KONSOLE_ALPHA, temp);
			start += k;
			if (i >= cols || data[j * cols + i].ch == 0) break;
			cc = data[j*cols + i].color;
			k = 0;
		}	
	}
	
	// place the cursor:
	if (get_ticks() / 500 % 2 && font->h() * cur_y - 1 - yoffset >= 0) {
		font->printxy(p, fb, font->w_int() * cur_x, font->h() * cur_y - yoffset, 0x00ff00, KONSOLE_ALPHA, "_");
		font->printxy(p, fb, font->w_int() * cur_x, font->h() * cur_y - 1 - yoffset, 0x00ff00, KONSOLE_ALPHA, "_");
	}
#endif
}

bool Konsole::handle_keycode( int code, bool shift )
{
#ifndef ACTUALLYDISPLAY
	return false;
#else
	char c = try_char(code, shift);
	if (c != 0) {
		//putchar(c);
		int n = strlen(buffer);
		if (n < cols - 4) {
			for (int i = n; i > buffpos; i--)
				buffer[i] = buffer[i-1];
			buffer[buffpos++] = c;
		}
		return true;
	}
	if (code == SDLK_BACKSPACE) {
		if (buffpos > 0) {
			--buffpos;
			for (int i = buffpos; buffer[i]; i++)
				buffer[i] = buffer[i+1];
		}
		return true;
	}
	if (code == SDLK_LEFT || code == SDLK_KP4) {
		if (buffpos > 0) --buffpos;
		return true;
	}
	if (code == SDLK_RIGHT || code == SDLK_KP6) {
		if (buffpos < (int) strlen(buffer)) ++buffpos;
		return true;
	}
	if (code == SDLK_UP || code == SDLK_KP8) {
		history_prev();
	}
	if (code == SDLK_DOWN || code == SDLK_KP2) {
		history_next();
	}
	if (code == SDLK_DELETE || code == SDLK_KP_PERIOD) {
		if (buffpos < (int) strlen(buffer)) {
			for (int i = buffpos; buffer[i]; i++) {
				buffer[i] = buffer[i+1];
			}
		}
		return true;
	}
	if (code == SDLK_RETURN || code == SDLK_KP_ENTER) {
		history_add(buffer, buffpos);
		selected_history = NULL;
		putchar('\n');
		//execute the command
		execute(buffer);
		//reclear the buffer
		memset(buffer, 0, sizeof(buffer[0]) * (cols+1));
		buffpos = 0;
		return true;
	}
	if (code == SDLK_TAB) {
		// get the partial expression:
		int i = buffpos - 1;
		while (i >= 0 && !isspace(buffer[i])) i--;
		++i;
		int length = buffpos - i;
		char *part = (char *) alloca(sizeof(char) * (length + 1));
		memcpy(part, buffer + i, length);
		part[length] = 0;
		
		//attempt to complete it
		StringArray array;
		// add all commands and cvars
		for (int i = 0; i < cmdcount(); i++)
			array.add(allcommands[i].cmdname, NULL);
		for (CVar *cvar = cvars_start(); cvar; cvar = cvars_iter()) 
			array.add(cvar->name, cvar);
		
		// filter using the partial expression:
		array.filter(part);
		
		// see what's left
		if (array.size() == 0) return true;
		if (array.size() == 1) { // woot - we can tab-complete it!
			const char * whole = array.get_string(0);
			whole += length;
			int n = strlen(buffer);
			int m = strlen(whole) + 1;
			if (n < cols - 3 - m) {
				for (int i = n - 1 + m; i >= buffpos + m; i--)
					buffer[i] = buffer[i-m];
				for (int i = 0; i < m-1; i++)
					buffer[buffpos+i] = whole[i];
				buffer[buffpos+m-1] = ' ';
				buffpos += m;
			}
		} else { // too many matches - display them all...
			array.sort();
			konsole.write("\n");
			for (int i = 0; i < array.size(); i++) {
				//align under cursor
				for (int j = 0; j < 3 + buffpos; j++)
					konsole.write(" ");
				konsole.set_color(array.get_traits(i) ? 0xccccff : 0xffffcc);
				konsole.write("%s", array.get_string(i));
				konsole.set_default_color();
				konsole.write("\n");
			}
		}
			
		return true;
	}
	return false;
#endif
}

void Konsole::remember_from_history(void)
{
	if (!selected_history) return;
	strcpy(buffer, selected_history->line);
	buffpos = selected_history->pos;
}

void Konsole::history_prev(void)
{
	if (!selected_history) {
		selected_history = history;
		if (!selected_history) return;
		remember_from_history();
	} else {
		if (selected_history->prev) {
			selected_history = selected_history->prev;
			remember_from_history();
		}
	}
}

void Konsole::history_next(void)
{
	if (!selected_history) return;
	if (selected_history->next) {
		selected_history = selected_history->next;
		remember_from_history();
	} else {
		buffer[0] = 0;
		buffpos = 0;
		selected_history = NULL;
	}
}

void Konsole::history_add(const char *command, int pos)
{
	HistoryEntry *e = new HistoryEntry(command, pos);
	if (history) {
		history->next = e;
		e->prev = history;
	}
	history = e;
}

char Konsole::try_char(int code, bool shift)
{
#ifdef ACTUALLYDISPLAY
	if (code >= SDLK_a && code <= SDLK_z) {
		char base = shift ? 'A' : 'a';
		return base + (code - SDLK_a);
	}
	if (shift) {
		switch (code) {
			case SDLK_1:            return '!';
			case SDLK_2:            return '@';
			case SDLK_3:            return '#';
			case SDLK_4:            return '$';
			case SDLK_5:            return '%';
			case SDLK_6:            return '^';
			case SDLK_7:            return '&';
			case SDLK_8:            return '*';
			case SDLK_9:            return '(';
			case SDLK_0:            return ')';
			case SDLK_MINUS:        return '_';
			case SDLK_EQUALS:       return '+';
			case SDLK_LEFTBRACKET:  return '{';
			case SDLK_RIGHTBRACKET: return '}';
			case SDLK_SEMICOLON:    return ':';
			case SDLK_QUOTE:        return '\"';
			case SDLK_BACKSLASH:    return '|';
			case SDLK_SLASH:        return '?';
			case SDLK_COMMA:        return '<';
			case SDLK_PERIOD:       return '>';
		}
	} else {
		if (code >= SDLK_SPACE && code <= SDLK_BACKQUOTE) 
			return (char) code;
	}
	
#endif
	return 0;
}

static void cleanup(int argc, char **argv)
{
	for (int i = 0; i < argc; i++)
		free(argv[i]);
	free(argv);
}

int Konsole::execute(const char *rawcmd)
{
	// see if the command line contains multiple commands, delimited by ';'s
	
	if (strchr(rawcmd, ';')) {
		char *t = (char *) alloca(sizeof(char) * (strlen(rawcmd) + 1));
		strcpy(t, rawcmd);
		int lasti = 0;
		int n = strlen(t);
		for (int i = 0; i < n; i++) {
			if (t[i] == ';') {
				t[i] = 0;
				int r = execute(t + lasti);
				if (r) return r;
				lasti = i+1;
			}
		}
		return execute(t + lasti); // execute the last command on the line 
	}
	
	// split the command into arguments...
	int argc, n; char **argv;
	
	argc = 0;
	n = (int) strlen(rawcmd);
	bool whitespace = true;
	for (int i = 0; i <= n; i++) {
		if (i == n || isspace(rawcmd[i])) {
			whitespace = true;
		} else {
			if (whitespace) {
				argc++;
			}
			whitespace = false;
		}
	}
	if (!argc) return 0;
	argv = (char**) malloc(sizeof(char*) * argc);
	int start = -1;
	argc = 0;
	for (int i = 0; i <= n; i++) {
		if (i == n || isspace(rawcmd[i])) {
			if (start >= 0 && !whitespace) {
				int length = i - start;
				argv[argc-1] = (char *) malloc(sizeof(char) * (length + 1));
				memcpy(argv[argc-1], rawcmd+start, length * sizeof(char));
				argv[argc-1][length] = 0;
			}
			whitespace = true;
		} else {
			if (whitespace) {
				argc++;
				start = i;
			}
			whitespace = false;
		}
	}
	
	
	for (unsigned i = 0; i < sizeof(allcommands)/sizeof(allcommands[0]); i++) {
		if (0 == strcmp(argv[0], allcommands[i].cmdname)) {
			int r = allcommands[i].exec_fn(argc, argv);
			cleanup(argc, argv);
			return r;
		}
	}
	CVar *cvar = find_cvar_by_name(argv[0]);
	if (cvar) {
		if (argc == 1) {
			konsole.write("%s is %s\n", argv[0], cvar->to_string());
		} else {
			if (!cvar->set_value(argv[1])) {
				konsole.write("Can't set %s to %s - value is invalid.\n", argv[0], argv[1]);
				cleanup(argc, argv);
				return 1;
			}
		}
		cleanup(argc, argv);
		return 0;
	}
	konsole.write("No such cvar or command: `%s'\n", rawcmd);
	cleanup(argc, argv);
	return -1;
}

void Konsole::set_color(int new_color)
{
	current_color = new_color;
}

void Konsole::set_default_color(void)
{
	current_color = KONSOLE_DEFAULT_COLOR;
}

void Konsole::exit(void)
{
	_exit = true;
}

bool Konsole::wants_exit() 
{
	return _exit;
}

//////// data...

Konsole konsole;
