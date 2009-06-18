##
## cxxptl (C++ Portable Thread Library) Make script
## 
## Written from scratch by Veselin Georgiev
##
## Date: 2006-06-15
##

OSNAME := $(shell uname -s)

ifeq ($(OS), Linux)
  CXX = g++
  OBJEXT = o
  OUTOPT = -o 
  LIBEXT = a
  ISSUE_LINK = ar cru libcxxptl.$(LIBEXT)
  OPTFLAGS = -Wall -O2 -g
  BINARY = ptltest
  LINKFLAGS =
  COMPILEONLY = -c
else
  CXX = cl
  OBJEXT = obj
  OUTOPT = /Fe
  LIBEXT = lib
  ISSUE_LINK = lib /nologo /out:libcxxptl.$(LIBEXT)
  OPTFLAGS = /nologo /O2 /MT
  BINARY = ptltest.exe
  LINKFLAGS =
  COMPILEONLY = /c
endif


LIBRARY = libcxxptl.$(LIBEXT)
lib_OBJECTS = \
	cxxptl.$(OBJEXT) \
	sysspec_linux.$(OBJEXT) \
	sysspec_win32.$(OBJEXT) \
	sysspec_apple.$(OBJEXT) \
	sysspec_posix.$(OBJEXT)

SOURCES = cxxptl.cpp sysspec_linux.cpp sysspec_win32.cpp sysspec_apple.cpp\
	sysspec_posix.cpp tester.cpp
HEADERS = cxxptl.h


## Phony targets

all: $(BINARY)

clean:
	@echo CLEAN
	-@rm $(BINARY) *.$(OBJEXT) $(LIBRARY)

## Non-phony targets
%.$(OBJEXT): %.cpp $(HEADERS)
	@echo C++     $<
	@$(CXX) $(OPTFLAGS) $(COMPILEONLY) $<

$(LIBRARY): $(lib_OBJECTS)
	@echo LINK    $(LIBRARY)
	@$(ISSUE_LINK) $(lib_OBJECTS)
ifeq ($(OS), Linux)
	@echo RANLIB  $(LIBRARY)
	@$(RANLIB) $(LIBRARY)
endif

$(BINARY): $(LIBRARY) tester.cpp cxxptl.h
	@echo C++    $(BINARY)
	@$(CXX) $(OUTOPT)$(BINARY) $(OPTFLAGS) tester.cpp $(LIBRARY) $(LINKFLAGS)
	

.PHONY: all clean
