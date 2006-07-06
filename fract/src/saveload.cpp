/***************************************************************************
 *   Copyright (C) 2004 by Veselin Georgiev                                *
 *   vesko@ViruS                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *-------------------------------------------------------------------------*
 *                                                                         *
 * Handles save / load procedures. Fract engine can load its state (camera *
 * position, spheres, animation, etc.) in text files (usually .fsv files)  *
 * and may also save its state as well.                                    *
 *                                                                         *
***************************************************************************/
#include <limits.h>
#include <string.h>
#include "MyGlobal.h"
#include "MyTypes.h"
#include "bitmap.h"
#include "blur.h"
#include "cmdline.h"
#include "fract.h"
#include "mesh.h"
#include "scene.h"
#include "sphere.h"
#include "saveload.h"
#include "shaders.h"
#include "shadows.h"
#include "triangle.h"
#include "thorus.h"
#include "tracer.h"

/* ------------------------------------ export section ------------------------------------ */
extern int spherecount;
extern Sphere sp[];
extern int lx;
extern int ly;
extern int lz;
extern Vector cur;
extern int Physics;
extern int CollDetect;
extern char floor_texture[];
extern char default_font[];
extern AniSphere AS[];
extern int anicnt;
extern double alpha, beta;
extern double sv_gravity, sv_air;
extern double fov;

/* ----------------------------------- variables ------------------------------------------ */

frameinfo cd[MAXCD];
int cd_frames;

double play_time;

#ifdef RECORD
int rec_initialized = 0, rec_init_try = 0;
FILE *rec_file;
#endif

const char * squalities[3] = { "low", "good", "best" };

void write_comments(FILE *f)
{
	fprintf(f, "#\n# This is a fract-generated save context file.\n# Every line starting with a pound sign is ignored; ");
	fprintf(f, "Don't remove the first line, it is important.\n#\n# All data can be edited.\n#\n");
}

void write_triplet(FILE *f, const char *prefix, double x, double y, double z)
{
	fprintf(f, "%s.x=%.5lf\n", prefix, x);
	fprintf(f, "%s.y=%.5lf\n", prefix, y);
	fprintf(f, "%s.z=%.5lf\n", prefix, z);
}

void write_triplet1(FILE *f, const char *prefix, Vector &a)
{
	write_triplet(f, prefix, a[0], a[1], a[2]);
}

void write_sphere(FILE *f, int num, Sphere a)
{int i;
	fprintf(f, "Sphere[%d].x=%.5lf\n", num, a.pos[0]);
	fprintf(f, "Sphere[%d].y=%.5lf\n", num, a.pos[1]);
	fprintf(f, "Sphere[%d].z=%.5lf\n", num, a.pos[2]);
	fprintf(f, "Sphere[%d].xi=%.5lf\n", num, a.mov[0]);
	fprintf(f, "Sphere[%d].yi=%.5lf\n", num, a.mov[1]);
	fprintf(f, "Sphere[%d].zi=%.5lf\n", num, a.mov[2]);
	fprintf(f, "Sphere[%d].d=%.5lf\n", num, a.d);
	fprintf(f, "Sphere[%d].mass=%.5lf\n", num, a.mass);
	fprintf(f, "Sphere[%d].refl=%.5lf\n", num, a.refl);
	fprintf(f, "Sphere[%d].opacity=%.5f\n", num, a.opacity);
	fprintf(f, "Sphere[%d].r=%d\n", num, a.r);
	fprintf(f, "Sphere[%d].g=%d\n", num, a.g);
	fprintf(f, "Sphere[%d].b=%d\n", num, a.b);
	fprintf(f, "Sphere[%d].AniIndex=%d\n", num, a.AniIndex);
	if (a.flags & MAPPED)
	fprintf(f, "Sphere[%d].texture=\"%s\"\n", num, a.tex->get_tex_fn());
	for (i=0;i<31;i++)
		if (flag2str[i].name[0])
			fprintf(f, "Sphere[%d].flags.%s=%d\n", num, flag2str[i].name, ((flag2str[i].flag & a.flags)!=0));
}

void write_ani(FILE *f, int num, AniSphere a)
{
	fprintf(f, "AS[%d].cx=%.5lf\n", num, a.cx);
	fprintf(f, "AS[%d].cy=%.5lf\n", num, a.cy);
	fprintf(f, "AS[%d].cz=%.5lf\n", num, a.cz);
	fprintf(f, "AS[%d].radius=%.5lf\n", num, a.radius);
	fprintf(f, "AS[%d].alpha=%.5lf\n", num, a.alpha);
	fprintf(f, "AS[%d].beta=%.5lf\n", num, a.beta);
	fprintf(f, "AS[%d].speed=%.5lf\n", num, a.speed);
	fprintf(f, "AS[%d].betaspeed=%.5lf\n", num, a.betaspeed);
}

void write_mesh(FILE *f, int num, Mesh & m)
{
	fprintf(f, "Mesh[%d].file_name=%s\n", num, m.file_name);
	fprintf(f, "Mesh[%d].scale=%.5lf\n", num, m.total_scale);
	char pre[100];
	sprintf(pre, "Mesh[%d].translate", num);
	write_triplet1(f, pre, m.total_translation);
	fprintf(f, "Mesh[%d].refl=%.5lf\n", num, trio[num * MAX_TRIANGLES_PER_OBJECT].refl);
	fprintf(f, "Mesh[%d].opacity=%.5lf\n", num, trio[num * MAX_TRIANGLES_PER_OBJECT].opacity);
	for (int i = 0; i < 31; i++)
		if (flag2str[i].name[0])
			fprintf(f, "Mesh[%d].flags.%s=%d\n", num, flag2str[i].name, ((flag2str[i].flag & m.flags)!=0));
}

// saves the current context to the given text file. If it exists, it is overwritten.
int save_context(const char *fn)
{
	FILE *f;
	int i;
	if ((f = fopen(fn, "wt"))==NULL)
		return 0;
	fprintf(f, "%s\n", SAVEFILE_MAGIC);
	write_comments(f);
	fprintf(f, "\n##\n#Global Entries:\n#\n\n");
	fprintf(f, "\n# The scene type, may be FrameBased or TimeBased\n");
	fprintf(f,   "# Time based scenes run for a fixed time, frame based render\n");
	fprintf(f,   "# in fixed amount of frames.\n");
	fprintf(f, "SceneType=%s\n", SceneType==FRAME_BASED?"FrameBased":"TimeBased");
	if (BackgroundMode == BACKGROUND_MODE_FLOOR) {
		fprintf(f, "\n# The background type: may be InfinitePlane or Voxel\n");
		fprintf(f, "Background.type=InfinitePlane\n");
		fprintf(f, "Background.texture=%s\n", tex.get_tex_fn());
		fprintf(f, "Background.daFloor=%.3lf\n", daFloor);
		fprintf(f, "Background.daCeiling=%.3lf\n", daCeiling);
	}
	if (BackgroundMode == BACKGROUND_MODE_VOXEL) {
		fprintf(f, "\n# The background type: may be InfinitePlane or Voxel\n");
		fprintf(f, "Background.type=Voxel\n");
	}
	fprintf(f, "\n# Light source coordinates:\n");
	write_triplet(f, "Light", lx, ly, lz);
	fprintf(f, "\n# Shadowing quality:\n");
	fprintf(f, "ShadowQuality=%s\n");
	fprintf(f, "\n# User position information:\n");
	write_triplet1(f, "User", cur);
	fprintf(f, "alpha=%.6lf\n", alpha);
	fprintf(f, "beta=%.6lf\n", beta);
	fprintf(f, "\n# Physics (1=on)\n");
	fprintf(f, "Physics=%d\n", Physics);
	fprintf(f, "\n# Collision Detection (1=on)\n");
	fprintf(f, "CollDetect=%d\n", CollDetect);
	fprintf(f, "\n# Physics engine parameters\n");
	fprintf(f, "sv_gravity=%.6lf\n", sv_gravity);
	fprintf(f, "sv_air=%.6lf\n", sv_air);
	fprintf(f, "fov=%.6lf\n", fov);
	fprintf(f, "\n# Some vital file names\n");
	fprintf(f, "floor_texture=\"%s\"\n", floor_texture);
	fprintf(f, "default_font=\"%s\"\n", default_font);
	fprintf(f, "\n#\n# Sphere information section\n#\n\n");
	fprintf(f, "spherecount=%d\n", spherecount);
	for (i=0;i<spherecount;i++) write_sphere(f, i, sp[i]);
	fprintf(f, "\n#\n# Animation structure information section\n#\n\n");
	fprintf(f, "anicnt=%d\n", anicnt);
	for (i=0;i<anicnt;i++) write_ani(f, i, AS[i]);
	fprintf(f, "\n");
	fprintf(f, "\n#\n# Mesh information section\n#\n\n");
	fprintf(f, "meshcount=%d\n", mesh_count);
	for (i=0;i<mesh_count;i++) write_mesh(f, i, mesh[i]);
	fclose(f);
	return 1;
}

#ifdef RECORD
// Does a record snapshot - writes down the current values of cur[], alpha and beta along with the current time
void record_do(double time)
{
	// check if the work file should be opened first
	if (rec_init_try) {
		if (!rec_initialized) return;
		//fprintf(rec_file, "%.3lf\t%.6lf\t%.6lf\t%.6lf\t%.6lf\t%.6lf\n", time, cur[0], cur[1], cur[2], alpha, beta);
		fprintf(rec_file, "%.3lf\t%.6lf\t%.6lf\t%.6lf\t%.6lf\t%.6lf\t%.6lf\t%.6lf\t%.6lf\t%d\t%.4f\n",
			time,
			cur[0], cur[1], cur[2],
			alpha,
			beta,
			(double) lx, (double) ly, (double) lz,
			pp_state,
			shader_param);
	} else	{
		rec_init_try = 1;
		rec_file = fopen(RECORD_FILE, "wt");
		rec_initialized = 0;
		if (rec_file!=NULL) {
			rec_initialized = 1;
			record_do(time); // recurse once.
		}
	}
}
#endif

// hash function, which operates on a single line: hashes everything to the first occurence
// of a '=' character (if it does not exist, the whole line is hashed)
static Uint16 hashid(char *s)
{
	register Uint32 acc;
	int i, l;
	char sc;

	l = 0;
	while (s[l]!=0 && s[l]!='=' && s[l]!='[' && s[l]!='.') l++;
	sc=s[l];
	s[l]=0;
	for (acc=0, i=0;i<(l&~1);i++)
		acc += (Uint32) s[i] + (((Uint32) s[i+1])<<8);
	if (l%2)
		acc ^= s[l-1];
	s[l]=sc;
	acc = (acc&0xffff)^(acc>>16);
	return acc;
}

static inline int  line_empty(char *s)
{
	int i;

	i = 0;
	while (s[i]!=0) {
		if (s[i]!='\n' && s[i]!='#' && s[i]!=' ' && s[i]!='\t' && s[i]!='\r') return 0;
		i++;
		}
	return 1;

}

static inline int get_int(char *s)
{
	double f;
	int i;

	for (i=0;s[i]!='=';i++);
	i++;
	if (sscanf(s+i, "%lf", &f)) {
		if (f < -INT_MAX || f > INT_MAX) {
			printf("Value at line `%s' won't fit in the integer type!\n", s);
			return 0;
		} else	return ((int) f);
		}
		else {	printf("Value not read well at line `%s'!\n", s);
			return 0;
			}
}

double get_real(char *s)
{
	double f;
	int i;

	for (i=0;s[i]!='=';i++);
	i++;
	if (sscanf(s+i, "%lf", &f)) return f;
		else {	printf("Value not read well at line `%s'!\n", s);
			return 0.0;
			}
}

static void get_int_triple(char *s, int *x, int *y, int *z)
{
	int i=0;

	while (s[i]!='.') i++;
	i++;
	s += i;
	if (s[0]=='x') *x = get_int(s);
	if (s[0]=='y') *y = get_int(s);
	if (s[0]=='z') *z = get_int(s);
}

static void get_vector(char *s, Vector &a)
{
	int i=0;

	while (s[i]!='.') i++;
	i++;
	s += i;
	if (s[0]=='x') a.v[0] = get_real(s);
	if (s[0]=='y') a.v[1] = get_real(s);
	if (s[0]=='z') a.v[2] = get_real(s);
}

static inline void get_str(char *s, char *so)
{
	int i=0,j;
	while (s[i]!=0 && s[i]!='=') i++;
	if (s[i]==0) { so[0]=0; return; }
	j = ++i;
	while (s[j]!=0) j++;
	j--;
	if (s[j]=='\n') s[j--]=0;
	if (s[j] == '"' && s[i] == '"') {
		++i;
		s[j--] = 0;
	}
	strcpy(so, s+i);
}

static void get_sphere_flags(char *s, int *f)
{
	int i, j;

	j = 0;
	while (s[j]!='=') j++;
	s[j]=0;
	for (i = 0; i < 32; i++)
		if (!strcmp(s, flag2str[i].name)) {
			if (s[j + 1] == '1')
				*f |= flag2str[i].flag;
			else
				*f &= ~flag2str[i].flag;
		}
}

static void get_shadow_quality(char *line, int *res)
{
	char *s = line;
	while (s[0] && s[0] != '=') s++;
	if (s[0]) {
		s++;
		for (int i = 0; i < 3; i++)
			if (!strncmp(s, squalities[i], strlen(squalities[i]))) {
				*res = i;
				return;
			}
	}
	*res = 1; // default choice
}

static int get_index(char *si) 
{
	int res;
	
	while (si[0] != '[') si++;
	if (1 == sscanf(si, "[%d]", &res))
		return res;
	else
		return -1;
}

static void get_sphere_info(char *si, Sphere *sph)
{
	int i;
	char *s;

	for (i=0;si[i]!=']';i++);
	s = si+i+2;
	switch (hashid(s)) {
		case 0x0078: sph->pos.v[0]	=	get_real(s); break;
		case 0x0079: sph->pos.v[1]	=	get_real(s); break;
		case 0x007a: sph->pos.v[2]	=	get_real(s); break;
		case 0x69e1: sph->mov.v[0]	=	get_real(s); break;
		case 0x69e2: sph->mov.v[1]	=	get_real(s); break;
		case 0x69e3: sph->mov.v[2]	=	get_real(s); break;
		case 0x0064: sph->d	=	get_real(s); break;
		case 0x48b5: sph->mass	=	get_real(s); break;
		case 0x38a8: sph->refl	=	get_real(s); break;
		case 0x8cfb: sph->opacity	=	get_real(s); break;
		case 0x0072: sph->r	=	get_int (s); break;
		case 0x0067: sph->g	=	get_int (s); break;
		case 0x0062: sph->b	=	get_int (s); break;
		case 0xd212: sph->AniIndex =	get_int (s); break;
		case 0xa8e8: get_sphere_flags(s+6, &sph->flags);break;
		default:
			printf("LoadContext: The following line has no meaning to me:\n[%s]\n", s); fflush(stdout);
			break;
		}
}

static void get_thorus_info(char *si, Thorus *thor)
{
	int i;
	char *s;

	for (i=0;si[i]!=']';i++);
	s = si+i+2;
	switch (hashid(s)) {
		case 0x47b9: thor->rods = get_int(s); break;
		case 0xd1b1: thor->phi = get_real(s); break;
		case 0x9fc8: thor->warpspeed = get_real(s); break;
		default: {
			get_sphere_info(si, static_cast<Sphere*>(thor));
			break;
		}
	}
	thor->init();
}

static void get_tracer_info(char *si, Tracer *t)
{
	int i;
	char *s;
	for (i = 0; si[i] != ']'; i++);
	s = si + i + 2;
	switch (hashid(s)) {
		case 0xed5a: get_vector(s, t->points[0]); break;
		case 0xee5d: get_vector(s, t->points[1]); break;
		case 0xef5c: get_vector(s, t->points[2]); if (strstr(s, ".z")!=NULL) t->init(); break;
		case 0x5b29: t->thickness = get_real(s); break;
		case 0xb621: t->phasespeed = get_real(s); break;
		default: {
			get_sphere_info(si, static_cast<Sphere*>(t));
			break;
		}
	}
}

static void get_mesh_info(char *basedir, char *si)
{
	static Vector v_translation;
	int i, index;
	char *s;
	sscanf(si, "Mesh[%d]", &index);
	i = 0;
	while (si[i] != '.') i ++;
	s = si + i + 1;// after the dot
	switch (hashid(s)) {
		case 0x3d5d:
		{
			char objfn[256];
			strcpy(objfn, strstr(s, "=") + 1);
			objfn[strlen(objfn)-1]=0;
			// if any directory delimiters are given, use the path provided
			// else use the directory of the .fsv file
			if (!strchr(objfn, '/')) {
				char temp[256];
				strcpy(temp, basedir);
				strcat(temp, objfn);
				strcpy(objfn, temp);
			}
			int l = strlen(objfn);
			//l--;
			while (l--) if (objfn[l] < 32) objfn[l] = 0;
			if (!mesh[index].read_from_obj(objfn)) {
				printf("LoadContext: unable to open mesh object (file: `%s')\n", objfn);
				break;
			}
			break;
		}
		case 0xa8e8: 
		{
			get_sphere_flags(s + 6, &mesh[index].flags); 
			mesh[index].set_flags(mesh[index].flags, -1, -1);
			break;
		}
		case 0x38a8: 
		{
			double refl = get_real(s);
			mesh[index].set_flags(0, refl, -1);
			break;
		}
		case 0x8cfb:
		{
			double opacity = get_real(s);
			mesh[index].set_flags(0, -1, opacity);
			break;
		}
		case 0x96c7:
		{
			double scale = get_real(s);
			mesh[index].scale(scale);
			break;
		}
		case 0x5d0f:
		{
			get_vector(s, v_translation);
			if (strstr(s, ".z")) {
				mesh[index].translate(v_translation);
				mesh[index].bound(1, daFloor, daCeiling);
			}
			break;
		}
		case 0xaddd:
		{
			for (int i = 0; i < mesh[index].triangle_count; i++)
				trio[i + mesh[index].get_triangle_base()].color = get_int(s);
			break;
		}
		default:
			printf("LoadContext: The following line has no meaning to me:\n[%s]\n", s); fflush(stdout);
			break;
	}
}

static void get_ani_info(char *si)
{
	int i, j, index;
	char *s;

	s = si;
	for (i=0;s[i]!='[';i++);
	j = ++i;
	while (s[j]!=']') j++;
	s[j]= 0;
	sscanf(s+i, "%d", &index);
	j+=2;
	s = si + j;

	switch (hashid(s)) {
		case 0x78db: AS[index].cx     = get_real(s); break;
		case 0x79dc: AS[index].cy     = get_real(s); break;
		case 0x7add: AS[index].cz     = get_real(s); break;
		case 0x188a: AS[index].radius = get_real(s); break;
		case 0xa6c5: AS[index].alpha  = get_real(s); break;
		case 0x3b9d: AS[index].beta   = get_real(s); break;
		case 0x9fc8: AS[index].speed  = get_real(s); break;
		case 0x4e2e: AS[index].betaspeed = get_real(s); break;
		default:
			printf("LoadContext: The following line has no meaning to me:\n[%s]\n", s); fflush(stdout);
			break;
		}
}

// gets a context from a fract-written text file. On failure, returns 0.
// Loads everything (including textures)
int load_context(const char *fn)
{
	FILE *f;
	char line[256];
	char basedir[256];
	int shb = 0;
	
	strcpy(basedir, fn);
	int i = strlen(basedir)-1 ;
	while (i && basedir[i] != '/') i--;
	if (i) i++;
	basedir[i] = 0;
	
	int input_type = 0;
	
	cd_frames = 0;
	if ((f = fopen(fn, "rt")) == NULL) {
		printf("LoadContext: Can't open context file `%s'!\n", fn);
		return 0;
	}
	fgets(line, 256, f);
	if (strncmp(line, SAVEFILE_MAGICn, 8)) {
		printf("LoadContext: Can't open context file [%s]: bad magic!\n", fn);
		return 0;
	}
	while (!feof(f)) {
		fgets(line, 256, f);
		if (!line_empty(line) && line[0]!='#') {
			/*printf("%s\n", line);
			fflush(stdout);*/
			switch (hashid(line)) {
			/* Scene Type */
				case 0x404d: SceneType = 
					strstr(line, "TimeBased") ? TIME_BASED : FRAME_BASED; break;
			/* LIGHT */
				case 0xadf1: get_int_triple (line, &lx, &ly, &lz); break;
				
			/* SHADOWING QUALITY */
				case 0x00aa: get_shadow_quality(line, &g_shadowquality); break;
				
			/* CAMERA */
				case 0x4b9e: get_vector(line, cur); break;
				case 0xa6c5: alpha = get_real(line); break;
				case 0x3b9d: beta = get_real(line); break;
			/* Physics: */
				case 0x9501: Physics = get_int (line); break;
				case 0xa3e0: CollDetect = get_int (line); break;
				case 0xdf4d: sv_gravity = get_real(line); break;
				case 0x1386: sv_air = get_real(line); break;
				case 0xe5a3: fov = get_real(line); break;
			/* Textures: */
				case 0x314d: get_str (line, floor_texture); break;
				case 0x9bff: /*get_str (line, default_font);*/ break;
			/* Spheres: */
				case 0x414c: spherecount = get_int (line); break;
				case 0x1665: get_sphere_info(line, sp + get_index(line)); break;
			/* Ani structures: */
				case 0x1e7f: anicnt = get_int (line); break;
				case 0x5394: get_ani_info(line); break;
			/* Meshes: */
				case 0x6c15: mesh_count = get_int(line); break;
				case 0x418c: get_mesh_info(basedir, line); break;
			/* Thori: */
				case 0x5e2a: thors_count = get_int(line); break;
				case 0x3387: get_thorus_info(line, thor + get_index(line)); break;
			/* Tracers: */
				case 0x0d79: tracers_count = get_int(line); break;
				case 0x0f63: get_tracer_info(line, tracer + get_index(line)); break;
			/* MISC: */
				case 0x2988: input_type = SAVLOAD_COORDS; shb = 1; break;
				case 0xc0df: input_type = SAVLOAD_COORDS_106; shb = 1; break;
				default:
					printf("LoadContext: The following line has no meaning to me:\n[%s]\n", line);
					fflush(stdout);
					break;
			}
		}
	
		if (shb) break;
	}
	if (input_type == SAVLOAD_COORDS) {
		cd_frames = 0;
		while (fscanf(f, "%lf%lf%lf%lf%lf%lf", 	&cd[cd_frames].time  , cd[cd_frames].cam, cd[cd_frames].cam+1,
							cd[cd_frames].cam+2, &cd[cd_frames].alpha, &cd[cd_frames].beta)==6) 
							cd[cd_frames++].is_106 = false;
	}
	if (input_type == SAVLOAD_COORDS_106) {
		cd_frames = 0;
		while (fscanf(f, "%lf%lf%lf%lf%lf%lf%lf%lf%lf%d%f",&cd[cd_frames].time  , 
			cd[cd_frames].cam, cd[cd_frames].cam+1,
			cd[cd_frames].cam+2, &cd[cd_frames].alpha, &cd[cd_frames].beta,
			cd[cd_frames].light, cd[cd_frames].light+1, cd[cd_frames].light+2,
			&cd[cd_frames].pp_state, &cd[cd_frames].shader_param)==11) 
			cd[cd_frames++].is_106 = true;
	}
	if (input_type == SAVLOAD_COORDS || input_type == SAVLOAD_COORDS_106) {
		if (cd[0].time == cd[1].time) {// non-smoothed input detected
			int i = 0;
			while (i < cd_frames) {
				int j = i + 1;
				while (j < cd_frames && cd[i].time == cd[j].time) j++;
				if (j < cd_frames) {
					for (int k = i + 1; k < j; k++) {
						float f = (k-i)/(float)(j-i);
						cd[k].time = (1-f) * cd[i].time + f * cd[j].time;
					}
				}
				i = j;
			}
		}
	}
	fclose(f);
	if (input_type)
		return (input_type);
		else
		return (SAVLOAD_OK);
}

static void check_state(int & state, int newstate)
{
	if (newstate != state) {
		int diff = newstate ^ state;
		if (diff == BLUR_ID_CONTINUOUS || diff == BLUR_ID_NORMAL) {
			//cycle_blur_mode();
			if (!apply_blur) {
				if (diff == BLUR_ID_CONTINUOUS)
					set_blur_method(BLUR_CONTINUOUS);
				else
					set_blur_method(BLUR_DISCRETE);
				blur_reinit();
			}
			apply_blur = !apply_blur;
			
		}
	}
	state = newstate;
}

bool load_frame(int frame_no, double time, int mySceneType)
{
	int i;
	if (mySceneType == FRAME_BASED) {
		if (frame_no>=cd_frames) return false;
		play_time = cd[frame_no].time;
		for (i=0;i<3;i++) cur.v[i] = cd[frame_no].cam[i];
		alpha = cd[frame_no].alpha;
		beta  = cd[frame_no].beta;
		if (cd[frame_no].is_106) {
			check_state(system_pp_state, cd[frame_no].pp_state);
			shader_param = cd[frame_no].shader_param;
			lx = (int) (cd[frame_no].light[0]);
			ly = (int) (cd[frame_no].light[1]);
			lz = (int) (cd[frame_no].light[2]);
		}
		return true;
	}
	if (mySceneType == TIME_BASED) {
		if (time > cd[cd_frames-1].time) return false;
		int l = 0, r = cd_frames - 1;
		while (r - l > 1) {
			int m = (l + r) / 2;
			if (cd[m].time > time) {
				r = m;
			} else {
				l = m;
			}
		}
		double f = (time - cd[l].time) / (cd[r].time - cd[l].time);
		for (i = 0; i < 3; i++) {
			cur.v[i] = cd[l].cam[i] * (1-f) + cd[r].cam[i] * f;
		}
		alpha = cd[l].alpha * (1 - f) + cd[r].alpha * f;
		beta =  cd[l].beta  * (1 - f) + cd[r].beta  * f;
		if (cd[l].is_106) {
			check_state(system_pp_state, cd[l].pp_state);
			shader_param = cd[l].shader_param * (1 - f) + cd[r].shader_param * f;
			lx = (int) (cd[l].light[0] * (1 - f) + cd[r].light[0] * f);
			ly = (int) (cd[l].light[1] * (1 - f) + cd[r].light[1] * f);
			lz = (int) (cd[l].light[2] * (1 - f) + cd[r].light[2] * f);
		}
		return true;
	}
	return false;
}

void saveload_close(void)
{
#ifdef RECORD
	if (rec_initialized)
		fclose(rec_file);
#endif
}


void generate_coords_bench(void)
{
	cd_frames = 1000;
	for (int i = 0; i < cd_frames; i++) {
		double t = (double) i / cd_frames, tt;
		cd[i].time = t * 20.0;
		tt = t;
		tt = (1-cos(tt*M_PI/2));
		
		double radius = 200 - 80*tt;
		double angle = tt*7;
		Vector xx = Vector(120, 20+100*sin(t*M_PI), 200) + 
				Vector(radius,0,0) * sin(angle) + Vector(0,0,radius)*cos(angle);
		for (int j = 0; j < 3; j++)
			cd[i].cam[j] = xx[j];
		cd[i].alpha = angle+M_PI;
		cd[i].beta  = 0;
		cd[i].is_106 = true;
		Vector ll = Vector(256,125,256);
		
		if (t > 0.3) {
			double q = (t-0.3)/0.7;
			double rad;
			if (q < 0.2) {
				rad = 70 * (q/0.2); 
			} else {
				rad = 70 + (q-0.2) * 200;
			}
			double angle = q * 12;
			ll += Vector(rad, 0, 0) * sin(angle) + Vector(0, 0, -rad) * cos(angle);
		}
		
		for (int j = 0; j < 3; j++) 
			cd[i].light[j] = ll[j];
		cd[i].pp_state = 0;
		cd[i].shader_param = 0;
		/*if (t > 0.85) {
			cd[i].pp_state = SHADER_ID_GAMMA;
			cd[i].shader_param = sqrt(1-(t-0.85)/0.15);
		}
		*/
		if (t > 0.85) {
			cd[i].pp_state = SHADER_ID_SHUTTERS;
			cd[i].shader_param = (t-0.85)/0.15;
		}
	}
}


void generate_coords_oldbench(void)
{
	cd_frames = 2000;
	for (int i = 0; i < cd_frames; i++) {
		double t = (double) i / cd_frames;
		double angle, re=200;
		cd[i].time = t * 30.0;
		cd[i].is_106 = true;
		cd[i].pp_state = 0;
		cd[i].shader_param = 0;
		Vector c(200, 35, 330);
		c += Vector(10, 0, 0) * sin(t*8) + Vector(0,0,10)*cos(t*8);
		
		//angle = t* 10;
		double tt = t;
		angle = tt * sqrt(tt*400);
		if (tt > 0.28) {
			double q = (tt - 0.28)/0.72;
			//angle += (t-0.28)*20;
			angle += 10 * (1-cos(q*M_PI/2));
		}
		re = 220 + 80 * cos(tt);
		c += Vector (re,0,0) * sin(angle) + Vector (0, 0, re) * cos(angle) + Vector(0, 60, 0) * sqrt(tt);
		
		
		
		for (int j = 0; j < 3; j++)
			cd[i].cam[j] = c[j];
		
		cd[i].light[0] = 200;
		cd[i].light[1] = 100;
		cd[i].light[2] = 500;
		cd[i].alpha = M_PI + angle;
		cd[i].beta = 0.1;
		
		if (t > 0.55) {
			cd[i].pp_state |= BLUR_ID_CONTINUOUS;
		}
		
		if (t > 0.84) {
			cd[i].pp_state |= SHADER_ID_EDGE_GLOW;
		}
		
		if (t > 0.90) {
			cd[i].pp_state |= SHADER_ID_GAMMA;
			cd[i].shader_param = sqrt(1-(t-0.90)/0.10);
		}
	}
}

void generate_coords(void)
{
	if (spherecount == 6) {
		generate_coords_bench();
	} else {
		generate_coords_oldbench();
	}
}

void save_coords(const char *fn)
{
	FILE *rec_file = fopen(fn, "wt");
	fprintf(rec_file, "coords106=(\n");
	for (int i=0; i < cd_frames; i++)
	fprintf(rec_file, "%.3lf\t%.6lf\t%.6lf\t%.6lf\t%.6lf\t%.6lf\t%.6lf\t%.6lf\t%.6lf\t%d\t%.4f\n",
		cd[i].time,
		cd[i].cam[0], cd[i].cam[1], cd[i].cam[2],
		cd[i].alpha,
		cd[i].beta,
		cd[i].light[0], cd[i].light[1], cd[i].light[2],
		cd[i].pp_state,
		cd[i].shader_param);
	fprintf(rec_file, ");\n");
	fclose(rec_file);
}
