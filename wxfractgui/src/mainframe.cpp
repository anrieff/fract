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
 
#include <stdlib.h>
 
#ifndef __WIN32__
#include "f_icon2.xpm"
#endif
#include "mainframe.h"
#include "fract_specific.h"
#include "fract_logo.xpm"

#include "generictab.h"
#include "basictab.h"
#include "advancedtab.h"
#ifdef _MSC_VER
#include "resource.h"
#endif


BEGIN_EVENT_TABLE(MainFrame, wxFrame)
		EVT_NOTEBOOK_PAGE_CHANGED(myTabbedCtrl, MainFrame:: TabChanged)
		EVT_BUTTON(myRunButton, MainFrame:: RunPressed)
END_EVENT_TABLE()



int EndX(wxWindow * w)
{
	wxRect r = w->GetRect();
	return r.x + r.width;
}

MainFrame::MainFrame(const wxString & title, 
		     const wxPoint  & pos, 
		     const wxSize   & size)
	: wxFrame((wxFrame *)NULL, wxID_ANY, title, pos, size,
		   wxDEFAULT_FRAME_STYLE & ~wxMAXIMIZE_BOX & ~wxRESIZE_BORDER)
{
	SetIcon(wxICON(f_icon2));
	wxPanel *panel = new wxPanel(this);
// an ugly hack to use no panel even under windoze :)
	BGCol = panel->GetBackgroundColour();
	panel->Destroy();
	SetOwnBackgroundColour(BGCol);
	SetBackgroundColour(BGCol);
	wxStaticText *tex1 = new wxStaticText(this, -1, "Fract version:", 
					      wxPoint(10, 13));
	wxFont txtFont = tex1->GetFont();
	txtFont.SetWeight(wxFONTWEIGHT_BOLD);
	tex1->SetFont(txtFont);
	
	tex1->Disable();
	
	wxStaticText *ver = new wxStaticText(this, -1, get_version(),
					     wxPoint(10, 32));
	ver->Disable();
	
	wxStaticText *tex2 = new wxStaticText(this, -1, "Renderer:",
					      wxPoint(10, 58));
	tex2->SetFont(txtFont);
	tex2->Disable();
	wxStaticText *sset = new wxStaticText(this, -1, have_SSE2()?"SSE2":"SSE",
					      wxPoint(10, 77));
	sset->Disable();
	
	wxBitmap bmp(fract_logo_xpm);
	int cw, ch;
	GetClientSize(&cw, &ch);
	wxStaticBitmap *static_bmp = new wxStaticBitmap (this, -1, bmp, 
			wxPoint(cw/2-102, 13));
	static_bmp->Refresh();
	
	run_prog = NULL;
	wxWindow *align_wnd;
	if (have_SSE2()) {
		wxArrayString a;
		a.Add("fract_p5");
		a.Add("fract_sse2");
		run_prog = new wxComboBox(this, -1, "fract_sse2", 
					  wxPoint(10, ch-100), wxSize(100, -1),
					  a, wxCB_READONLY | wxCB_DROPDOWN);
		align_wnd = run_prog;
	}
	else {
		wxStaticText *cl = new wxStaticText(this, -1, "Run arguments:",
					    wxPoint(10, ch - 100));
		align_wnd = cl;
	}
	
	
	run_line = new wxTextCtrl(this, myTextCtrl, "", 
				  wxPoint(EndX(align_wnd)+5, ch-100), 
				  wxSize(cw -15 - EndX(align_wnd), 22));
	run_button = new wxButton(this, myRunButton, "&Run Fract", 
				  wxPoint(cw / 2 - 100, ch - 70),
				  wxSize(200, 60));
	/* 10, 128 */	
	tabbed = new Tabbed(this, myTabbedCtrl, wxPoint(10, 128),
			    wxSize(cw - 20, ch - 108 - 128), run_line);
}

Tabbed::Tabbed(wxWindow * parent, int id, const wxPoint & p, const wxSize & s, wxTextCtrl *rl) :
		wxNotebook(parent, id, p, s)
{
	wxPanel *wnd1 = new BasicTab(this, rl);
	wxPanel *wnd2 = new AdvancedTab(this, rl);
	AddPage(wnd1, "Basic Options", true);
	AddPage(wnd2, "Advanced Options", false);
	GenericTab *current = dynamic_cast<GenericTab*>(GetCurrentPage());
	current->RefreshCmdLine();
}


void MainFrame::TabChanged(wxNotebookEvent & event)
{
#ifdef __WIN32__
	static bool firstchange = true;
	if (firstchange) {
		firstchange = false;
		return;
	}
#endif
	GenericTab *current = dynamic_cast<GenericTab*>(tabbed->GetCurrentPage());
	current->RefreshCmdLine();
}

void MainFrame::RunPressed(wxCommandEvent &)
{
	GenericTab *current = dynamic_cast<GenericTab*>(tabbed->GetCurrentPage());
	if (!current->RunPressed()) return;
	char cmd[1000];
	strcpy(cmd,"");
#if defined linux || defined __linux__
	strcat(cmd,"./");
#endif	
	if (run_prog && run_prog->GetValue() == "fract_sse2")
		strcat(cmd, "fract_sse2 ");
	else
		strcat(cmd, "fract_p5 ");
	strcat(cmd, run_line->GetValue().c_str());
	system(cmd);
}
