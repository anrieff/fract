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

#include "resbrowser.h"


BEGIN_EVENT_TABLE(ResultBrowser, GenericTab)
	EVT_BUTTON(bSendResult, ResultBrowser:: OnBtnSendResult)
END_EVENT_TABLE()

void add_last_result(void)
{
}

int file_count(const char *filter)
{
	wxString sx = wxFindFirstFile(filter);
	int c = 0;
	while (!sx.empty())
	{
		c++;
		sx = wxFindNextFile();
	}
	return c;
}

/**
 * @class ResultBrowser
*/ 

ResultBrowser::ResultBrowser(wxWindow *parent, wxTextCtrl *cmdline) : GenericTab(parent, cmdline)
{
	m_maintext = new wxStaticText(this, -1, "Bla bla bla", wxPoint(30, 50));
	m_sendbutton = new wxButton(this, bSendResult, "&Send Result", wxPoint(120, 50), wxSize(100, 40));
}

void ResultBrowser::OnBtnSendResult(wxCommandEvent &)
{
	wxMessageBox("The result is being sent");
}

void ResultBrowser::RefreshCmdLine(void)
{
	cmd->SetValue("");
}

bool ResultBrowser::RunPressed(void)
{
	return false;
}

bool ResultBrowser::CanRun(void)
{
	return false;
}

