##
## Makefile for Omega's plugins
##
## File version: 0.1.0
## Version date: 2006-11-18
##
## Version History:
##	0.1.0: Initial script
##

.PHONY: all clean

##determine our host OS (if not defined check with uname)
ifndef OSTYPE
	OSTYPE := $(shell uname -s)
	ifeq "$(OSTYPE)" "Linux"
		OSTYPE := linux-gnu
	endif
endif

ifeq ($(OSTYPE),linux-gnu)
DYNLIBEXT = so
else
DYNLIBEXT = dll
endif

.SILENT:

PLUGINS = \
	mandelbrot.$(DYNLIBEXT)

all: $(PLUGINS)

mandelbrot.$(DYNLIBEXT): mandelbrot.cpp
	echo "C++[lnk] $<"
	$(CXX) $(OPTFLAGS) -fPIC -shared -o $@ -Wl,-soname,$@ -I.. -lm $<

clean:
	echo "CLEAN plugins"
	-rm *.$(DYNLIBEXT)
