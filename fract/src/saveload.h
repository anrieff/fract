/***************************************************************************
 *   Copyright (C) 2004 by Veselin Georgiev                                *
 *   vesko@ViruS                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#define SAVEFILE_MAGIC  "#@# WSM ."
#define SAVEFILE_MAGICn "#@# WSM .\n"

#define SAVLOAD_ERROR		0
#define SAVLOAD_OK		1
#define SAVLOAD_COORDS		2
#define SAVLOAD_COORDS_106	3


#define MAXCD 5000

typedef struct {
	double time;
	double cam[3];
	double alpha, beta;
	// new in 1.06:
	bool is_106; // true if 1.06 .fsv file was read in the first place
	double light[3];
	int pp_state;
	float shader_param;
}frameinfo;

typedef struct {
	char name[16];
	unsigned flag;
}flag2strmapper;

const flag2strmapper flag2str[32] = {
	{ "raytraced",		RAYTRACED },
	{ "recursive",		RECURSIVE },
	{ "transparent",	SEETHROUGH },
	{ "recurse_self",	RECURSE_SELF},
	{ "static",		STATIC },
	{ "collideable", 	COLLIDEABLE },
	{ "animated",		ANIMATED },
	{ "nofloorbounce",	NOFLOORBOUNCE },
	{ "gravity",		GRAVITY },
	{ "air",		AIR },
	{ "mapped",		MAPPED },
	{ "stochastic",		STOCHASTIC},
	{ "normal_map",		NORMAL_MAP},
	{ "casts_shadow",	CASTS_SHADOW},
	{ "fresnel",		FRESNEL },
};

/* ----------------------------------- prototype seciton --------------------------------- */

int save_context(const char *fn);
void save_coords(const char *fn);
int load_context(const char *fn);
void generate_coords(void);
void record_do(double time);
bool load_frame(int frame_no, double time, int mySceneType, int loopmode, int& loops_remaining);
void saveload_close(void);



/*EOF*/
