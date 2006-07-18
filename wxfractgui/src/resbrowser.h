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

#ifndef __RESBROWSER_H__
#define __RESBROWSER_H__

#include "generictab.h"

void add_last_result(void);
int file_count(const char *filter);

class ResultBrowser : public GenericTab {
	wxStaticText *m_maintext;
	wxButton *m_sendbutton; 
	
	void OnBtnSendResult(wxCommandEvent &);
public:
	ResultBrowser(wxWindow *parent, wxTextCtrl *cmdline);
	void RefreshCmdLine(void);
	bool RunPressed(void);
	bool CanRun(void);
	
	DECLARE_EVENT_TABLE()
};

enum {
	bSendResult=100
};

#endif
