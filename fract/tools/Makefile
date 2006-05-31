
CPP=g++
TOOLS_LDFLAGS=-lm

all: bmp2fda

bmp2fda: bmp2fda.cpp

.cpp:
	@echo "CPP   $@.cpp"
	@$(CPP) -O0 -g -o $@ $@.cpp $(TOOLS_LDFLAGS)

clean:
	@echo "CLEAN"
	-@rm bmp2fda > /dev/null 2>/dev/null


.PHONY: clean all
