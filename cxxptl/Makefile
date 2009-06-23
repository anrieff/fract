##
## cxxptl (C++ Portable Thread Library) Make script
## 
## Written from scratch by Veselin Georgiev
##
## Date: 2006-06-15
##

OS_UNAME := $(shell uname -s)

ifeq ($(OS_UNAME), Linux)
  OS := UNIX
endif
ifeq ($(OS_UNAME), Darwin)
  OS := UNIX
endif
ifeq ($(OS_UNAME), Win32)
  OS := Win32
endif

ifeq ($(OS), UNIX)
  CXX = g++
  OBJEXT = o
  OUTOPT = -o 
  LIBEXT = a
  ISSUE_LINK = ar cru libcxxptl.$(LIBEXT)
  OPTFLAGS = -Wall -O2 -g
  BINARY = ptltest
  LINKFLAGS = -lpthread
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
	-rm $(BINARY) *.$(OBJEXT) $(LIBRARY)

## Non-phony targets
%.$(OBJEXT): %.cpp $(HEADERS)
	$(CXX) $(OPTFLAGS) $(COMPILEONLY) $<

$(LIBRARY): $(lib_OBJECTS)
	$(ISSUE_LINK) $(lib_OBJECTS)
ifeq ($(OS), UNIX)
	ranlib $(LIBRARY)
endif

$(BINARY): $(LIBRARY) tester.cpp cxxptl.h
	$(CXX) $(OUTOPT)$(BINARY) $(OPTFLAGS) tester.cpp $(LIBRARY) $(LINKFLAGS)
	

.PHONY: all clean
