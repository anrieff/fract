/***************************************************************************
 *   Copyright (C) 2004 by Veselin Georgiev                                *
 *   vesko@ViruS                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/
#ifndef __ASMCONFIG_H__
#define __ASMCONFIG_H__
// turning this option off will disable every single assembly source line.
// Useful for compilation on non-x86 machines
// Will shut itself off automagically on x86_64 and PowerPC

#if !defined __x86_64__ && !defined _ARCH_PPC
#define USE_ASSEMBLY
#endif

#endif // __ASMCONFIG_H__
