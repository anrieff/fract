bin_PROGRAMS = fract
fract_SOURCES = antialias.cpp bitmap.cpp blur.cpp cmdline.cpp common.cpp \
																	fract.cpp gfx.cpp infinite_plane.cpp profile.cpp render.cpp rgb2yuv.cpp \
																	sphere.cpp threads.cpp triangle.cpp saveload.cpp scene.cpp shaders.cpp \
																	vectormath.cpp voxel.cpp mesh.cpp aes.cpp md5.cpp resultoutput.cpp shadows.cpp \
																	random.cpp sdtree.cpp cpu.cpp thorus.cpp tracer.cpp credits.cpp memory.cpp \
												konsole.cpp konsole_commands.cpp cvar.cpp cvars.cpp string_array.cpp progress.cpp \
						light.cpp hierarchy.cpp radiosity.cpp physics.cpp matrix.cpp sor.cpp

# set the include path found by configure
AM_CPPFLAGS = $(all_includes) $(LIBSDL_CFLAGS)

# the library search path.
fract_LDFLAGS = $(all_libraries) 
noinst_HEADERS = antialias.h asmconfig.h bitmap.h blur.h cmdline.h common.h \
											cross_vars.h fract.h gfx.h hierarchy.h idnames.h infinite_plane.h mesh.h MyGlobal.h \
											MyLimits.h MyTypes.h object.h profile.h progress.h render.h rgb2yuv.h saveload.h \
											scene.h shaders.h sphere.h threads.h triangle.h vector3.h vectormath.h voxel.h \
											x86_asm.h shadows.h array.h vector2f.h sdtree.h bbox.h cpu.h thorus.h credits.h \
								konsole.h konsole_commands.h cvar.h cvars.h cvarlist.h string_array.h light.h \
			radiosity.h physics.h matrix.h
fract_LDADD = -lpthread -lSDL
