
#ifndef _WXFRACTGUI_H_
#define _WXFRACTGUI_H_

/**
 * @short Application Main Window
 * @author Veselin Georgiev <vesko@ChaosGroup.com>
 * @version 0.1
 */

class 
wxfractguiapp : public wxApp
{
	public:
		virtual bool OnInit();
};

class 
wxfractguiFrame : public wxFrame
{
	public:
		wxfractguiFrame( const wxString& title, const wxPoint& pos, const wxSize& pos );
		void OnQuit( wxCommandEvent& event );
		void OnAbout( wxCommandEvent& event );

	private:
		DECLARE_EVENT_TABLE()
};

enum
{
	Menu_File_Quit = 100,
	Menu_File_About
};

#endif // _WXFRACTGUI_H_
