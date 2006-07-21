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
#include <wx/xml/xml.h>
#include <wx/grid.h>
#include <vector>

int file_count(const char *filter);

enum Status {
	STATUS_NORMAL,
	STATUS_MY,
	STATUS_HOT
};

class FConfig {
	std::vector<wxString> par;
	std::vector<wxString> val;
public:
	void refresh(void);
	const char* operator [] (const wxString& s) const;
};

extern FConfig cfg;

struct ResultNode {
	ResultNode() { defaults(); }
	void defaults(void) {
		sys_desc = "";
		res_file = "";
		fps = 0.0;
		cpu_mhz = 1;
		mem_mhz = 0;
		status = STATUS_NORMAL;
	}
	
	// the actual data:
	wxString sys_desc;
	wxString res_file;
	float fps;
	int cpu_mhz, mem_mhz;
	Status status;
	
	bool operator < (const ResultNode &r ) const { return fps > r.fps; }
};

class ResultXml {
	std::vector<ResultNode> m_data;
	wxString m_fn;
public:
	ResultXml(wxString file_name);
	void load(void);
	void save(void);
	void add_entry(const ResultNode &re);
	int get_date(void);
	
	const ResultNode& operator [] (int index) const;
	ResultNode& operator [] (int index);
	int size() const;
};

class ResultBrowser : public GenericTab {
	wxButton *m_sendbutton; 
	ResultXml *builtin, *my;
	wxGrid *m_grid;
	
	void OnBtnSendResult(wxCommandEvent &);
public:
	ResultBrowser(wxWindow *parent, wxTextCtrl *cmdline);
	~ResultBrowser();
	void CreateColorBox(wxPoint pos, wxSize size, wxColour color);
	void AddLastResult(void);
	void RefreshCmdLine(void);
	bool RunPressed(void);
	bool CanRun(void);
	void UpdateGrid(void);
	void AddResults(ResultXml *, std::vector<ResultNode>&);
	void DisplayResults(std::vector<ResultNode>&);
	
	DECLARE_EVENT_TABLE()
};

enum {
	bSendResult=100,
	gGrid,
};

#endif
