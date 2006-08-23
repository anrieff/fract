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
#include "cpulistdlg.h"
#include "cpuid.h"
#include "cpu_list.h"

CPUListDlg::CPUListDlg(wxWindow *parent)
	: wxDialog(parent, -1, "CPU Selection", wxDefaultPosition, wxSize(500, 170))
{
	wxStaticText *txt1 = new wxStaticText(this, -1, 
		"Select your CPU (if unsure, use CPU-Z (www.cpuid.com/cpu-z))",
		wxPoint(6, 6));
	txt1->Refresh();
	const char *brand = cpu_brand_string();
	if (brand) {
		wxStaticText *txt2 = new wxStaticText(this, -1, 
			wxString::Format("Hint: Your CPU brand string is \"%s\"", brand),
			wxPoint(6, 26));
		txt2->Refresh();
	}
	
	wxStaticText *txt3 = new wxStaticText(this, -1, "CPU:", wxPoint(6, 66));
	wxArrayString cpus(cpu_list_size(), cpu_list);
	cpulist = new wxComboBox(this, -1, default_cpu, wxPoint(EndX(txt3) + 3, 64), 
				 wxSize(200,-1), cpus, wxCB_DROPDOWN);
	
	wxButton *okb = new wxButton(this, wxID_OK, "", wxPoint(150, 105), wxSize(90, 35));
	wxButton *cab = new wxButton(this, wxID_CANCEL, "", wxPoint(260, 105), wxSize(90, 35));
	int xx, yy;
	this->GetClientSize(&xx,&yy);
	yy -= okb->GetRect().height + 9;
	
	okb->SetSize(-1, yy, -1, -1, wxSIZE_USE_EXISTING);
	cab->SetSize(-1, yy, -1, -1, wxSIZE_USE_EXISTING);
}
