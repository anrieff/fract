##
## cxxptl (C++ Portable Thread Library) Make script
## 
## Written from scratch by Veselin Georgiev
##
## Date: 2006-06-15
##

CXX = g++
CXXLD = $(CXX)
RANLIB = ranlib
AR = ar
OPTFLAGS = -O2 -g
LINKFLAGS = -lpthread -lm
BINARY = cxxptl
LIBRARY = libcxxptl.a
lib_OBJECTS = cxxptl.o sysspec_linux.o sysspec_win32.o sysspec_apple.o\
	sysspec_posix.o
SOURCES = cxxptl.cpp sysspec_linux.cpp sysspec_win32.cpp sysspec_apple.cpp\
	sysspec_posix.cpp tester.cpp
HEADERS = cxxptl.h


## Phony targets

all: $(BINARY)

clean:
	@echo CLEAN
	-@rm $(BINARY) *.o $(LIBRARY)

## Non-phony targets
$(lib_OBJECTS): %.o: %.cpp $(HEADERS)
	@echo "C++     $<"
	@$(CXX) -c $< $(OPTFLAGS)

$(LIBRARY): $(lib_OBJECTS)
	@echo "AR      $(LIBRARY)"
	@$(AR) cru $(LIBRARY) $(lib_OBJECTS)
	@echo "RANLIB  $(LIBRARY)"
	@$(RANLIB) $(LIBRARY)

$(BINARY): $(LIBRARY) tester.cpp cxxptl.h
	@echo "LINK    $(BINARY)"
	@$(CXXLD) -o $(BINARY) $(OPTFLAGS) tester.cpp $(LIBRARY) $(LINKFLAGS)
	

.PHONY: all clean
