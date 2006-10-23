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
 
 
#include "comparedlg.h"

#include <wx/image.h>
#include <math.h>
#include <algorithm>
using namespace std;
		
BEGIN_EVENT_TABLE(CompareDialog, wxDialog)
	EVT_BUTTON(bSave, CompareDialog:: OnSaveChart)
END_EVENT_TABLE()


const unsigned COLOR_GRID = 0xababab;
const unsigned COLOR_GRID_LIGHT = 0xefefef;
const int PER_ENTRY = 50;


static float fun_fps(CompareInfo & x) { return x.fps; }
static float fun_eff(CompareInfo & x) { return x.fps / x.mhz; }

static inline void decomp(unsigned x, float r[3])
{
	r[2] = x & 0xff; x >>= 8;
	r[1] = x & 0xff; x >>= 8;
	r[0] = x & 0xff; 
}

static inline unsigned recompose(float r[3])
{
	unsigned x = 0;
	for (int i = 0; i < 3; i++) {
		if (r[i] < 0) r[i] = 0;
		if (r[i] > 255.0f) r[i] = 255.0f;
		x <<= 8;
		x |= (unsigned) r[i];
	}
	return x;
}

static unsigned mulcol(unsigned x, float mul)
{
	float r[3];
	decomp(x, r);
	for (int i = 0; i < 3; i++)
		r[i] *= mul;
	return recompose(r);
}

static void draw(unsigned *fb, int xr, int x, int y, unsigned color, double alpha)
{
	unsigned &p = fb[y * xr + x];
	float src[3], dst[3];
	decomp(p, src); decomp(color, dst);
	for (int i = 0; i < 3; i++)
		src[i] = (1.0 - alpha) * src[i] + alpha * dst[i];
	p = recompose(src);
}

/**
 * @class FractChart
*/ 

void FractChart::draw_line(int x1, int y1, int x2, int y2)
{
	int xsteps, ysteps, steps;
	unsigned color = drawcol, *fb = drawbuff;
	xsteps = x1 - x2; if (xsteps < 0) xsteps = -xsteps;
	ysteps = y1 - y2; if (ysteps < 0) ysteps = -ysteps;
	steps = max(max(xsteps, ysteps), 1);
	
	for (int i = 0; i <= steps; i++) {
		float xf = x1 + ((float)x2-x1)*i/steps;
		float yf = y1 + ((float)y2-y1)*i/steps;
		int x = (int) xf;
		int y = (int) yf;
		if (xsteps > ysteps) {
			draw(fb, xr, x, y, color, 1 - (yf - y));
			draw(fb, xr, x, y + 1, color, (yf - y));
		} else {
			draw(fb, xr, x, y, color, 1 - (xf - x));
			draw(fb, xr, x + 1, y, color, (xf - x));
		}
	}
}

static float pover(float x, int po)
{
	float r = 1.0f;
	for (int i = 0; i < po; i++)
		r *= x;
	return r;
}

void FractChart::draw_chart(CompareInfo a[], int n, float (*fun) (CompareInfo&), int sx, int sy, int sizex, int sizey, FontMan &fm)
{
	float minval, maxval;
	minval = fun(a[0]);
	maxval = minval;
	for (int i = 0; i < n; i++) {
		float val = fun(a[i]);
		if (val < minval) minval = val;
		if (val > maxval) maxval = val;
	}

	int mtick = (int) ceil(maxval/minval);
	
	drawcol = COLOR_GRID_LIGHT;
	if ((sizex - 20) / mtick > 20) {
		for (int i = 1; i < mtick * 10; i++) if (i % 10) {
			int xx = i * (sizex-20) / (10*mtick);
			draw_line(sx + xx, sy + sizey, sx + xx + 20, sy + sizey - 15);
			draw_line(sx + xx + 20, sy + sizey - 15, sx + xx + 20, sy );
		}
	}
	drawcol = COLOR_GRID;
	draw_line(sx, sy + 15, sx, sy + sizey);
	draw_line(sx, sy + sizey, sx + sizex - 20, sy + sizey);
	
	draw_line(sx + 20, sy, sx + 20, sy + sizey - 15);
	draw_line(sx + 20, sy + sizey - 15, sx + sizex, sy + sizey - 15);
	draw_line(sx + sizex, sy + sizey - 15, sx + sizex, sy);
	draw_line(sx + 20, sy, sx + sizex, sy);
	
	draw_line(sx, sy + 15, sx + 20, sy);
	draw_line(sx, sy + sizey, sx + 20, sy + sizey - 15);
	draw_line(sx + sizex - 20, sy + sizey, sx + sizex, sy + sizey - 15);
	
	for (int i = 1; i < n; i++) {
		int yy = i * (sizey-15) / n;
		draw_line(sx, sy + yy + 15, sx + 20, sy + yy);
		draw_line(sx + 20, sy + yy, sx + sizex, sy + yy);
	}
	
	for (int i = 1; i < mtick; i++) {
		int xx = i * (sizex-20) / mtick;
		draw_line(sx + xx, sy + sizey, sx + xx + 20, sy + sizey - 15);
		draw_line(sx + xx + 20, sy + sizey - 15, sx + xx + 20, sy );
	}
	
	int per_entry = (sizey-15)/n;
	
	for (int i = 0; i < n; i++) {
		float coeff;
		if (maxval != minval) coeff = (fun(a[i]) - minval) / (maxval - minval);
			else coeff = 1.0f;
		unsigned mycolor = (int) (0xff * (1-coeff)) | (((int) (0xff0000 * coeff))&0xff0000);
		for (float angle = 0.0f; angle < 2 * M_PI; angle += 0.02f) {
			int sgx = 10 + (int) ( 10.0f * sin(angle));
			int sgy = i * (sizey-15) / n + per_entry/2 + 5 - (int) ( 15.0f * cos(angle));
			float beta = fabs(angle - 5.12);
			if (beta > M_PI) beta = 2 * M_PI - beta;
			
			float intensity = cos(beta); if (intensity < 0.40f) intensity = 0.40f;
			intensity += pover(intensity, 65);
			drawcol = mulcol(mycolor, intensity);
			draw_line(sx + sgx, sy + sgy, sx + sgx + (int) ((sizex-20)*fun(a[i])/minval/mtick), sy + sgy);
		}
		fm.set_cursor(sx + 10, sy + i * (sizey-15) / n + per_entry/2 - 3);
		fm.set_color(0xffffff);
		fm.set_contrast(true);
		fm.print("%s", a[i].name.c_str());
		fm.set_contrast(false);
		fm.set_cursor(sx+sizex+4, -1);
		fm.set_color(0);
		fm.print("%.2f", fun(a[i])/minval);
	}
	
}

void FractChart::render(CompareInfo a[], int n)
{
	unsigned *p = drawbuff;
	for (int i = 0; i < yr; i++)
		for (int j = 0; j < xr; j++) {
			p[i*xr+j] = (255-(16*i/yr)-(16*j/xr)) << 16 |
			            (255-( 8*i/yr)-( 8*j/yr)) << 8 | 0xff;
			if (i == 0 || i == yr-1 || j == 0 || j == xr - 1)
				p[i*xr+j] = 0;
		}
	FontMan fontman(p, xr, yr);
	fontman.printxy(26, 2, "FPS chart");
	fontman.printxy(26, 42 + n * PER_ENTRY + 15, "Effectivity chart (FPS/MHz)");
	draw_chart(a, n, fun_fps, 5, 18, 550, n*PER_ENTRY + 15, fontman);
	draw_chart(a, n, fun_eff, 5, 58 + n * PER_ENTRY + 15, 550, n * PER_ENTRY + 15, fontman);
	
	fontman.set_color(0x0);
	fontman.printxy(10, 90 + 2*n*PER_ENTRY, "All scores are relative to the slowest system,");
	fontman.printxy(10, 108 + 2*n*PER_ENTRY, "which is taken as a base (1.0)");
	
	fontman.set_color(0xaaccff);
	fontman.printxy(xr - 200, yr-20, "Created with Fract 1.07b");
	drawcol = 0;
}

FractChart::FractChart(wxWindow *parent, int id, CompareInfo a[], int count, wxPoint pos, wxSize size)
{
	drawbuff = new unsigned[size.x*size.y];
	xr = size.x;
	yr = size.y;
	render(a, count);
	unsigned char *temp = (unsigned char*) malloc(xr*yr*3);
	for (int i = 0; i < xr*yr; i++) {
		temp[i * 3 + 2] = (drawbuff[i]     ) & 0xff;
		temp[i * 3 + 1] = (drawbuff[i] >> 8) & 0xff;
		temp[i * 3 + 0] = (drawbuff[i] >>16) & 0xff;
	}
	wxImage img(xr, yr, temp);
	if (!img.Ok()) return;
	wxBitmap bmp(img);
	if (!bmp.Ok()) return;
	wxStaticBitmap *sbmp = new wxStaticBitmap(parent, id, bmp, pos);
	sbmp->Refresh();
}

FractChart::~FractChart()
{
	if (drawbuff) delete [] drawbuff;
	drawbuff = NULL;
}

wxSize FractChart::get_needed_area(int how_many_results)
{
	return wxSize(600, 150 + 2*how_many_results*PER_ENTRY);
}

bool FractChart::save_chart(wxString fn)
{
	static bool png_inited = false;
	if (!png_inited) {
		png_inited = true;
		wxImage::AddHandler(new wxPNGHandler);
	}
	unsigned char *temp = (unsigned char*) malloc(xr*yr*3);
	for (int i = 0; i < xr*yr; i++) {
		temp[i * 3 + 2] = (drawbuff[i]     ) & 0xff;
		temp[i * 3 + 1] = (drawbuff[i] >> 8) & 0xff;
		temp[i * 3 + 0] = (drawbuff[i] >>16) & 0xff;
	}
	wxImage img(xr, yr, temp);
	return img.SaveFile(fn);
}

/**
 * @class CompareDialog
*/ 

CompareDialog::CompareDialog(wxWindow* parent, CompareInfo a[], int count) :
	wxDialog(parent, wxID_ANY, "Result comparison", 
	wxDefaultPosition, wxSize(600, 600))
{
	wxSize sz = FractChart::get_needed_area(count);
	SetClientSize(sz.x + 30, sz.y + 80);
	fc = new FractChart(this, fcChart, a, count, wxPoint(15, 10), sz);
	
	wxButton *closebtn = new wxButton(this, wxID_OK, "&Close", 
		wxPoint(195, 30 + sz.y), wxSize(100, 30));
	closebtn->Refresh();
	m_savebut = new wxButton(this, bSave, "&Save chart", 
		wxPoint(305, 30 + sz.y), wxSize(100, 30));
}

void CompareDialog::OnSaveChart(wxCommandEvent&)
{
	wxString filename = wxFileSelector("Choose file name to save", ".", 
		"Fract Chart.png", "png", "PNG files (*.png)|*.png", wxSAVE);
	if (filename.empty()) return;
	
	if (!fc->save_chart(filename))
		wxMessageBox("The chart cannot be saved (read-only media?)", "Error", wxICON_ERROR);
	else
		wxMessageBox("The chart has been saved to '" + filename + "'");
}
