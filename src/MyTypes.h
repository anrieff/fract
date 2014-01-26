/***************************************************************************
 *   Copyright (C) 2004 by Veselin Georgiev                                *
 *   vesko@ViruS                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifdef ACTUALLYDISPLAY
#include <SDL/SDL.h>
#ifndef DEBUG
#define SDLFLAGS SDL_HWSURFACE|SDL_DOUBLEBUF|SDL_FULLSCREEN
#else
#define SDLFLAGS SDL_HWSURFACE|SDL_DOUBLEBUF
#endif

#else // define basic types

typedef unsigned int Uint32;
typedef int Sint32;
typedef unsigned short Uint16;
typedef short Sint16;
typedef unsigned char Uint8;
typedef signed char Sint8 ;

#endif
