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
#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <ctype.h>
#ifdef _WIN32
#include <malloc.h>
#endif

const int KONSOLE_DEFAULT_COLOR = 0xcccccc;

CommandStruct allcommands[] = {
	{ "help", cmd_help },
	{ "cpu", cmd_cpu },
	{ "testarg", cmd_testarg }, /// FIXME!
	{ "cmdlist", cmd_cmdlist },
};

int cmdcount(void)
{
	return (int) (sizeof(allcommands)/sizeof(allcommands[0]));
}

Konsole::Konsole()
{
	buffer = NULL;
	data = NULL;
	konsole_background = NULL;
	is_on = false;
	font = NULL;
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
	int fsx = (int) ceil(font->w());
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
}

void Konsole::show(bool reallyshow)
{
	is_on = reallyshow;
}

bool Konsole::visible() const
{
	return is_on;
}

void Konsole::render(void* screen, Uint32 *fb)
{
	if (!is_on) return;
#ifdef ACTUALLYDISPLAY
	char *temp = (char *) alloca(cols+2);
	SDL_Surface *p = (SDL_Surface*) screen;
	
	int xp = 0;
	for (int j = 0; j < yr/2; j++)
		for (int i = 0; i < xr; i++, xp++)
			fb[xp] = konsole_background[xp];
	
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
			font->printxy(p, fb, start * (font->w_int()), j * font->h(), cc, 0.8f, temp);
			start += k;
			if (i >= cols || data[j * cols + i].ch == 0) break;
			cc = data[j*cols + i].color;
			k = 0;
		}	
	}
	
	// place the cursor:
	font->printxy(p, fb, font->w_int() * cur_x, font->h() * cur_y, 0x00ff00, 0.8f, "_");
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
		if (n < cols - 3) {
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
	if (code == SDLK_DELETE || code == SDLK_KP_PERIOD) {
		if (buffpos < (int) strlen(buffer)) {
			for (int i = buffpos; buffer[i]; i++) {
				buffer[i] = buffer[i+1];
			}
		}
		return true;
	}
	if (code == SDLK_RETURN || code == SDLK_KP_ENTER) {
		putchar('\n');
		//execute the command
		execute(buffer);
		//reclear the buffer
		memset(buffer, 0, sizeof(buffer[0]) * (cols+1));
		buffpos = 0;
		return true;
	}
	return false;
#endif
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


//////// data...

Konsole konsole;
