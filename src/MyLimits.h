/***************************************************************************
 *   Copyright (C) 2004 by Veselin Georgiev                                *
 *   vesko@ViruS                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 * ----------------------------------------------------------------------- *
 *                                                                         *
 *  Defines some commonly used limits/constants                            *
 *                                                                         *
 ***************************************************************************/

#ifndef __MYLIMITS_H__
#define __MYLIMITS_H__
 
/* ====================== Const's / Defines ============================== */
#define PRIM_TYPE_BITS			1
#define PRIM_TYPE_MASK			0x8000
#define PRIM_TYPE_SPHERE		0x0000
#define PRIM_TYPE_TRIANGLE		0x8000

#define TRI_ID_BITS			14
#define TRI_ID_MASK			((1<<TRI_ID_BITS)-1)
#define OBJ_ID_BITS			(16-PRIM_TYPE_BITS-TRI_ID_BITS)
#define OBJ_ID_MASK			(((1<<OBJ_ID_BITS)-1)<<TRI_ID_BITS)

#define MAX_MESHES			2
#define MAX_TRIANGLES_PER_OBJECT	(1 << TRI_ID_BITS)
#define MAX_TRIANGLES			(MAX_MESHES*MAX_TRIANGLES_PER_OBJECT)

#define MAX_SPHERES 10000
#define MAX_SPHERE_SIDES 128

#endif
