# these are the headers for your project
noinst_HEADERS = wxfractgui.h mainframe.h fract_specific.h basictab.h \
							advancedtab.h generictab.h countries_list.h cpu_list.h global.h resbrowser.h \
					comparedlg.h fonts.h senddialog.h cpuid.h cpuid_database.h cpulistdlg.h

#########################################################################
# APPLICATION SECTION
#########################################################################
# this is the program that gets installed.  it's name is used for all
# of the other Makefile.am variables
bin_PROGRAMS = wxfractgui

# the application source, library search path, and link libraries
wxfractgui_SOURCES = wxfractgui.cpp mainframe.cpp fract_specific.cpp \
							basictab.cpp advancedtab.cpp resbrowser.cpp comparedlg.cpp fonts.cpp luxi_font.cpp \
			senddialog.cpp cpuid.cpp cpulistdlg.cpp
wxfractgui_LDFLAGS = $(WX_LIBS) 


