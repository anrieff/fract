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
#include <time.h>
#include <algorithm>
using namespace std;

const int COLCNT = 5;

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

int isleap(int year)
{
	if (0 == year % 400) return 1;
	if (0 == year % 100) return 0;
	if (0 == year % 4  ) return 1;
	return 0;
}

int monthlen(int mon, int year)
{
	const int mlen[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	int r = mlen[mon];
	if (isleap(year) && mon == 1) r ++;
	return r;
}

int absdate(int year, int month, int day)
{
	int r = 0;
	for (int i = 1900; i < year; i++)
		r += 356 + isleap(i);
	for (int i = 0; i < month; i++)
		r += monthlen(i, year);
	r += day;
	return r;
}

int get_current_date(void)
{
	time_t t;
	time(&t);
	struct tm *tt = localtime(&t);
	return absdate(tt->tm_year+1900, tt->tm_mon, tt->tm_mday);
}

int get_int(const wxString& s)
{
	int r;
	sscanf(s.c_str(), "%d", &r);
	return r;
}

float get_float(const wxString& s)
{
	float f;
	sscanf(s.c_str(), "%f", &f);
	return f;
}

wxString transform(wxString s)
{
	wxString r;
	int acc = 0;
	for (unsigned i = 0; i < s.length(); i++) {
		r += s[i];
		if (acc > 19 && s[i] == ' ') {
			r += '\n';
			acc = 0;
		}
		++acc;
		
	}
	return r;
}

/**
 * @class ResultXml
*/
ResultXml::ResultXml(wxString file_name)
{
	m_loaded = false;
	m_fn = file_name;
} 

void ResultXml::load(void)
{
	if (!m_xml.Load(m_fn)) return;
	m_loaded = true;
	wxXmlNode *root = m_xml.GetRoot();
	
	wxXmlNode *p = root->GetChildren();
	while (p) {
		if (p->GetName() == "results") {
			wxXmlNode *q = p->GetChildren();
			while (q) {
				if (q->GetName() == "result") {
					ResultNode res;
					wxXmlNode *t = q->GetChildren();
					while (t) {
						const wxString name = t->GetName();
						if (name == "description")
							res.sys_desc = t->GetChildren()->GetContent();
						if (name == "score") {
							res.fps = get_float(t->GetChildren()->GetContent());
						}
						if (name == "cpu_mhz") {
							res.cpu_mhz = get_int(t->GetChildren()->GetContent());
						}
						if (name == "mem_mhz") {
							res.mem_mhz = get_int(t->GetChildren()->GetContent());
						}
						t = t->GetNext();
					}
					m_data.push_back(res);
				}
				q = q->GetNext();
			}
		}
		p = p->GetNext();
	}
}

int ResultXml::get_date(void)
{
	if (!m_loaded)
		return 0;
	wxXmlNode *root = m_xml.GetRoot();
	
	wxXmlNode *p = root->GetChildren();
	while (p) {
		if (p->GetName() == "date") {
			wxString s = p->GetChildren()->GetContent();
			int yy, mm, dd;
			for (unsigned i = 0; i < s.length(); i++) 
				if (s[i] < '0' || s[i] > '9') 
					s[i] = ' ';
			sscanf(s.c_str(), "%d%d%d", &yy, &mm, &dd);
			return absdate(yy, mm-1, dd);
		}
		p = p->GetNext();
	}
	return 0;
}

int ResultXml::size() const 
{
	return m_data.size();
}

ResultNode ResultXml::operator [] (int index) const
{
	return m_data[index];
}

/**
 * @class ResultBrowser
*/ 

ResultBrowser::ResultBrowser(wxWindow *parent, wxTextCtrl *cmdline) : GenericTab(parent, cmdline)
{
	m_sendbutton = new wxButton(this, bSendResult, "&Send Result", wxPoint(510, 50), wxSize(100, 30));
	
	builtin = my = NULL;
	if (wxFileExists("db.xml"))
		builtin = new ResultXml("db.xml");
	else if (wxFileExists("data/db.xml"))
		builtin = new ResultXml("data/db.xml");
	
	
	if (wxFileExists("my.xml"))
		my = new ResultXml("my.xml");
		
	if (builtin) builtin->load();
	if (my) my->load();
	
	if (builtin && get_current_date() - builtin->get_date() > 20) {
		wxStaticText* old_warning = new wxStaticText(this, -1, 
		"Your result database (db.xml) is more than 20 days old\n"
		"Get a fresh copy from http://fbench.com/", wxPoint(20, 325));
		old_warning->Refresh();
	}
	
	m_grid = new wxGrid(this, gGrid, wxPoint(20, 20), wxSize(480, 300));
	
	m_grid->CreateGrid(0, COLCNT);
	m_grid->SetSelectionMode(wxGrid::wxGridSelectRows);
	m_grid->SetRowLabelSize(15);
	m_grid->SetDefaultRowSize(46);
	m_grid->SetColSize(0, 180);
	m_grid->SetColSize(1, 72);
	m_grid->SetColSize(2, 76);
	m_grid->SetColSize(3, 72);
	m_grid->SetColSize(4, 56);
	m_grid->SetDefaultCellAlignment(wxALIGN_CENTRE, wxALIGN_CENTRE);
	m_grid->SetColLabelValue(0, "Description");
	m_grid->SetColLabelValue(1, "Score");
	m_grid->SetColLabelValue(2, "CPU");
	m_grid->SetColLabelValue(3, "Memory");
	m_grid->SetColLabelValue(4, "Select");
	m_grid->SetColFormatBool(4);
	
	UpdateGrid();
}

void ResultBrowser::UpdateGrid(void)
{
	m_grid->ClearGrid();
	vector<ResultNode> all;
	if (builtin) AddResults(builtin, all);
	if (my) AddResults(my, all);
	DisplayResults(all);
}

void ResultBrowser::AddResults(ResultXml *file, vector<ResultNode> & all)
{
	for (int i = 0; i < file->size(); i++)
		all.push_back((*file)[i]);
}

void ResultBrowser::DisplayResults(vector<ResultNode> & results)
{
	if (results.empty()) return;
	sort(results.begin(), results.end());
	m_grid->AppendRows(results.size());
	
	for (unsigned i = 0; i < results.size(); i++) {
		ResultNode &r = results[i];
		wxColour col;
		if (r.status == STATUS_NORMAL) col = wxColour(0xffffff);
		if (r.status == STATUS_MY) col = wxColour(0xb2daef);
		if (r.status == STATUS_HOT) col = wxColour(0xefc3b2);
		for (int j = 0; j < COLCNT; j++) {
			m_grid->SetCellBackgroundColour(i, j, col);
			if (j != COLCNT - 1)
				m_grid->SetReadOnly(i, j);
		}
		m_grid->SetCellValue(i, 0, transform(r.sys_desc));
		m_grid->SetCellValue(i, 1, wxString::Format("%.2f FPS", r.fps));
		m_grid->SetCellValue(i, 2, wxString::Format("%d MHz", r.cpu_mhz));
		m_grid->SetCellValue(i, 3, wxString::Format("%d MHz", r.mem_mhz));
		m_grid->SetCellEditor(i, 4, new wxGridCellBoolEditor);
		m_grid->SetRowLabelValue(i, "");
	}
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

