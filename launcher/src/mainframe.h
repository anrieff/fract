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

#ifndef __MAINFRAME_H__
#define __MAINFRAME_H__
#include <wx/wx.h>
#include <wx/notebook.h>
#include <wx/propdlg.h>
#include <wx/generic/propdlg.h>

class Tabbed : public wxNotebook
{
public:
	Tabbed(wxWindow * parent, int id, const wxPoint & p, const wxSize & s, wxTextCtrl *rl);
};

class MainFrame : public wxFrame {
	wxColour BGCol;
	wxTextCtrl *run_line;
	wxButton *run_button;
	wxComboBox *run_prog;
	Tabbed *tabbed;
	bool creating_tabbed;
	DECLARE_EVENT_TABLE();
public:
	MainFrame(const wxString &, const wxPoint &, const wxSize &);
	void TabChanged(wxNotebookEvent &);
	void RunPressed(wxCommandEvent &);
};

enum {
	myTextCtrl,
	myRunButton,
	myTabbedCtrl
};


#endif
