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

#include <string.h>
#include <ctype.h>
#include "advancedtab.h"

#include <vector>
#include <string>
using namespace std;

BEGIN_EVENT_TABLE(AdvancedTab, GenericTab)
	EVT_RADIOBOX(bLooping, AdvancedTab:: OnbLooping)
	EVT_CHECKBOX(cbWindowed, AdvancedTab:: OncbWindowed)
	EVT_COMBOBOX(cmbResolution, AdvancedTab:: OncmbResolution)
	EVT_COMBOBOX(cmbAntialiasing, AdvancedTab:: OncmbAntialiasing)
	EVT_COMBOBOX(cmbDontUse, AdvancedTab:: OncmbDontUse)
	EVT_COMBOBOX(cmbNumCPU, AdvancedTab:: OncmbNumCPU)
	EVT_CHECKBOX(cbShadows, AdvancedTab:: OncbShadows)
	EVT_COMBOBOX(cmbShaders, AdvancedTab:: OncmbShaders)
	EVT_CHECKBOX(cbFreeLook, AdvancedTab:: OncbFreeLook)
	EVT_CHECKBOX(cbCustomScene, AdvancedTab:: OncbCustomScene)
	EVT_TEXT(tbCustomScene, AdvancedTab:: OntbCustomScene)
	EVT_BUTTON(btnCustomScene, AdvancedTab:: OnbtnCustomScene)
END_EVENT_TABLE()


AdvancedTab::AdvancedTab(wxWindow *parent, wxTextCtrl *cmdline)
	:GenericTab(parent, cmdline)
{
	wxPanel *panel = new wxPanel(this);
// an ugly hack to use no panel even under windoze :)
	wxColour BGCol = panel->GetBackgroundColour();
	panel->Destroy();
	SetOwnBackgroundColour(BGCol);
	SetBackgroundColour(BGCol);

	RebuildOK = false;
	int cw, ch;
	parent->GetClientSize(&cw, &ch);
	/*FILE *f;
	f = fopen("gui.txt", "rt");
	if (!f) {
		wxMessageBox("Can't open file GUI.txt", "Error", wxICON_ERROR);
		return;
}*/
	//
	//ParseGUI(parent, f, cw, ch);
	//WriteGUI(parent, f, cw, ch);
	GenerateGUI();
	RebuildOK = true;
	//fclose(f);
}

void AdvancedTab::RefreshCmdLine(void)
{
	//cmd->SetValue("--developer");
	Rebuild();
}

bool AdvancedTab::RunPressed(void)
{
	return true;
}

bool AdvancedTab::CanRun(void)
{
	return true;
}

bool kmp(const char * s, const char *match) 
{
	return (0 == strncmp(s, match, strlen(match)));
}

class CallParser {
	vector<string> opts;
public:
	CallParser(const char *s)
	{
		opts.clear();
		int i = 0;
		while (s[i] != '(') i++;
		while (s[i] != ')') {
			int c = 0;
			string q = "";
			//while (s[i] != ')' && s[i] != ',') q += s[i++];
			while (i++,1) {
				if (s[i]=='[') { c++; continue;}
				if (s[i]==']') { c--; continue;}
				if (s[i]==',' && c == 0) break;
				if (s[i]==')') break;
				q += s[i];
			}

			opts.push_back(q);
		}
	}
	string operator [] (const int idx) const
	{
		if (idx >= opts.size() || idx < 0) {
			return "";
		}
		return opts[idx];
	}
};

int I(string s)
{
	int q;
	sscanf(s.c_str(), "%d", &q);
	return q;
}

wxString STRING(string s)
{
	if (s.find('"') == string::npos) return "";
	wxString q;
	int c = 0;
	for (int i = 0; i < s.length(); i++) {
		if (s[i] == '"') c = !c;
		if (s[i] != '"' && c) q += s[i];
	}
	return q;
}

wxString STR(string s)
{
	wxString q;
	for (int i = 0; i < s.length(); i++) {
		if (isalnum(s[i])) q += s[i];
	}
	return q;
}

wxArrayString ARRAY(string s)
{
	wxArrayString a;
	int c = 0;
	wxString q;
	for (int i = 0; i < s.length(); i++) {
		if (s[i] == '"') {
			if (c)
				a.Add(q);
			else
				q.clear();
			c = !c;
		} else {
			if (c) q += s[i];
		}
	}
	return a;
}

void AdvancedTab::ParseGUI(wxWindow *parent, FILE *f, int cw, int ch)
{
	char s[1000];
	while (fgets(s, 1000, f)) {
		int l = strlen(s);
		if (l < 5 || s[0] == '/')
			continue;
		int i = 0;
		while (i < l && s[i] != '(') i++;
		if (i >= l) continue;
		if (kmp(s, "BOX")) {
			CallParser A(s);
			wxStaticBox *sb = new wxStaticBox(this, -1, STRING(A[1]), 
				wxPoint(I(A[2]), I(A[3])),
				wxSize(I(A[4]), I(A[5])));
		}
		if (kmp(s, "TEXT")) {
			CallParser A(s);
			wxStaticText *sb = new wxStaticText(this, -1, STRING(A[1]), 
				wxPoint(I(A[2]), I(A[3])),
				wxSize(I(A[4]), I(A[5])));
		}
		if (kmp(s, "BUTTON")) {
			CallParser A(s);
			wxButton *bb = new wxButton(this, -1, STRING(A[1]), 
				wxPoint(I(A[2]), I(A[3])),
				wxSize(I(A[4]), I(A[5])));
		}
		if (kmp(s, "TEXTBOX")) {
			CallParser A(s);
			wxTextCtrl *txc = new wxTextCtrl(this, -1, STRING(A[1]),
					wxPoint(I(A[2]), I(A[3])),
					wxSize(I(A[4]), I(A[5])));
		}
		if (kmp(s, "CHECKBOX")) {
			CallParser A(s);
			wxCheckBox *bb = new wxCheckBox(this, -1, STRING(A[1]), 
				wxPoint(I(A[2]), I(A[3])),
				wxSize(I(A[4]), I(A[5])));
			if (I(A[6])) {
				bb->SetValue(true);
			} else {
				bb->SetValue(false);
			}
		}
		if (kmp(s, "COMBOBOX")) {
			CallParser A(s);
			wxArrayString ass = ARRAY(A[1]);
			wxComboBox *cb = new wxComboBox(this, -1, ass[0], 
				wxPoint(I(A[2]), I(A[3])),
				wxSize(I(A[4]), I(A[5])),
				ass, wxCB_READONLY | wxCB_DROPDOWN);

		}
		if (kmp(s, "RADIOBOX")) {
			CallParser A(s);
			wxArrayString ass = ARRAY(A[2]);
			wxRadioBox *rb = new wxRadioBox(this, -1, STRING(A[1]), 
				wxPoint(I(A[3]), I(A[4])),
				wxSize(I(A[5]), I(A[6])),
				ass, ass.size(), wxRA_SPECIFY_ROWS);
		}
	}
}

void vadd(vector<pair<string, string> > & vs, const char *var, const char *tp)
{
	string v(var), t(tp);
	vs.push_back(make_pair(v, t));
}

string p2var(pair<string, string> a)
{
	string q;
	q = a.second + " *m_" + a.first + ";";
	return q;
}

string p2enum(pair<string, string> a)
{
	return a.first+",";
}

string p2fundecl(pair<string, string> a)
{
	return "void On"+a.first+"(wxCommandEvent &);";
}

string p2funimpl(pair<string, string> a)
{
	return "void AdvancedTab::On" + a.first + "(wxCommandEvent &)\n{\n}\n";
}

string p2event(pair<string, string> a)
{
	string fp = "EVT_";
	for (int i = 2; i < a.second.length(); i++) {
		fp += toupper(a.second[i]);
	}
	fp += "(" + a.first + ", AdvancedTab:: On"+a.first + ");";
	return fp;
}

void AdvancedTab::WriteGUI(wxWindow *parent, FILE *f, int cw, int ch)
{
	FILE * out = fopen("adv.cpp", "wt");
	vector<pair<string, string> > vars;
	char s[1000];
	while (fgets(s, 1000, f)) {
		int l = strlen(s);
		if (l < 5 || s[0] == '/')
			continue;
		int i = 0;
		while (i < l && s[i] != '(') i++;
		if (i >= l) continue;
		if (kmp(s, "BOX")) {
			CallParser A(s);
			char p1[100];
			strcpy(p1, STR(A[0]).c_str());
			fprintf(out, "m_%s = new wxStaticBox(this, %s, \"%s\",\n\t\twxPoint(%d, %d),\n\t\twxSize(%d, %d));\n", p1, p1, STRING(A[1]).c_str(), I(A[2]), I(A[3]), I(A[4]), I(A[5]));
			vadd(vars, p1, "wxStaticBox");
		}
		if (kmp(s, "TEXT")) {
			CallParser A(s);
			/*wxStaticText *sb = new wxStaticText(this, -1, STRING(A[1]), 
					wxPoint(I(A[2]), I(A[3])),
			wxSize(I(A[4]), I(A[5])));*/
			char p1[100];
			strcpy(p1, STR(A[0]).c_str());
			fprintf(out, "m_%s = new wxStaticText(this, %s, \"%s\",\n\t\twxPoint(%d, %d),\n\t\twxSize(%d, %d));\n", p1, p1, STRING(A[1]).c_str(), I(A[2]), I(A[3]), I(A[4]), I(A[5]));
			vadd(vars, p1, "wxStaticText");
			
		}
		if (kmp(s, "BUTTON")) {
			CallParser A(s);
			/*wxButton *bb = new wxButton(this, -1, STRING(A[1]), 
					wxPoint(I(A[2]), I(A[3])),
			wxSize(I(A[4]), I(A[5])));*/
			char p1[100];
			strcpy(p1, STR(A[0]).c_str());
			fprintf(out, "m_%s = new wxButton(this, %s, \"%s\",\n\t\twxPoint(%d, %d),\n\t\twxSize(%d, %d));\n", p1, p1, STRING(A[1]).c_str(), I(A[2]), I(A[3]), I(A[4]), I(A[5]));
			
			vadd(vars, p1, "wxButton");
		}
		if (kmp(s, "TEXTBOX")) {
			CallParser A(s);
			/*wxTextCtrl *txc = new wxTextCtrl(this, -1, STRING(A[1]),
					wxPoint(I(A[2]), I(A[3])),
					wxSize(I(A[4]), I(A[5])));
			*/
			char p1[100];
			strcpy(p1, STR(A[0]).c_str());
			fprintf(out, "m_%s = new wxTextCtrl(this, %s, \"%s\",\n\t\twxPoint(%d, %d),\n\t\twxSize(%d, %d));\n", p1, p1, STRING(A[1]).c_str(), I(A[2]), I(A[3]), I(A[4]), I(A[5]));
			
			vadd(vars, p1, "wxTextCtrl");
		}
		if (kmp(s, "CHECKBOX")) {
			CallParser A(s);
			/*wxCheckBox *bb = new wxCheckBox(this, -1, STRING(A[1]), 
					wxPoint(I(A[2]), I(A[3])),
					wxSize(I(A[4]), I(A[5])));
			*/
			char p1[100];
			strcpy(p1, STR(A[0]).c_str());
			fprintf(out, "m_%s = new wxCheckBox(this, %s, \"%s\",\n\t\twxPoint(%d, %d),\n\t\twxSize(%d, %d));\n", p1, p1, STRING(A[1]).c_str(), I(A[2]), I(A[3]), I(A[4]), I(A[5]));
			
			if (I(A[6])) {
				//bb->SetValue(true);
				fprintf(out, "m_%s->SetValue(true);\n", p1);
			} else {
				//bb->SetValue(false);
				fprintf(out, "m_%s->SetValue(false);\n", p1);
			}
			vadd(vars, p1, "wxCheckBox");
		}
		if (kmp(s, "COMBOBOX")) {
			CallParser A(s);
			wxArrayString ass = ARRAY(A[1]);
			char p1[100];
			strcpy(p1, STR(A[0]).c_str());
			/*wxComboBox *cb = new wxComboBox(this, -1, ass[0], 
					wxPoint(I(A[2]), I(A[3])),
					wxSize(I(A[4]), I(A[5])),
			ass, wxCB_READONLY | wxCB_DROPDOWN);*/
			fprintf(out, "wxArrayString a_%s;\n", p1);
			for (int i =0 ; i < ass.size(); i++) {
				fprintf(out, "a_%s.Add(\"%s\");\n", p1, ass[i].c_str());
			}
			fprintf(out, "m_%s = new wxComboBox(this, %s, \"%s\",\n\t\twxPoint(%d, %d),\n\t\twxSize(%d, %d),\n\t\ta_%s, wxCB_READONLY | wxCB_DROPDOWN);\n", p1, p1, ass[0].c_str(), I(A[2]), I(A[3]), I(A[4]), I(A[5]), p1);
			vadd(vars, p1, "wxComboBox");
		}
		if (kmp(s, "RADIOBOX")) {
			CallParser A(s);
			wxArrayString ass = ARRAY(A[2]);
			/*
			wxRadioBox *rb = new wxRadioBox(this, -1, STRING(A[1]), 
					wxPoint(I(A[3]), I(A[4])),
					wxSize(I(A[5]), I(A[6])),
					ass, ass.size(), wxRA_SPECIFY_ROWS);
			*/
			char p1[100];
			strcpy(p1, STR(A[0]).c_str());
			fprintf(out, "wxArrayString a_%s;\n", p1);
			for (int i =0 ; i < ass.size(); i++) {
				fprintf(out, "a_%s.Add(\"%s\");\n", p1, ass[i].c_str());
			}
			fprintf(out, "m_%s = new wxRadioBox(this, %s, \"%s\",\n\t\twxPoint(%d, %d),\n\t\twxSize(%d, %d),\n\t\ta_%s, a_%s.size(), wxRA_SPECIFY_ROWS);\n", p1, p1, STR(A[1]).c_str(), I(A[3]), I(A[4]), I(A[5]), I(A[6]), p1, p1);
			vadd(vars, p1, "wxRadioBox");

		}
	}
	fprintf(out, "//variables:\n");
	for (int i = 0; i < vars.size(); i++) {
		fprintf(out, "%s\n", p2var(vars[i]).c_str());
	}
	fprintf(out, "//enum:\nenum {\n");
	for (int i = 0; i < vars.size(); i++) {
		fprintf(out, "%s\n", p2enum(vars[i]).c_str());
	}
	fprintf(out, "};\n");
	fprintf(out, "//Function declarations:\n");
	for (int i = 0; i < vars.size(); i++) {
		fprintf(out, "%s\n", p2fundecl(vars[i]).c_str());
	}
	fprintf(out, "//Event specs:\n");
	for (int i = 0; i < vars.size(); i++) {
		fprintf(out, "%s\n", p2event(vars[i]).c_str());
	}
	fprintf(out, "//Function implementations:\n");
	for (int i = 0; i < vars.size(); i++) {
		fprintf(out, "%s\n", p2funimpl(vars[i]).c_str());
	}
	fclose(out);
}

void AdvancedTab::GenerateGUI(void)
{
	m_bRenderSettings = new wxStaticBox(this, bRenderSettings, "Render Settings",
					    wxPoint(16, 16),
					    wxSize(247, 210));
	m_bEffects = new wxStaticBox(this, bEffects, "Fullscreen Effects",
				     wxPoint(16, 236),
				     wxSize(247, 80));
	m_bSceneSettings = new wxStaticBox(this, bSceneSettings, "Scene Settings",
					   wxPoint(273, 16),
					   wxSize(319, 140));
	wxArrayString a_bLooping;
	a_bLooping.Add("Single Run");
	a_bLooping.Add("Loop 5 times");
	a_bLooping.Add("Loop forever");
	m_bLooping = new wxRadioBox(this, bLooping, "Looping",
				wxPoint(273, 166),
				wxSize(319, 100),
				a_bLooping, a_bLooping.size(), wxRA_SPECIFY_ROWS);
	m_cbWindowed = new wxCheckBox(this, cbWindowed, "Run in a Window",
				      wxPoint(32, 38),
				      wxSize(-1, -1));
	m_cbWindowed->SetValue(false);
	m_useless1 = new wxStaticText(this, useless1, "Resolution:",
				      wxPoint(32, 68),
				      wxSize(84, -1));
	wxArrayString a_cmbResolution;
	a_cmbResolution.Add("320x240");
	a_cmbResolution.Add("400x300");
	a_cmbResolution.Add("512x384");
	a_cmbResolution.Add("640x480");
	a_cmbResolution.Add("800x600");
	a_cmbResolution.Add("1024x768");
	m_cmbResolution = new wxComboBox(this, cmbResolution, "640x480",
					 wxPoint(120, 64),
					 wxSize(130, -1),
					a_cmbResolution, wxCB_READONLY | wxCB_DROPDOWN);
	m_useless2 = new wxStaticText(this, useless2, "Antialiasing:",
				      wxPoint(32, 98),
				      wxSize(84, -1));
	wxArrayString a_cmbAntialiasing;
	a_cmbAntialiasing.Add("none");
	a_cmbAntialiasing.Add("4xlo-fi");
	a_cmbAntialiasing.Add("4xhi-fi");
	a_cmbAntialiasing.Add("4xAAA");
	a_cmbAntialiasing.Add("5xAAA");
	a_cmbAntialiasing.Add("10xAAA");
	a_cmbAntialiasing.Add("16xAAA");
	m_cmbAntialiasing = new wxComboBox(this, cmbAntialiasing, "none",
					   wxPoint(120, 94),
					   wxSize(130, -1),
					a_cmbAntialiasing, wxCB_READONLY | wxCB_DROPDOWN);
	m_useless3 = new wxStaticText(this, useless3, "Don't Use:",
				      wxPoint(32, 128),
				      wxSize(84, -1));
	wxArrayString a_cmbDontUse;
	a_cmbDontUse.Add("");
	a_cmbDontUse.Add("SSE");
	a_cmbDontUse.Add("SSE & MMX2");
	a_cmbDontUse.Add("SSE,MMX2,MMX");
	m_cmbDontUse = new wxComboBox(this, cmbDontUse, "",
				      wxPoint(120, 124),
				      wxSize(130, -1),
				a_cmbDontUse, wxCB_READONLY | wxCB_DROPDOWN);
	m_useless4 = new wxStaticText(this, useless4, "# of Threads:",
				      wxPoint(32, 158),
				      wxSize(104, -1));
	wxArrayString a_cmbNumCPU;
	a_cmbNumCPU.Add("auto");
	a_cmbNumCPU.Add("1");
	a_cmbNumCPU.Add("2");
	a_cmbNumCPU.Add("3");
	a_cmbNumCPU.Add("4");
	a_cmbNumCPU.Add("5");
	a_cmbNumCPU.Add("6");
	a_cmbNumCPU.Add("7");
	a_cmbNumCPU.Add("8");
	a_cmbNumCPU.Add("16");
	a_cmbNumCPU.Add("32");
	a_cmbNumCPU.Add("64");
	m_cmbNumCPU = new wxComboBox(this, cmbNumCPU, "auto",
				     wxPoint(140, 154),
				     wxSize(110, -1),
					a_cmbNumCPU, wxCB_READONLY | wxCB_DROPDOWN);
	m_cbShadows = new wxCheckBox(this, cbShadows, "Show Shadows",
				     wxPoint(32, 188),
				     wxSize(-1, -1));
	m_cbShadows->SetValue(true);
	m_useless5 = new wxStaticText(this, useless5, "Effect:",
				      wxPoint(32, 274),
				      wxSize(60, -1));
	wxArrayString a_cmbShaders;
	a_cmbShaders.Add("None");
	a_cmbShaders.Add("EdgeGlow");
	a_cmbShaders.Add("Sobel");
	a_cmbShaders.Add("FFTFilter");
	a_cmbShaders.Add("Blur");
	a_cmbShaders.Add("Inversion");
	a_cmbShaders.Add("ObjectGlow");
	m_cmbShaders = new wxComboBox(this, cmbShaders, "None",
				      wxPoint(100, 270),
				      wxSize(150, -1),
					a_cmbShaders, wxCB_READONLY | wxCB_DROPDOWN);
	m_cbFreeLook = new wxCheckBox(this, cbFreeLook, "Free Look",
				      wxPoint(290, 38),
				      wxSize(-1, -1));
	m_cbFreeLook->SetValue(false);
	m_cbCustomScene = new wxCheckBox(this, cbCustomScene, "Custom Scene:",
					 wxPoint(290, 68),
					 wxSize(-1, -1));
	m_cbCustomScene->SetValue(false);
	m_tbCustomScene = new wxTextCtrl(this, tbCustomScene, "data/heart.fsv",
					 wxPoint(290, 98),
					 wxSize(260, -1));
	m_btnCustomScene = new wxButton(this, btnCustomScene, "...",
					wxPoint(555, 98),
					wxSize(24, 24));
	m_tbCustomScene->Disable();
	m_btnCustomScene->Disable();
}

void AdvancedTab::Rebuild(void)
{
	if (!RebuildOK)
		return;
	wxString a;
	if (m_bLooping->GetSelection() == 1) {
		a += "--loops=5 ";
	}
	if (m_bLooping->GetSelection() == 2) {
		a += "--loop ";
	}
	if (m_cbWindowed->GetValue()) 
		a += "-w ";
	if (m_cmbResolution->GetValue() != "640x480") {
		wxString q = m_cmbResolution->GetValue();
		a += "--xres=" + q.BeforeFirst('x') + " ";
	}
	if (m_cmbAntialiasing->GetValue() != "none") {
		a += "--fsaa=" + m_cmbAntialiasing->GetValue()  + " ";
	}
	if (m_cmbDontUse->GetValue() != "") {
		if (m_cmbDontUse->GetValue() == "SSE,MMX2,MMX") {
			a += "--no-mmx --no-mmx2 --no-sse ";
		}
		if (m_cmbDontUse->GetValue() == "SSE & MMX2") {
			a += "--no-mmx2 --no-sse ";
		}
		if (m_cmbDontUse->GetValue() == "SSE") {
			a += "--no-sse ";
		}
	}
	if (m_cmbNumCPU->GetValue() != "auto") {
		a += "--cpus=" + m_cmbNumCPU->GetValue() + " ";
	}
	if (!m_cbShadows->GetValue() ) {
		a += "--no-shadows ";
	}
	if (m_cmbShaders->GetValue() != "None") {
		a += "--shader=" + m_cmbShaders->GetValue() + " ";
	}
	if (m_cbFreeLook->GetValue() ) {
		a += "--developer ";
	}
	if (m_tbCustomScene->IsEnabled() && m_tbCustomScene->GetValue() != "" ) {
		a += "--scene=" + m_tbCustomScene->GetValue() + " ";
	}
	cmd->SetValue(a);
}

void AdvancedTab::OnbLooping(wxCommandEvent &)
{
	Rebuild();
}

void AdvancedTab::OncbWindowed(wxCommandEvent &)
{
	Rebuild();
}

void AdvancedTab::OncmbResolution(wxCommandEvent &)
{
	Rebuild();
}

void AdvancedTab::OncmbAntialiasing(wxCommandEvent &)
{
	Rebuild();
}

void AdvancedTab::OncmbDontUse(wxCommandEvent &)
{
	Rebuild();
}

void AdvancedTab::OncmbNumCPU(wxCommandEvent &)
{
	Rebuild();
}

void AdvancedTab::OncbShadows(wxCommandEvent &)
{
	Rebuild();
}

void AdvancedTab::OncmbShaders(wxCommandEvent &)
{
	Rebuild();
}

void AdvancedTab::OncbFreeLook(wxCommandEvent &)
{
	Rebuild();
}

void AdvancedTab::OncbCustomScene(wxCommandEvent &)
{
	if (m_cbCustomScene->GetValue()) {
		m_tbCustomScene->Enable();
		m_btnCustomScene->Enable();
	} else {
		m_tbCustomScene->Disable();
		m_btnCustomScene->Disable();
	}
	Rebuild();
}

void AdvancedTab::OntbCustomScene(wxCommandEvent &)
{
	Rebuild();
}

void AdvancedTab::OnbtnCustomScene(wxCommandEvent &)
{
	wxString FN = wxFileSelector("Open a Scene", "", "", "*.fsv",
				     "Fract Scene Files (*.fsv)|*.fsv",
				     wxOPEN|wxFILE_MUST_EXIST);
	if (!FN.empty()) {
		for (int i = 0; i < FN.Length(); i++)
			if (FN[i] == '\\') FN[i] = '/';
		m_tbCustomScene->SetValue(FN);
	}
	Rebuild();
}
