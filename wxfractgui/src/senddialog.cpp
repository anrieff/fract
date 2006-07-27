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
#include <wx/thread.h>

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
	EVT_BUTTON(bCancelClick, SendDialog:: OnCancelBtnClick)
	EVT_TIMER(tTimer, SendDialog::OnTimerTick)
	EVT_CLOSE(SendDialog::OnTryClose)
END_EVENT_TABLE()

SendDialog::SendDialog(wxWindow *parent, wxString server, int port, wxString fn)
	: wxDialog(parent, -1, "Send result", wxDefaultPosition, wxSize(480, 250))
{
	m_text1 = new wxStaticText(this, -1, 
		"This will submit your result for participation in http://fbench.com/results/\n"
		"NOTE: no personal information except your score will be submitted",
		wxPoint(10, 10));
	m_text2 = new wxStaticText(this, -1,
		"Direct connection to the internet is required. The sending might fail\n"
		"if you are behind a proxy, firewall or other type of non-transparent\n"
		"network element. In such cases, you can mail your .result file\n"
		"(plain attachment or zip/rar-ed) to the following address:\n\n"
		"skalaren_alpinist@abv.bg",
		wxPoint(10,  90));
	m_sendbtn = new wxButton(this, bSendClick, "&Send", wxPoint(140, 210), 
		wxSize(95, 30));
	wxButton *cb = new wxButton(this, bCancelClick, "&Cancel", 
		wxPoint(245, 210), wxSize(95, 30));
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

void SendThread::setv(wxString a, int b, wxString c)
{
	dlg->lock.Lock();
	dlg->th_ht2 = a;
	dlg->th_gval = b;
	dlg->th_text2 = c;
	dlg->lock.Unlock();
}

void SendThread::DoWork(void)
{
	#define FAIL(x) { setv("Failed", 5, x); return; }
	//
	// Step 1: Resolve IP address of server
	//
	
	// is it an IP?
	int useless[4];
	if (4 != sscanf(dlg->m_server.c_str(), "%d.%d.%d.%d", useless, useless+1, useless+2, useless+3))
	{
		setv("Resolving IP Address...", 1, "");
		//
		struct hostent *he = gethostbyname(dlg->m_server.c_str());
		if (he == NULL) 
			FAIL("Cannot resolve IP Address");
		unsigned x = ntohl(*(long*)he->h_addr_list[0]);
		char buff[20];
		sprintf(buff, "%u.%u.%u.%u", x >> 24, (x >> 16) & 0xff, (x >> 8) & 0xff, x & 0xff);
		dlg->m_server = buff;
	}
	
	
	//
	// Step 2: Create a socket
	//
	
	if (want_to_quit) return;
	TestDestroy();
	setv("Connecting...", 2, "");
	
	int fd = socket(PF_INET, SOCK_STREAM, 0);
	if (fd == -1)
		FAIL("Cannot create socket");
		
	//
	// Step 3: connect()
	//
	
	if (want_to_quit) return;
	TestDestroy();
	setv("Connecting...", 3, "");
	
	struct sockaddr_in sa;
	sa.sin_family = AF_INET;
	sa.sin_port = htons(dlg->m_port);
	sscanf(dlg->m_server.c_str(), "%d.%d.%d.%d", useless, useless+1, useless+2, useless+3);
	unsigned x = useless[3] | (useless[2] << 8) | (useless[1] << 16) | (useless[0] << 24);
	sa.sin_addr.s_addr = htonl(x);
	
	int res = connect(fd, (struct sockaddr *)&sa, sizeof(sa));
	
	if (res) 
		FAIL("Cannot connect to the server; it may be busy or\ndown right now - try again later");
	
	if (want_to_quit) return;
	TestDestroy();
	setv("Sending...", 4, "");
	
	//
	// Step 4: Send the result
	//
	
	int i, tosend = 1024;
	do {
		int r = send(fd, (dlg->fbuff)+i, tosend, 0);
		printf("Successfully sent %d bytes\n", r);
		if (r == -1) {
			perror("send()");
			FAIL("Sending failed; the server may be busy or\ndown right now - try again later");
		}
		i += r;
		tosend -= r;
		if (want_to_quit) return;
		TestDestroy();
	} while(tosend);
	
	setv("OK", 5, 
			"Your result was sent successfully. Please allow 15 minutes\n"
			"for your score to appear on the site");
	dlg->th_finished = 1;
	return;
}

void* SendThread::Entry()
{
	DoWork();
	dlg->th_finished = 1;
	return NULL;
}

void SendDialog::OnSendBtnClick(wxCommandEvent & )
{
	
	//char fbuff[1024];
	FILE *f = fopen(m_fn.c_str(), "rb");
	int r = fread(fbuff, 1, 1024, f);
	fclose(f);
	if (r != 1024) {
		wxMessageBox("The result file is incomplete or corrupted", "Error", wxICON_ERROR);
		return;
	}
	
	ht1 = new wxStaticText(this, -1, "Status:", wxPoint(15, 50));
	wxFont txtFont = ht1->GetFont();
	txtFont.SetWeight(wxFONTWEIGHT_BOLD);
	ht1->SetFont(txtFont);
	ht2 = new wxStaticText(this, -1, "Initializing...", wxPoint(EndX(ht1)+5, 50));
	ht2->Refresh();
	m_sendbtn->Disable();
	m_text2->SetLabel("");
	gauge = new wxGauge(this, -1, 5, wxPoint(10, 70), wxSize(420, 15));
	
	th_ht2 = "Initializing...";
	th_text2 = "";
	th_gval = 0;
	th_finished = 0;
	m_timer = new wxTimer(this, tTimer);
	m_timer->Start(150);
	
	trd = new SendThread(this);
	trd->Create();
	trd->Run();
}

void SendDialog::OnCancelBtnClick(wxCommandEvent &)
{
	Cleanup();
	EndModal(0);
}

void SendDialog::OnTimerTick(wxTimerEvent &)
{
	lock.Lock();
	ht2->SetLabel(th_ht2);
	m_text2->SetLabel(th_text2);
	gauge->SetValue(th_gval);
	lock.Unlock();
}

void SendDialog::Cleanup(void)
{
	if (!th_finished) {
		trd->want_to_quit = true;
	}
	m_timer->Stop();
	delete m_timer;
}

void SendDialog::OnTryClose(wxCloseEvent&)
{
	Cleanup();
}
