##
## Makefile, written for the fract tools
## Written from scratch by Veselin Georgiev, 2006-11-27
## 
## Version 0.1.0
##
## Version History:
##	0.1.0 - 2006-11-28: Initial Makefile, that compiles everything
##
##


CC=gcc
CPP=g++
FPC=fpc
TOOLS_LDFLAGS=-lm


TOOLS=\
	bmp2fda\
	coords_to_106\
	find_circumscribed_sphere\
	fix1\
	gcc2msvc\
	hashgen\
	sample_normalizer\
	smooth\
	tesselate\
	time_rescale

all: $(TOOLS)

bmp2fda: bmp2fda.cpp

coords_to_106: coords_to_106.c

find_circumscribed_sphere: find_circumscribed_sphere.cpp

fix1: fix1.c

gcc2msvc: gcc2msvc.pas
	@echo "FPC   gcc2msvc.pas"
	@$(FPC) gcc2msvc.pas

hashgen: hashgen.c

sample_normalizer: sample_normalizer.cpp

smooth: smooth.c

tesselate: tesselate.cpp

time_rescale: time_rescale.c

.c:
	@echo "CC    $@.c"
	@$(CC) -O0 -g -o $@ $@.c $(TOOLS_LDFLAGS)

.cpp:
	@echo "CPP   $@.cpp"
	@$(CPP) -O0 -g -o $@ $@.cpp $(TOOLS_LDFLAGS)

clean:
	@echo "CLEAN"
	-@rm $(TOOLS) *.o 1> /dev/null 2> /dev/null


.PHONY: clean all
