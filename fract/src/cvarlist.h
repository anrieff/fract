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
DECLARE_VARIABLE(double, fov         , 1.0  , "field-of-view");
DECLARE_VARIABLE(bool,   bilinear    , false, "bilinear filtering toggle");
DECLARE_VARIABLE(bool,   anisotrophic, true , "anisotrophic-like filtering toggle");
DECLARE_VARIABLE(double, area_const  , 0.0  , "mipmap selection area unit");
DECLARE_VARIABLE(double, brightness  , 1.0  , "final image brightness");
DECLARE_VARIABLE(int,    v_ores      , 16   , "voxel outer image sampling size");
DECLARE_VARIABLE(double, v_ires      , 0.5  , "voxel inner image sampling multiplier");
DECLARE_VARIABLE(bool,   v_showspeed , false, "show processing time for each sub-block");
DECLARE_VARIABLE(int,    v_maxdiff   , 10   , "threshold for corners' colors difference");
DECLARE_VARIABLE(int,    shadow_algo , 0    , "0 - raster shadow algo, 1 - raytracing");
DECLARE_VARIABLE(int,    shadowquality,  1  , "shadow quality selector");
DECLARE_VARIABLE(int,    lmsize,       0    , "light map size");
DECLARE_VARIABLE(int,    gloss_samples, 1   , "glossiness samples per raytrace");
DECLARE_VARIABLE(bool,   physics     , true , "toggles the physics engine");
DECLARE_VARIABLE(bool,   collisions  , true , "toggles the collision detection engine");
DECLARE_VARIABLE(bool,   animation   , true , "toggles overall object animation");
DECLARE_VARIABLE(double, sv_gravity  ,166.66, "gravity coefficient");
DECLARE_VARIABLE(double, sv_air      , 3.99 , "air drag coefficient");
DECLARE_VARIABLE(double, aspect_ratio, 4./3., "screen aspect ratio");
DECLARE_VARIABLE(bool,   crosshair   , true , "show crosshair toggle");
DECLARE_VARIABLE(bool,   photomode   , true , "photorealistic rendering toggle");
DECLARE_VARIABLE(double, aperture    , 11.0  , "DOF aperture");
DECLARE_VARIABLE(int,    dof_samples , 20   , "DOF samples per pixel");
DECLARE_VARIABLE(bool,   qmc         , true , "Quasi-monte carlo sampling");
DECLARE_VARIABLE(bool,   g_speedup   , true , "SDTree usage toggle");
DECLARE_VARIABLE(bool,   fisheye     , false, "Fisheye lens emulation in photomode");
DECLARE_VARIABLE(bool,   shut_down   , false, "Screenshot & exit afther next frame");
DECLARE_VARIABLE(int ,   bucket_size , 0    , "Setup static bucket size");
DECLARE_VARIABLE(int ,   dof_blades  , 0    , "# of diaphragm blades; 0 to disable");
