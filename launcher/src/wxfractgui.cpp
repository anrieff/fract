/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <wx/wx.h>
#ifndef _WIN32
#include <stdlib.h>
#include <unistd.h>
#endif
#include "mainframe.h"

class MyApp : public wxApp
{
	public:
		virtual bool OnInit();
};

IMPLEMENT_APP(MyApp);
bool MyApp::OnInit()
{
#ifndef _WIN32
	// change dir to the process's dir...
	if (getenv("_")) {
		char *dir = strdup(getenv("_"));
		int i = strlen(dir) - 1;
		while (i && dir[i] != '/') i--;
		dir[++i] = 0;
		if (strcmp(dir, "./")) chdir(dir);
		free(dir);
	}
#endif
	MainFrame *main_frame = new MainFrame(
			"Fract Launcher for Fract 1.07",
			wxPoint(50, 50),
			wxSize(640, 640));
	main_frame->Show(TRUE);
	return TRUE;
}
