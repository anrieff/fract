##
## GNU Make Makefile for the omega project
## Written from scratch on 2006-12-18 by Veselin Georgiev
##
## File version: 0.1.0
## Version date: 2006-11-18
##
## Version History:
##	0.1.0: Initial script
##


## Setup the compilers, etc...
CXX = g++
CXXLINK = $(CXX)
OBJEXT = o

## Optimization flags
OPTFLAGS = -O0 -g

## Source files:
OBJECTS = \
	main.$(OBJEXT) \
	pluginmanager.$(OBJEXT)

##determine our host OS (if not defined check with uname)
ifndef OSTYPE
	OSTYPE := $(shell uname -s)
	ifeq "$(OSTYPE)" "Linux"
		OSTYPE := linux-gnu
	endif
endif

## Binary name:
ifeq ($(OSTYPE), linux-gnu)
BINARY = omega
else
BINARY = omega.exe
endif

## Include dirs:
INCLUDES = -I../cxxptl

## Lib search paths:
LIBSEARCH = -L../cxxptl

## Link with those libraries
LINK_WITH = -lSDL -lcxxptl -lpthread -ldl -lm


## Don't output those needless info
.SILENT:

## Phony targets:

all: $(BINARY) plugins

clean: plugins-clean
	echo "CLEAN"
	-rm *.o $(BINARY)

plugins:
	$(MAKE) -C plugins

plugins-clean:
	$(MAKE) -C plugins clean

.PHONY: all clean plugins plugins-clean

## Non-phony targets:

$(BINARY): $(OBJECTS)
	echo "C++[lnk] $(BINARY)"
	$(CXXLINK) $(OPTFLAGS) $(OBJECTS) $(LIBSEARCH) $(LINK_WITH) -o $(BINARY)

.cpp.$(OBJEXT):
	echo "C++      $<"
	$(CXX) $(OPTFLAGS) $(INCLUDES) -c $<

## export everything defined here to the subdirs
.EXPORT_ALL_VARIABLES:
