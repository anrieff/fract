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

#include "senddialog.h"
#include <wx/gauge.h>

#ifdef _WIN32
#	include <Winsock2.h>
#else
#	include <netdb.h>
#	include <sys/socket.h>
#	include <netinet/in.h>
#	include <netinet/tcp.h>
#endif

BEGIN_EVENT_TABLE(SendDialog, wxDialog)
	EVT_BUTTON(bSendClick, SendDialog:: OnSendBtnClick)
END_EVENT_TABLE()

SendDialog::SendDialog(wxWindow *parent, wxString server, int port, wxString fn)
	: wxDialog(parent, -1, "Send result", wxDefaultPosition, wxSize(450, 250))
{
	m_text1 = new wxStaticText(this, -1, 
		"This will submit your result for participation in http://fbench.com/results/\n"
		"NOTE: no personal information except your score will be submitted",
		wxPoint(10, 10));
	m_text2 = new wxStaticText(this, -1,
		"Direct connection to the internet is required. The sending might fail\n"
		"if you are behind a proxy, firewall or other type of non-transparent\n"
		"network element. In such cases, you can mail your .result file\n"
		"(plain attachment or .zip/.rar-ed) to the following address:\n\n"
		"skalaren_alpinist@abv.bg",
		wxPoint(10,  90));
	m_sendbtn = new wxButton(this, bSendClick, "&Send", wxPoint(125, 210), 
		wxSize(95, 30));
	wxButton *cb = new wxButton(this, wxID_CANCEL, "&Cancel", 
		wxPoint(230, 210), wxSize(95, 30));
	cb->Refresh();
	
	m_server = server;
	m_port = port;
	m_fn = fn;
}

static int EndX(wxWindow * w)
{
	wxRect r = w->GetRect();
	return r.x + r.width;
}


void SendDialog::OnSendBtnClick(wxCommandEvent & )
{
	#define FAIL(x) { gauge->SetValue(5); ht2->SetLabel("Failed"); m_text2->SetLabel(x); m_text2->Show(); return; }
	m_sendbtn->Disable();
	m_text2->Hide();
	
	wxStaticText *ht1 = new wxStaticText(this, -1, "Status:", wxPoint(15, 40));
	wxFont txtFont = ht1->GetFont();
	txtFont.SetWeight(wxFONTWEIGHT_BOLD);
	ht1->SetFont(txtFont);
	
	wxStaticText *ht2 = new wxStaticText(this, -1, "Initializing...", wxPoint(EndX(ht1), 40));
	ht2->Refresh();
	
	wxGauge *gauge = new wxGauge(this, -1, 5, wxPoint(10, 60), wxSize(420, 15));
	
	//
	// Step 1: Resolve IP address of server
	//
	
	// is it an IP?
	int useless[4];
	if (4 != sscanf(m_server.c_str(), "%d.%d.%d.%d", useless, useless+1, useless+2, useless+3))
	{
		ht2->SetLabel("Resolving IP Address...");
		gauge->SetValue(1);
		//
		struct hostent *he = gethostbyname(m_server.c_str());
		if (he == NULL) 
			FAIL("Cannot resolve IP Address");
		unsigned x = ntohl(*(long*)he->h_addr_list[0]);
		char buff[20];
		sprintf(buff, "%u.%u.%u.%u", x >> 24, (x >> 16) & 0xff, (x >> 8) & 0xff, x & 0xff);
		m_server = buff;
	}
	
	gauge->SetValue(2);
	ht2->SetLabel("Connecting...");
	
	int fd = socket(PF_INET, SOCK_STREAM, 0);
	if (fd == -1)
		FAIL("Cannot create socket");
		
	struct sockaddr sa;
	
	//int res = connect(fd, 
	
}
