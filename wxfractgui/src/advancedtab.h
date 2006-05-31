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

#ifndef __ADV_TAB_H__
#define __ADV_TAB_H__

#include "generictab.h"

class AdvancedTab: public GenericTab
{
	wxStaticBox *m_bRenderSettings;
	wxStaticBox *m_bEffects;
	wxStaticBox *m_bSceneSettings;
	wxRadioBox *m_bLooping;
	wxCheckBox *m_cbWindowed;
	wxStaticText *m_useless1;
	wxComboBox *m_cmbResolution;
	wxStaticText *m_useless2;
	wxComboBox *m_cmbAntialiasing;
	wxStaticText *m_useless3;
	wxComboBox *m_cmbDontUse;
	wxStaticText *m_useless4;
	wxComboBox *m_cmbNumCPU;
	wxCheckBox *m_cbShadows;
	wxStaticText *m_useless5;
	wxComboBox *m_cmbShaders;
	wxCheckBox *m_cbFreeLook;
	wxCheckBox *m_cbCustomScene;
	wxTextCtrl *m_tbCustomScene;
	wxButton *m_btnCustomScene;
	//*************************************************
	void OnbLooping(wxCommandEvent &);
	void OncbWindowed(wxCommandEvent &);
	void OncmbResolution(wxCommandEvent &);
	void OncmbAntialiasing(wxCommandEvent &);
	void OncmbDontUse(wxCommandEvent &);
	void OncmbNumCPU(wxCommandEvent &);
	void OncbShadows(wxCommandEvent &);
	void OncmbShaders(wxCommandEvent &);
	void OncbFreeLook(wxCommandEvent &);
	void OncbCustomScene(wxCommandEvent &);
	void OntbCustomScene(wxCommandEvent &);
	void OnbtnCustomScene(wxCommandEvent &);
		
	void Rebuild(void);

	bool RebuildOK;
public:
	AdvancedTab(wxWindow *parent, wxTextCtrl *cmdline);
	void ParseGUI(wxWindow *parent, FILE *f, int cw, int ch);
	void WriteGUI(wxWindow *parent, FILE *f, int cw, int ch);
	void GenerateGUI(void);
	void RefreshCmdLine(void);
	bool RunPressed(void);
	DECLARE_EVENT_TABLE()
};

enum {
	bRenderSettings,
	bEffects,
	bSceneSettings,
	bLooping,
	cbWindowed,
	useless1,
	cmbResolution,
	useless2,
	cmbAntialiasing,
	useless3,
	cmbDontUse,
	useless4,
	cmbNumCPU,
	cbShadows,
	useless5,
	cmbShaders,
	cbFreeLook,
	cbCustomScene,
	tbCustomScene,
	btnCustomScene,
};

#endif // __ADV_TAB_H__
