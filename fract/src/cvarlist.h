/***************************************************************************
 *   Copyright (C) 2006 by Veselin Georgiev                                *
 *   anrieff@mailg.com (convert to gmail)                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#undef DECLARE_VARIABLE
#ifdef IMPLEMENTATION
#	define DECLARE_VARIABLE(type, var, value, description) \
		allcvars.add(CVar(#var, &CVars::var, description))
#else
#	ifdef STORAGE
#		define DECLARE_VARIABLE(type, var, value, description) \
							type var = value
#	else
#		define DECLARE_VARIABLE(type, var, value, description) \
							extern type var
#	endif // STORAGE
#endif // IMPLEMENTATION


DECLARE_VARIABLE(double, alpha       , 0.0  , "camera rotation around Y");
DECLARE_VARIABLE(double, beta        , 0.0  , "camera rotation around XZ");
DECLARE_VARIABLE(bool,   bilinear    , false, "bilinear filtering toggle");
DECLARE_VARIABLE(bool,   anisotrophic, true , "anisotrophic-like filtering toggle");
