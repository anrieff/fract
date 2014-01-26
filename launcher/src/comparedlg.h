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

#ifndef __COMPAREDLG_H__
#define __COMPAREDLG_H__

#include <wx/wx.h>
#include <wx/control.h>
#include "fonts.h"

struct CompareInfo {
	wxString name;
	float fps;
	int mhz;
};

class FractChart {
	int xr, yr;
	unsigned *drawbuff, drawcol;
	void draw_line(int,int,int,int);
	void draw_chart(CompareInfo a[], int n, float (*fun) (CompareInfo&), int sx, int sy, int sizex, int sizey, FontMan &fm);
	void render(CompareInfo a[], int n);
public:
	FractChart(wxWindow *parent, int id, CompareInfo *, int count, wxPoint pos, wxSize size);
	~FractChart();
	static wxSize get_needed_area(int how_many_results);
	bool save_chart(wxString filename);
};

class CompareDialog : public wxDialog {
	FractChart *fc;
	wxButton *m_savebut;
	
	void OnSaveChart(wxCommandEvent &);
public:
	CompareDialog(wxWindow *parent, CompareInfo *, int count);
	
	DECLARE_EVENT_TABLE()
};

enum {
	fcChart = 155,
	bSave
};

#endif // __COMPAREDLG_H__
