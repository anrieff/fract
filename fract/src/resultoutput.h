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
#define FRACT_CONFIG_FILE "fract.cfg"
#include "resultfmt.h"

void generate_result_file(FPSWatch & );
int query_integrity(void);

struct FractConfig {
	void init(void);
	void finish(void);
	void defaults(void);
	FractConfig();

	char credits_shown[16];
	int install_id;
	int last_mhz;
	float last_fps;
	char last_resultfile[256];
	char server[64];
	int server_port;
};

extern FractConfig config;

#endif // __RESULT_OUTPUT_H__
