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
#include "comparedlg.h"
#include "senddialog.h"
#include <time.h>
#include <algorithm>
using namespace std;

const int COLCNT = 5;
FConfig cfg;

BEGIN_EVENT_TABLE(ResultBrowser, GenericTab)
	EVT_BUTTON(bSendResult, ResultBrowser:: OnBtnSendResult)
	EVT_BUTTON(bCompare, ResultBrowser::OnBtnCompare)
	EVT_GRID_CELL_LEFT_CLICK(ResultBrowser::OnGridClick)
END_EVENT_TABLE()


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

static int isleap(int year)
{
	if (0 == year % 400) return 1;
	if (0 == year % 100) return 0;
	if (0 == year % 4  ) return 1;
	return 0;
}

static int monthlen(int mon, int year)
{
	const int mlen[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	int r = mlen[mon];
	if (isleap(year) && mon == 1) r ++;
	return r;
}

static int absdate(int year, int month, int day)
{
	int r = 0;
	for (int i = 1900; i < year; i++)
		r += 356 + isleap(i);
	for (int i = 0; i < month; i++)
		r += monthlen(i, year);
	r += day;
	return r;
}

static int get_current_date(void)
{
	time_t t;
	time(&t);
	struct tm *tt = localtime(&t);
	return absdate(tt->tm_year+1900, tt->tm_mon, tt->tm_mday);
}

static int get_int(const wxString& s)
{
	int r;
	sscanf(s.c_str(), "%d", &r);
	return r;
}

static float get_float(const wxString& s)
{
	float f;
	sscanf(s.c_str(), "%f", &f);
	return f;
}

static wxString read_trim_file(wxString fn)
{
	wxString res;
	char buff[1024];
	FILE *f = fopen(fn.c_str(), "rt");
	if (!f) return res;
	while (fgets(buff, 1024, f)) {
		for (unsigned i = 0; i < strlen(buff); i++)
		{
			switch (buff[i]) {
				case '\t':
				case '\n': res += ' '; break;
				case '\r':
					break;
				default:
					if (buff[i] >= 32)
						res += buff[i];
			}
		}
	}
	fclose(f);
	return res;
}

static wxString transform(wxString s)
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
 * @class FConfig
*/

void FConfig::refresh(void)
{
	par.clear(); val.clear();
	char buff[2048];
	FILE *f = fopen("fract.cfg", "rt");
	if (!f) return;
	while (fgets(buff, 2048, f)) {
		int i = strlen(buff)-1;
		while (i && (buff[i] == '\n' || buff[i] == '\r')) buff[i--] = 0;
		while (i > 0 && buff[i] != '=') i--;
		buff[i++] = 0;
		par.push_back(wxString(buff));
		val.push_back(wxString(&buff[i]));
	}
	fclose(f);
}

const char* FConfig::operator [] (const wxString & x) const
{
	for (unsigned i = 0; i < par.size(); i++)
		if (par[i] == x)
			return val[i].c_str();
	return NULL;
}

/**
 * @class ResultXml
*/
ResultXml::ResultXml(wxString file_name)
{
	m_fn = file_name;
} 

void ResultXml::load(void)
{
	wxXmlDocument xml;
	if (!xml.Load(m_fn)) return;
	wxXmlNode *root = xml.GetRoot();
	
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
						if (name == "res_file") {
							res.res_file = t->GetChildren()->GetContent();
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

static void addx(wxXmlNode *n, wxString caption, wxString data)
{
	wxXmlNode *p = new wxXmlNode(wxXML_ELEMENT_NODE, caption);
	wxXmlNode *q = new wxXmlNode(wxXML_TEXT_NODE, "", data);
	p->AddChild(q);
	n->AddChild(p);
}

void ResultXml::save(void)
{
	wxXmlDocument xml;
	wxXmlNode *results = new wxXmlNode(wxXML_ELEMENT_NODE, "results");
	for (unsigned i = 0; i < m_data.size(); i++) {
		wxXmlNode *result = new wxXmlNode(wxXML_ELEMENT_NODE, "result");
		ResultNode& r = m_data[i];
		addx(result, "description", r.sys_desc);
		addx(result, "score", wxString::Format("%.2f", r.fps));
		addx(result, "cpu_mhz", wxString::Format("%d", r.cpu_mhz));
		addx(result, "mem_mhz", wxString::Format("%d", r.mem_mhz));
		addx(result, "res_file", r.res_file);
		results->AddChild(result);
	}
	wxXmlNode *root = new wxXmlNode(wxXML_ELEMENT_NODE, "FractDB");
	root->AddChild(results);
	xml.SetRoot(root);
	xml.Save(m_fn);
}

int ResultXml::get_date(void)
{
	wxXmlDocument xml;
	if (!xml.Load(m_fn)) return 0;
	wxXmlNode *root = xml.GetRoot();
	
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

ResultNode& ResultXml::operator [] (int index)
{
	return m_data[index];
}

const ResultNode& ResultXml::operator [] (int index) const
{
	return m_data[index];
}

void ResultXml::add_entry(const ResultNode& rn)
{
	for (unsigned i = 0; i < m_data.size(); i++)
		if (m_data[i].res_file == rn.res_file) return;
	for (unsigned i = 0; i < m_data.size(); i++)
		m_data[i].status = STATUS_MY;
	m_data.push_back(rn);
}

/**
 * @class ResultBrowser
*/ 

ResultBrowser::ResultBrowser(wxWindow *parent, wxTextCtrl *cmdline) : GenericTab(parent, cmdline)
{
	cfg.refresh();
	m_sendbutton = new wxButton(this, bSendResult, "&Send Result", wxPoint(510, 50), wxSize(100, 30));
	m_compare = new wxButton(this, bCompare, "&Compare", wxPoint(510, 90), wxSize(100, 30));
	m_compare->Disable();
	
	wxStaticBox *sbLegend = new wxStaticBox(this, -1, "Legend", wxPoint(510, 200), wxSize(100, 120));
	wxStaticText *stWhite = new wxStaticText(this, -1, "Stock", wxPoint(540, 230));
	wxStaticText *stBlue = new wxStaticText(this, -1, "Your", wxPoint(540, 260));
	wxStaticText *stRed = new wxStaticText(this, -1, "Last", wxPoint(540, 290));
	sbLegend->Refresh(); stWhite->Refresh(); stBlue->Refresh(); stRed->Refresh();
	CreateColorBox(wxPoint(515, 230), wxSize(20, 20), wxColour(0xffffff));
	CreateColorBox(wxPoint(515, 260), wxSize(20, 20), wxColour(0xefdab2));
	CreateColorBox(wxPoint(515, 290), wxSize(20, 20), wxColour(0xb2c3ef));
	
	builtin = my = NULL;
	if (wxFileExists("db.xml"))
		builtin = new ResultXml("db.xml");
	else if (wxFileExists("data/db.xml"))
		builtin = new ResultXml("data/db.xml");
	
	
	if (wxFileExists("my.xml"))
		my = new ResultXml("my.xml");
		
	if (builtin) builtin->load();
	if (my) {
		my->load();
		for (int i = 0; i < my->size(); i++)
			(*my)[i].status = STATUS_MY;
	}
	
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
	m_grid->SetDefaultRowSize(38);
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
	m_last_displayed_results = all;
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
	m_grid->DeleteRows(0, m_grid->GetNumberRows());
	m_grid->AppendRows(results.size());
	
	for (unsigned i = 0; i < results.size(); i++) {
		ResultNode &r = results[i];
		wxColour col;
		if (r.status == STATUS_NORMAL) col = wxColour(0xffffff);
		if (r.status == STATUS_MY) col = wxColour(0xefdab2);
		if (r.status == STATUS_HOT) col = wxColour(0xb2c3ef);
		for (int j = 0; j < COLCNT; j++) {
			m_grid->SetCellBackgroundColour(i, j, col);
			m_grid->SetReadOnly(i, j);
		}
		m_grid->SetCellValue(i, 0, transform(r.sys_desc));
		m_grid->SetCellValue(i, 1, wxString::Format("%.2f FPS", r.fps));
		m_grid->SetCellValue(i, 2, wxString::Format("%d MHz", r.cpu_mhz));
		m_grid->SetCellValue(i, 3, r.mem_mhz?wxString::Format("%d MHz", r.mem_mhz):"N/A");
		m_grid->SetCellEditor(i, 4, new wxGridCellBoolEditor);
		m_grid->SetRowLabelValue(i, "");
	}
}

void ResultBrowser::CreateColorBox(wxPoint p, wxSize s, wxColour c)
{
	//wxStaticBitmap *b = new wxStaticBitmap(this, -1, "yo", p, s);
	wxImage img(s.x, s.y);
	for (int j = 0; j < s.y; j++)
		for (int i = 0; i < s.x; i++) {
			wxColour col;
			if (i == 0 || j == 0 || i == s.x-1 || j == s.y-1) {
				col = wxColour((unsigned long) 0);
			}
			else col = c;
			img.SetRGB(i, j, col.Red(), col.Green(), col.Blue());
		}
	wxBitmap bmp(img);
	//b->SetBitmap(bmp);
	wxStaticBitmap *b = new wxStaticBitmap(this, -1, bmp, p, s);
	b->Refresh();
}

void ResultBrowser::AddLastResult(void)
{
	cfg.refresh();
	const char *fn = cfg["last_resultfile"];
	if (!fn) return;
	ResultNode rn;
	rn.sys_desc = "My PC";
	wxString bla = read_trim_file("comment.txt");
	if (!bla.empty()) {
		if (bla.length() > 30) { 
			bla = bla.substr(0, 27);
			bla += "...";
		}
		rn.sys_desc = rn.sys_desc + " (" + bla + ")";
	}
	rn.res_file = fn;
	rn.fps = get_float(cfg["last_fps"]);
	rn.cpu_mhz = get_int(cfg["last_mhz"]);
	rn.mem_mhz = 0;
	rn.status = STATUS_HOT;
	if (!my) my = new ResultXml("my.xml");
	my->add_entry(rn);
	UpdateGrid();
}

void ResultBrowser::OnBtnSendResult(wxCommandEvent &)
{
	wxArrayInt r = m_grid->GetSelectedRows();
	if (r.GetCount() == 0) {
		wxMessageBox("Select a result before trying to submit it", "Error", wxICON_ERROR);
		return ;
	}
	if (r.GetCount() > 1) {
		wxMessageBox("Please, select a single result", "Error", wxICON_ERROR);
		return ;
	}
	
	unsigned idx = r[0];
	if (idx >= m_last_displayed_results.size()) return;
	ResultNode rn = m_last_displayed_results[idx];
	
	if (!wxFileExists(rn.res_file)) {
		wxMessageBox("You can only submit your results", "Error", wxICON_ERROR);
		return ;
	}

	wxString server = cfg["server"];
	int server_port = get_int(cfg["server_port"]);
	SendDialog *sdlg = new SendDialog(this, server, server_port, rn.res_file);
	sdlg->ShowModal();
	sdlg->Destroy();
}

void ResultBrowser::OnBtnCompare(wxCommandEvent &)
{
	vector<CompareInfo> ci;
	for (int i = 0; i < m_grid->GetNumberRows(); i++) {
		if (m_grid->GetCellValue(i, 4) == "1") {
			CompareInfo info;
			info.name = m_grid->GetCellValue(i, 0);
			wxString fpss, mhzs;
			fpss = m_grid->GetCellValue(i, 1);
			mhzs = m_grid->GetCellValue(i, 2);
			sscanf(fpss.c_str(), "%f", &info.fps);
			sscanf(mhzs.c_str(), "%d", &info.mhz);
			ci.push_back(info);
		}
	}
	CompareDialog *dialog = new CompareDialog(this, &ci[0], ci.size());
	dialog->ShowModal();
	dialog->Destroy();
}

void ResultBrowser::OnGridClick(wxGridEvent &ev)
{
	if (ev.GetCol() == 4) {
		int r = ev.GetRow();
		wxString val = m_grid->GetCellValue(r, 4);
		if (val == "1") val = "";
			else val = "1";
		m_grid->SetCellValue(r, 4, val);
	}
	int selected = 0;
	for (int i = 0; i < m_grid->GetNumberRows(); i++) {
		if (m_grid->GetCellValue(i, 4) == "1")
			selected ++;
	}
	if (selected >= 2)
		m_compare->Enable();
	else
		m_compare->Disable();
	ev.Skip();
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

ResultBrowser::~ResultBrowser()
{
	if (builtin) {
		delete builtin; 
		builtin = NULL;
	}
	if (my) {
		my->save(); 
		delete my; 
		my = NULL;
	}
}

