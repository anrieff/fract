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
#include <wx/gauge.h>
#include <wx/timer.h>

class SendDialog;
class SendThread: public wxThread {
	SendDialog *dlg;
public:
	volatile bool want_to_quit;
	SendThread(SendDialog *d) : dlg(d) { want_to_quit = false;}
	void* Entry();
	void DoWork(void);
	void setv(wxString, int, wxString);
};


struct SendDialog : public wxDialog {
	wxButton *m_sendbtn;
	wxStaticText *m_text1, *m_text2, *ht1, *ht2;
	wxGauge *gauge;
	wxString m_server, m_fn;
	wxTimer *m_timer;
	wxMutex lock;
	SendThread *trd;
	
	int m_port;
	char fbuff[1024];
	volatile int th_gval;
	wxString th_ht2;
	wxString th_text2;
	volatile int th_finished;
	
	SendDialog (wxWindow *parent, wxString server, int port, wxString fn);
	void OnSendBtnClick(wxCommandEvent&);
	void OnCancelBtnClick(wxCommandEvent&);
	void OnTimerTick(wxTimerEvent&);
	void OnTryClose(wxCloseEvent&);
	void Cleanup(void);
	
	DECLARE_EVENT_TABLE()
};

enum {
	bSendClick=161,
	bCancelClick,
	tTimer,
};

#endif // __SENDDIALOG_H__
