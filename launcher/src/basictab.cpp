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

#include "basictab.h"
#include "countries_list.h"
#include "cpuid.h"
#include "global.h"
#include "cpulistdlg.h"

#include <wx/utils.h>

BEGIN_EVENT_TABLE(BasicTab, GenericTab)
	EVT_BUTTON(btCPUSelButton, BasicTab::OpenCPUSelectionDialog)
END_EVENT_TABLE()

void BasicTab::RefreshCmdLine(void)
{
	cmd->SetValue("--official");
}

char *lck(char *s)
{
	if (s[strlen(s)-1]=='\n') s[strlen(s)-1]=0;
	return s;
}


BasicTab::BasicTab(wxWindow *parent, wxTextCtrl *cmdline)
	:GenericTab(parent, cmdline)
{
	wxPanel *panel = new wxPanel(this);
// an ugly hack to use no panel even under windoze :)
	wxColour BGCol = panel->GetBackgroundColour();
	panel->Destroy();
	SetOwnBackgroundColour(BGCol);
	SetBackgroundColour(BGCol);

	int cw, ch;
	parent->GetClientSize(&cw, &ch);
	//
	wxStaticText *un = new wxStaticText(this, -1, "Username:",
					    wxPoint(16, 16));
	username = new wxTextCtrl(this, -1, "", wxPoint(EndX(un)+5, 13),
				  wxSize(160, -1));
	username->SetMaxLength(31);
	wxStaticText *ctry = new wxStaticText(this, -1, "Country:",
					      wxPoint(EndX(username) + 10, 16));
	wxArrayString clist(countries_list_size(), countries_list);
	country = new wxComboBox(this, -1, default_country, 
				 wxPoint(EndX(ctry) + 5, 13),
				 wxSize(200, -1), clist, wxCB_DROPDOWN | 
				wxCB_READONLY | wxCB_SORT);
	
	
	wxStaticText *cpusel = new wxStaticText(this, -1, "CPU:", wxPoint(16, 48));
	cputype = new wxTextCtrl(this, -1, identify_cpu(),
				 wxPoint(EndX(cpusel) + 5, 45),
				 wxSize(200, -1));
	cputype->SetMaxLength(31);
	wxButton *cpu_sel_button = new wxButton(this, btCPUSelButton, "...",
			wxPoint(EndX(cputype) + 3, 45), wxSize(30, cputype->GetRect().height));
	wxStaticText *chiptxt = new wxStaticText(this, -1, "Chipset:",
			wxPoint(EndX(cpu_sel_button)+20, 48));
	chipset = new wxTextCtrl(this, -1, "Unknown", wxPoint(EndX(chiptxt)+5, 45),
				 wxSize(110,-1));
	wxStaticText *comtxt = new wxStaticText(this, -1, "Comment:",
			wxPoint(16, 70));
	comtxt->Refresh();
	comment = new wxTextCtrl(this, -1, "", wxPoint(16, 90), wxSize(cw-32, ch-144),
				 wxTE_MULTILINE);
	
	if (wxFileExists(USER_COMMENT_FILE)) {
		comment->LoadFile(USER_COMMENT_FILE);
	}
	FILE *f;
	f = fopen(USER_INFO_FILE, "rt");
	if (f) {
		char line[64];
		fgets(line, 64, f);
		line[31] = 0;
		username->SetValue(lck(line));
		fgets(line, 64, f);
		line[31] = 0;
		cputype->SetValue(lck(line));
		fgets(line, 64, f);
		line[15] = 0;
		country->SetValue(lck(line));
		fgets(line, 64, f);
		line[15] = 0;
		chipset->SetValue(lck(line));
		
		fclose(f);
	} else {
		if (strstr(cputype->GetValue().c_str(), "Unknown")) {
			wxMessageBox(
				"Your CPU was not detected successfully :(\n"
				"This means that either you CPU is very new, or there is a bug in the program.\n"
				"You can still enter your CPU in the provided box, or you can choose it from\n"
				"the list (click on the \"...\" button for that)\n\n", "Warning", wxICON_INFORMATION);
		}
	}
}

bool BasicTab::RunPressed(void)
{
	FILE *f;
	f = fopen(USER_INFO_FILE, "wt");
	if (!f) {
		wxMessageBox(wxString::Format("Cannot save user info to text file `%s'.\nCheck if you have write permissions in this directory!", USER_INFO_FILE), "Error", wxICON_ERROR);
		return false;
	}
	fprintf(f, "%s\n", username->GetValue().c_str());
	fprintf(f, "%s\n", cputype->GetValue().c_str());
	fprintf(f, "%s\n", country->GetValue().c_str());
	fprintf(f, "%s\n", chipset->GetValue().c_str());
	fclose(f);
	comment->SaveFile(USER_COMMENT_FILE);
	return true;
}

bool BasicTab::CanRun(void)
{
	return true;
}

void BasicTab::OpenCPUSelectionDialog(wxCommandEvent&)
{
	CPUListDlg *dlg = new CPUListDlg(this);
	if (wxID_OK == dlg->ShowModal() )
	{
		cputype->SetValue(dlg->cpulist->GetValue());
	}
	dlg->Destroy();
}
