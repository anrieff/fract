/***************************************************************************
 *   Copyright (C) 2005 by Veselin Georgiev                                *
 *   vesko@ChaosGroup.com                                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

struct FractFont {
	int x, y, charwidth;
	unsigned char *data;
	virtual const char * get_name() const = 0;
};

struct LuxiFont : public FractFont {
	LuxiFont();
	const char *get_name() const;
};

class FontMan {
	unsigned color, *buff;
	int xr, yr;
	FractFont * fontlist[5];
	int fontcount, curfont;
	int cx, cy;
	bool contrast;
	
	void draw(int x, int y, unsigned char opacity);
public:
	FontMan(unsigned *p, int resx, int resy);
	~FontMan();
	void set_font(const char *name);
	void set_color(unsigned);
	void set_cursor(int x, int y);
	void set_contrast(bool on);
	void set_fb(unsigned *p);
	FractFont * get_current_font() const;
	int get_x() const;
	int get_y() const;
	void print(const char *format, ...);
	void printxy(int, int, const char *, ...);
	void write(const char *buff);
	
};
