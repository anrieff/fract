/***************************************************************************
 *   Copyright (C) 2004 by Veselin Georgiev                                *
 *   vesko@ViruS                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef __PROGRESS_H__
#define __PROGRESS_H__

#define prog_dist_precalc_base		0.0
#define prog_dist_precalc		0.5
#define prog_text_precalc_base		prog_dist_precalc
#define prog_text_precalc		0.1
#define prog_vox_init_base		(prog_text_precalc_base+prog_text_precalc)
#define prog_vox_init			0.15
#define prog_normal_precalc_base	(prog_vox_init_base+prog_vox_init)
#define prog_normal_precalc		0.25

#endif //__PROGRESS_H__
