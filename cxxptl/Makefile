##
## cxxptl (C++ Portable Thread Library) Make script
## 
## Written from scratch by Veselin Georgiev
##
## Date: 2006-06-15
##

CXX = g++
CXXLD = $(CXX)
OPTFLAGS = -O2 -g
LINKFLAGS = -lpthread -lm
BINARY = cxxptl
OBJECTS = cxxptl.o sysspec_linux.o sysspec_win32.o sysspec_apple.o\
	sysspec_posix.o tester.o
HEADERS = cxxptl.h

## Phony targets

all: $(BINARY)

clean:
	@echo CLEAN
	-@rm $(BINARY)
	-@rm *.o

## Non-phony targets
$(OBJECTS): %.o: %.cpp
	@echo "C++   $<"
	@$(CXX) -c $< $(OPTFLAGS)

$(BINARY): $(OBJECTS)
	@echo "LINK  $(BINARY)"
	@$(CXXLD) $(OPTFLAGS) -o $(BINARY) $(OBJECTS) $(LINKFLAGS)


.PHONY: all clean
