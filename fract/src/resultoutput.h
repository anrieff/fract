/***************************************************************************
 *   Copyright (C) 2004 by Veselin Georgiev                                *
 *   vesko@ViruS                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *-------------------------------------------------------------------------*
 *   Handles official .result file creation and security checking          *
 ***************************************************************************/

#ifndef __RESULT_OUTPUT_H__
#define __RESULT_OUTPUT_H__

#include "scene.h"


#define USER_INFO_FILE "userinfo.txt"
#define USER_COMMENT_FILE "comment.txt"
#include "resultfmt.h"

void generate_result_file(FPSWatch & );
int query_integrity(void);
#endif // __RESULT_OUTPUT_H__
