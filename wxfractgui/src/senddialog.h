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

#ifndef __SENDDIALOG_H__
#define __SENDDIALOG_H__

#include <wx/wx.h>

class SendDialog : public wxDialog {
	wxButton *m_sendbtn;
	wxStaticText *m_text1, *m_text2;
	wxString m_server, m_fn;
	int m_port;
public:
	SendDialog (wxWindow *parent, wxString server, int port, wxString fn);
	void OnSendBtnClick(wxCommandEvent&);
	
	DECLARE_EVENT_TABLE()
};

enum {
	bSendClick=161
};

#endif // __SENDDIALOG_H__
