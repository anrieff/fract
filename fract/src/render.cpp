/***************************************************************************
 *   Copyright (C) 2004 by Veselin Georgiev                                *
 *   vesko@ViruS                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *-------------------------------------------------------------------------*
 *                                                                         *
 *  render.cpp - all rendering routines, seperated for non-SSE and SSE-    *
 *               enabled CPU's                                             *
 *                                                                         *
 ***************************************************************************/



#include <stdlib.h>
#include <string.h>
#include <math.h>

#define RENDER_CPP
#include "MyGlobal.h"
#include "MyTypes.h"
#include "antialias.h"
#include "bitmap.h"
#include "blur.h"
#include "cmdline.h"
#include "common.h"
#include "cpu.h"
#include "cvars.h"
#include "fract.h"
#include "gfx.h"
#include "infinite_plane.h"
#include "mesh.h"
#include "light.h"
#include "konsole.h"
#include "object.h"
#include "profile.h"
#include "physics.h"
#include "radiosity.h"
#include "random.h"
#include "render.h"
#include "rgb2yuv.h"
#include "scene.h"
#include "shaders.h"
#include "shadows.h"
#include "sphere.h"
#include "threads.h"
#include "thorus.h"
#include "tracer.h"
#include "triangle.h"
#include "vector3.h"
#include "vectormath.h"
#include "voxel.h"

#include "cross_vars.h"

char floor_texture[64] = "data/2.bmp";

//Uint32 savebuffer[20*30][400*300]; // 20 s at 30 fps @ 400x300

SSE_ALIGN(Uint32 spherebuffer[RES_MAXX*RES_MAXY]);
SSE_ALIGN(unsigned short fbuffer[RES_MAXX*RES_MAXY]);
SSE_ALIGN(unsigned short sbuffer[RES_MAXX*RES_MAXY]);
int RowInfo[RES_MAXY];
int RowMax[RES_MAXY], RowMin[RES_MAXY];
ORNode obj_rows[2*MAX_OBJECTS];
int OR_size;
ObjectArray allobjects;
int obj_count;
int UseThreads = -1;
int r_shadows  = 1;
int stereo_type = STEREO_TYPE_NONE;
int stereo_mode = STEREO_MODE_NONE;
bool stereo_crossed = false;
Uint32 anaglyph_left_mask = 0xff0000, anaglyph_right_mask = 0x0000ff;
double stereo_depth = 1e6;
double stereo_separation = 6;
Uint32 *stereo_buffer = NULL;
bool prepass_photorealistic = true;

ThreadPool thread_pool;

#ifdef TEX_OPTIMIZE
SSE_ALIGN(ML_Entry ml_buffer[(RES_MAXX/ML_BUFFER_GRAN)*(RES_MAXY/ML_BUFFER_GRAN)]);
#endif


#ifdef ONLY_FIRST_FRAME
int checksum_done = 0;
#endif

#ifdef FSTAT
static int good, worse, realbad;
#endif
#ifdef DUMPPREPASS
extern bool wantdump;
#endif
extern int threads_first;

#define genrender_asm
#include "x86_asm.h"
#undef genrender_asm

/*
	Initializes the raytracer rendering: loads textures, calculates mipmaps, etc.
	returns 0 on success, 1 on failure
 */
int render_init(void)
{
	if (!tex.load_bmp(floor_texture)) {
		printf("Unable to load main texture (%s), bailing out\n", floor_texture);
		return 1;
	}
 	loop_mode = option_exists("--loop")||option_exists("--loops");
 	if (option_exists("--loops")) {
 		loops_remaining = option_value_int("--loops");
 	}
	if (option_exists("--glasses")) {
		const char *val = option_value_string("--glasses");
		bool failed = true;
		unsigned masks[2] = {0, 0}, k = 0;
		if (val && strchr(val, ',')) {
			failed = false;
			for (unsigned i = 0; i < strlen(val); i++) {
				switch (val[i]) {
					case ',':
					{
						k++;
						if (k > 1) failed = true;
						break;
					}
					case 'r': masks[k] |= 0xff0000; break;
					case 'g': masks[k] |= 0x00ff00; break;
					case 'b': masks[k] |= 0x0000ff; break;
					default:
						failed = true;
						break;
				}
				if (failed) break;
			}
			if (k != 1) failed = true;
		}
		if (!failed) {
			anaglyph_left_mask = masks[0];
			anaglyph_right_mask = masks[1];
		}
	}
	return 0;
}

/*
 *	This handles camera movement using current direction vector of the camera and the supplied
 *	`angle' param (angle = 0 means "move straight ahead"; angle = -M_PI means 'move backwards' and so on.
 */
void move_camera(double angle)
{double a, b;

 a = CVars::alpha + angle;
 b = CVars::beta;
 if (angle == M_PI/2 || angle == -M_PI/2) b = 0;
 if (angle == M_PI)	b = -b;
 Vector x(sin(a)*cos(b), sin(b), cos(a)*cos(b));
 x.scale(delta*speed);
 cur += x;
 camera_moved = 1;
}

/*
 *	Rotates the camera vector, depending on last frame generation time and mouse sensitivity
 */
void rotate_camera(double alphai, double betai)
{
 CVars::alpha += (alphai*delta*anglespeed);
 CVars::beta +=  (betai *delta*anglespeed);
 camera_moved = 1;
}

void check_params(void)
{
 double y = cur[1];
 if (y<daFloor+1) y = daFloor+1;
 if (y>daCeiling -1) y = daCeiling -1;
 cur = Vector(cur[0], y, cur[2]);
 if (CVars::beta< beta_limit_low ) CVars::beta = beta_limit_low;
 if (CVars::beta> beta_limit_high) CVars::beta = beta_limit_high;
}

double czero(double x)
{
 if (x<0.0001) x = 9999;
 return x;
}

Uint32 *get_frame_buffer(void)
{
	return framebuffer;
}

int determine_xres(int x)
{
	return ((stereo_type == STEREO_TYPE_PARALLEL) ? (2*x+8) : x);
}

/*
 *	This code deals with post-frame generation stuff, which is common for the P5 and SSE version of DrawIt
 */
#ifdef ACTUALLYDISPLAY
void postframe_do(SDL_Surface *p, SDL_Overlay *ov)
#else
void postframe_do(void)
#endif
{
	static char buff[1024];
#ifdef ACTUALLYDISPLAY
	SDL_Rect ddestrect;
	
	ddestrect.x = 0;
	ddestrect.y = 0;
	ddestrect.w = xres();
	if (stereo_type == STEREO_TYPE_PARALLEL) {
		ddestrect.w = ddestrect.w * 2 + 8;
	}
	ddestrect.h = yres();
#endif

	prof_enter(PROF_ANTIALIAS);
	antialias(framebuffer);
	prof_leave(PROF_ANTIALIAS);
	if (CVars::brightness != 1.0) {
		shader_clippedgamma(framebuffer, xres(), yres(), CVars::brightness);
	}
	if (pp_state) {
		bool have_mt_shader = cpu.count > 1 && 0 != (pp_state & SUPPORTED_MT_SHADERS);
		int st_pp_state = pp_state;
		if (have_mt_shader) st_pp_state &= ~SUPPORTED_MT_SHADERS;
		
		if (st_pp_state & SHADER_ID_INVERSION)
			shader_inversion(framebuffer, framebuffer, xres(), yres(), xres()/5);
		if (st_pp_state & SHADER_ID_EDGE_GLOW) 
			shader_edge_glow(framebuffer, framebuffer, xres(), yres());
		if (st_pp_state & SHADER_ID_SOBEL)
			shader_sobel(framebuffer, framebuffer, xres(), yres());
		if (st_pp_state & SHADER_ID_FFT_FILTER)
			shader_FFT_Filter(framebuffer, framebuffer, xres(), yres());
		if (st_pp_state & SHADER_ID_BLUR)
			shader_blur(framebuffer, framebuffer, xres(), yres(), 7);
		if (st_pp_state & SHADER_ID_OBJECT_GLOW) {
			antialias(fb_copy);
			shader_object_glow(framebuffer, fb_copy, 0xffffff, xres(), yres(), 0.36);
		}
		if (st_pp_state & SHADER_ID_SHUTTERS) {
			shader_shutters(framebuffer, 0x0, xres(), yres(), shader_param);
		}
		if (st_pp_state & SHADER_ID_GAMMA) {
			shader_gamma(framebuffer, xres(), yres(), shader_param);
		}
		
		if (have_mt_shader) {
			class ShaderThreads : public Parallel {
				Uint32 *fbuff;
				int pp_state;
				float shd_param;
				Barrier b;
			public:
				ShaderThreads(Uint32 *_fbuff, int xpp, float parm, int cpu_count) : 
					fbuff(_fbuff), pp_state(xpp), shd_param(parm), b(cpu_count)
				{
				}
				void entry(int ti, int tc)
				{
					mt_fb_memcpy(shader_tmp, fbuff, xres(), yres(), ti, tc);
					b.checkout();
					if (pp_state & SHADER_ID_SOBEL) {
						shader_sobel(shader_tmp, fbuff, xres(), yres(), ti, tc);
					}
					
					if (pp_state & SHADER_ID_GAMMA) {
						shader_gamma(fbuff, xres(), yres(), shd_param, ti, tc);
					}
					
				}
			} shader_threads(framebuffer, pp_state, shader_param, cpu.count);
			thread_pool.run(&shader_threads, cpu.count);
		}
		
	}
#ifdef MAKE_CHECKSUM
	prof_enter(PROF_CHECKSUM);
#ifdef ONLY_FIRST_FRAME
	if (!checksum_done) {
		checksum_done=1;
#endif // ONLY_FIRST_FRAME
	 	for (int i=0;i<xres()*yres();i++) {
 			cksum += (framebuffer[i]&0xffff)^(framebuffer[i]>>16);
		}
#ifdef ONLY_FIRST_FRAME
	}
#endif // ONLY_FIRST_TIME
	prof_leave(PROF_CHECKSUM);
#endif // MAKE_CHECKSUM
#ifdef BLUR
	if (apply_blur) {
		prof_enter(PROF_BLUR_DO);
		blur_do(framebuffer, framebuffer, xres(), yres(), vframe, thread_pool);
		prof_leave(PROF_BLUR_DO);
	}
#endif
#ifdef DUMPPREPASS
	if (wantdump) {
		wantdump = false;
		RawImg dump(xres(), yres(), framebuffer);
		dump.save_bmp("final_dump.bmp");
	}
#endif
	if (savevideo) {
		static int video_frame = 0;
		RawImg frame(xres(), yres(), framebuffer);
		char fn[100];
		sprintf(fn, "video/frame_%03d.bmp", video_frame++);
		frame.save_bmp(fn);
	}
#ifdef ACTUALLYDISPLAY
#ifdef SHOWFPS
	prof_enter(PROF_SHOWFPS);
	if (developer && stereo_mode == STEREO_MODE_NONE) {
		fpsfrm++;
		if (get_ticks()-fpsclk>FPS_UPDATE_INTERVAL || not_fps_written_yet) {
			not_fps_written_yet = 0;
			sprintf(buff, "fps: %.1lf", (double) fpsfrm / ((get_ticks()-fpsclk)/1000.0));
			font0.printxy(p, framebuffer, 0, 0, FPS_COLOR, 0.8, buff);
			fpsfrm = 0;
			fpsclk = get_ticks();
		} else {
			font0.printxy(p, framebuffer, 0, 0, FPS_COLOR, 0.8, buff);
		}
	}
	prof_leave(PROF_SHOWFPS);
#endif // SHOWFPS
#ifdef SHOWFSAA
	if (developer && stereo_mode == STEREO_MODE_NONE) {
		double tt = bTime() - last_fsaa_change;
		if (tt < 4) {
			double opacity = tt < 3 ? 0.75 : 0.75 - 0.75*(tt-3);
			char fsaabuf[100];
			sprintf(fsaabuf, "FSAA: %s", fsaa_name);
			font0.printxy(p, framebuffer, xres()-(int)(strlen(fsaabuf)*11)-5, 0, 0xdddddd, opacity, fsaabuf);
		}
	}
#endif
	if (developer && stereo_mode == STEREO_MODE_NONE && CVars::crosshair) {
		for (int j = -6; j <= 6; j++)
			for (int i = -6; i <= 6; i++) {
				Uint32 &pixel = framebuffer[(yres()/2+j)*xres() + xres()/2 + i];
				double dist = sqrt(i*i+j*j+0.0);
				if (fabs(dist-5.0) <= 1.0) {
					float intensity = 1.0f - fabs(dist-5.0);
					pixel = blend(0xffffff, pixel, intensity);
				}
			}
	}
#ifdef DEBUG
	if (developer && node_arr.size()) {
		for (int i = 0; i < node_arr.size(); i++) {
			font0.printxy(p, framebuffer, node_arr[i].x, node_arr[i].y, node_arr[i].color, 0.8f, "%d", node_arr[i].number);
		}
	}
#endif // DEBUG
	if (konsole.visible())
		konsole.render(p, framebuffer);
#endif // ACTUALLYDISPLAY
#ifdef ACTUALLYDISPLAY
	if (stereo_mode == STEREO_MODE_NONE || stereo_mode == STEREO_MODE_FINAL) {
		Uint32 *thebuffer = stereo_mode == STEREO_MODE_FINAL ? stereo_buffer: framebuffer;
	if (ov==NULL) { // use the surface directly
		surface_lock(p);
		
		prof_enter(PROF_MEMCPY);
		memcpy(p->pixels, thebuffer, p->h*p->w*4);
		prof_leave(PROF_MEMCPY);
		
		surface_unlock(p);
		
		prof_enter(PROF_SDL_FLIP);
		SDL_Flip(p);
		prof_leave(PROF_SDL_FLIP);
	} else { // use the overlay
		SDL_LockYUVOverlay(ov);
		
		prof_enter(PROF_CONVERTRGB2YUV);
		ConvertRGB2YUV((Uint32 *) ov->pixels[0], thebuffer, p->h*p->w);
		prof_leave(PROF_CONVERTRGB2YUV);

		SDL_UnlockYUVOverlay(ov);
		
		prof_enter(PROF_DISPLAYOVERLAY);
		SDL_DisplayYUVOverlay(ov, &ddestrect);
		prof_leave(PROF_DISPLAYOVERLAY);
	}
	} else { // not final mode in stereoscopic view:
		if (!stereo_buffer) {
			int buffersize = yres() * determine_xres(xres());
			stereo_buffer = (Uint32*) malloc(buffersize*4);
			memset(stereo_buffer, 0, buffersize*4);
		}
		if (stereo_type == STEREO_TYPE_PARALLEL) {
			int xx = xres();
			bool isleft = stereo_mode == STEREO_MODE_LEFT;
			int xoffset = (stereo_crossed ^ isleft) ? 0 : xx + 8;
			
			for (int j = 0; j < yres(); j++) 
				memcpy(&stereo_buffer[xoffset + j * (xx * 2+8)], &framebuffer[j * xx], xx * 4);
		} else {
			int size = xres() * yres();
			Uint32 overlapping = anaglyph_left_mask & anaglyph_right_mask;
			Uint32 omask = 0;
			if (overlapping) {
				omask = overlapping & 0xfefefe;
			}
			
			if (stereo_mode == STEREO_MODE_LEFT) {
				for (int i = 0; i < size; i++) {
					Uint32 value = framebuffer[i] & anaglyph_left_mask;
					if (overlapping) {
						value = (value & ~overlapping) | ((value & omask) >> 1);
					}
					stereo_buffer[i] = value;
				}
			} else {
				for (int i = 0; i < size; i++) {
					Uint32 value = framebuffer[i] & anaglyph_right_mask;
					if (overlapping) {
						value = (value & ~overlapping) | ((value & omask) >> 1);
					}
					stereo_buffer[i] += value;
				}
			}
		}
	}
#endif // ACTUALLYDISPLAY
	if (stereo_mode == STEREO_MODE_NONE || stereo_mode == STEREO_MODE_FINAL) vframe++;
#ifdef FSTAT
	int all = good + worse + realbad;
	printf("frame %6d: good/bad/realbad = %7.3lf%%/%7.3lf%%/%7.3lf%%\n", vframe,
	(double) 100.0*good    / ((double) all),
	(double) 100.0*worse   / ((double) all),
	(double) 100.0*realbad / ((double) all)); fflush(stdout);
#endif
}

/*
 *	This routine does the common stuff, identical for both the P5 and SSE version of Drawit
 */

void preframe_do(Uint32 *ptr, const Vector& lw, bool do_projection)
{
	int i, j;
	double btm, ttm;
	Vector obex;
	
	ysqrd_ceil = (int) (sqr(daCeiling-light.p[1]));
	ysqrd_floor= (int) (sqr(light.p[1]-daFloor)  );
	
	prof_enter(PROF_PREFRAME_MEMSET);
	memset(ptr, 0, xsize_render(xres())*ysize_render(yres())*4);
	prof_leave(PROF_PREFRAME_MEMSET);
	// preframe things -----------------------------------
	prof_enter(PROF_SPHERE_THINGS);
	if (CVars::animation) {
		btm = bTime();
		if (stereo_mode <= STEREO_MODE_LEFT) {
			if (cd_frames && vframe % cd_frames == 0) { // first frame
				for (i=0;i<spherecount;i++) {
					if (sp[i].flags & ANIMATED) {
						calculate_XYZ(sp+i, btm, obex);
						sp[i].pos = obex;
						sp[i].mov.zero();
						sp[i].time = btm;
					}
					sp[i].time = btm;
				}
				camera_moved = 1;
			}
			if (!cd_frames || vframe % cd_frames != 0) { // every frame but the first
				for (i=0;i<spherecount;i++) {
					if (sp[i].flags & ANIMATED) {
						calculate_XYZ(sp+i, btm, obex);
						if (btm - sp[i].time == 0) sp[i].mov.zero();
						else sp[i].mov = (obex-sp[i].pos) * (1.0/(btm - sp[i].time));
						camera_moved = 1;
					}
				}
			}
		}
	}
	prof_leave(PROF_SPHERE_THINGS);
	prof_enter(PROF_PHYSICS);
	if (CVars::animation) {
		physics_preprocess_hooks(btm);
		if (CVars::physics && (SceneType == TIME_BASED || developer) && stereo_mode <= STEREO_MODE_LEFT) { // should we process physics?
			camera_moved = 1;
			for (i=0;i<spherecount;i++)
				apply_gravity(sp+i, btm); // first of all, Gravity
			for (i=0;i<spherecount;i++)
				apply_air(sp+i, btm, APPLY_ALL, 1);    // air resistance
			if (CVars::collisions)	{		 // do we process collisions?
			/* The algorythm is simple:
				We iterate thru all collideable spheres and check for collision.
				If a collision occurs, we update vectors (and other data), and set the collided balls'
				time to the moment of the collision. The balls are ready to collide again within the same
				time segment of the frame, but now they differ from the others, being advanced a little
				in the future (this is very important). After everything is completed, all balls' times
				are synced to the new time
			*/
				//first of all, we make a list with all the spheres, which "move"
				
				check_moving();
				for (j=0;j<spherecount;j++) if (sp[j].flags & COLLIDEABLE)
					for (i=j+1;i<spherecount;i++) if ((sp[i].flags & COLLIDEABLE) && can_collide(j, i))
						if ((ttm=collide(sp+i, sp+j, btm))>0) // collision detected
							process_incident(sp+i, sp+j, ttm);
			}
			/*
				Here is where advance takes place. All uncollided (and some collided, too)
				spheres are moved with respect to the remaining time to the sync time and
				their movement vectors, too.
			*/
			for (i=0;i<spherecount;i++)
				advance(sp+i, btm);
		}
		physics_postprocess_hooks(btm);
	}
	prof_leave(PROF_PHYSICS);
	prof_enter(PROF_SORT);
	allobjects.clear(cur);
	if (!g_view_tracers) {
		for (i = 0; i < spherecount; i++) {
			allobjects += &sp[i];
		}
		for (TriangleIterator t; t.ok(); ++t) {
			t->recalc_normal();
			t->recalc_zdepth(cur);
			allobjects += t.current();
		}
		for (i = 0; i < thors_count; i++) 
			allobjects += &thor[i];
		
		if (BackgroundMode == BACKGROUND_MODE_VOXEL)
			allobjects += voxel_water_object();
	} else {
		for (i = 0; i < tracers_count; i++)
			allobjects += &tracer[i];
	}
	allobjects.sort();
	prof_leave(PROF_SORT);
	
	prof_enter(PROF_SHADOWED_CHECK);
	check_for_shadowed_spheres();
	prof_leave(PROF_SHADOWED_CHECK);
	
	for (i=0;i<allobjects.size();i++) {
		allobjects[i]->id = i;
		if (allobjects[i]->get_type() == OB_SPHERE) {
			Sphere *s = (Sphere*)allobjects[i];
			pdlo[i] 	= lw.distto(s->pos);
			sp[i].dist 	= cur.distto(s->pos);
		}
	}
	if (do_projection) {
		prof_enter(PROF_PROJECT);
		for (i=0;i<RES_MAXY;i++) {
			RowMin[i] = +65536;
			RowMax[i] = -65536;
		}
		OR_size = 0;
		for (i=0;i<allobjects.size();i++) {
			int y0 = 0, y1 = ysize_render(yres())-1;
			project_it(allobjects[i], num_sides+i, ptr, cur, w, xsize_render(xres()), 
				ysize_render(yres()), i+1, y0, y1);
			if (y1 >= 0) {
				obj_rows[OR_size].object_id = i;
				obj_rows[OR_size].y = y0 - 1;
				obj_rows[OR_size++].type = OPENING;
				obj_rows[OR_size].object_id = i;
				obj_rows[OR_size].y = y1 + 1;
				obj_rows[OR_size++].type = CLOSING;
			}
		}
		prof_leave(PROF_PROJECT);
	
		prof_enter(PROF_YSORT);
		sort(obj_rows, OR_size);
		prof_leave(PROF_YSORT);
	
		if (is_adaptive_fsaa()) {
			prof_enter(PROF_ANTIBUF_INIT);
			antibuffer_init(ptr, xsize_render(xres()), ysize_render(yres()));
			prof_leave(PROF_ANTIBUF_INIT);
		}
	}
	
	
	prof_enter(PROF_PASS);
	pass_pre(pdlo, pith(lw, cur), lw);
	prof_leave(PROF_PASS);
	
	prof_enter(PROF_MESHINIT);
	mesh_frame_init(cur, lw);
	prof_leave(PROF_MESHINIT);

	
	if (pp_state & SHADER_ID_OBJECT_GLOW) {
		shader_prepare_for_glow("mesh:0", ptr, fb_copy, xsize_render(xres()), ysize_render(yres()));
	}
#ifdef FSTAT
	good = worse = realbad = 0;
#endif
#ifdef DUMPPREPASS
	if (wantdump) {
		RawImg dump(xsize_render(xres()), ysize_render(yres()), ptr);
		dump.save_bmp("prepass_dump.bmp");
	}
#endif
	if (option_exists("--radiosity")) {
		radiosity_calculate(thread_pool);
	}
}

/*
 *	Parameter initialisation routine - initialises the vector grid basics and interpolation increasers
 */
void bash_preframe(Vector& lw, Vector& tt, Vector& ti, Vector& tti)
{
	lw = light.p;
	if (stereo_mode == STEREO_MODE_LEFT) {
		cur -= Vector(stereo_separation * 0.5 * sin(CVars::alpha+M_PI/2), 0, stereo_separation * 0.5 * cos(CVars::alpha+M_PI/2));
		CVars::alpha -= M_PI/2-atan(stereo_depth / (stereo_separation * 0.5));
	}
	if (stereo_mode == STEREO_MODE_RIGHT) {
		cur += Vector(stereo_separation * 0.5 * sin(CVars::alpha+M_PI/2), 0, stereo_separation * 0.5 * cos(CVars::alpha+M_PI/2));
		CVars::alpha += M_PI/2-atan(stereo_depth / (stereo_separation * 0.5));
	}
	calc_grid_basics(cur, CVars::alpha, CVars::beta, w);
	tt = w[0];
	ti = (w[1]-w[0])*(1.0/((double) xsize_render(xres())));
	tti= (w[2]-w[0])*(1.0/((double) xsize_render(yres())));
	if (use_shader)
		pp_state = user_pp_state | system_pp_state;
	else
		pp_state = system_pp_state;
	infinite_plane_perframe_init();
}

void render_spheres_init(unsigned short *fbuffer)
{
	int xr = xsize_render(xres()), yr = ysize_render(yres());
	prof_enter(PROF_MEMSETS);
	if (cpu.count == 1) memset(fbuffer, 0, xr*yr*sizeof(unsigned short));
	memset(RowInfo, 0, sizeof(RowInfo));
#ifdef TEX_OPTIMIZE
	memset(ml_buffer, 0, sizeof(ML_Entry)*(xr*yr/(ML_BUFFER_GRAN*ML_BUFFER_GRAN)));
#endif	
	prof_leave(PROF_MEMSETS);
}

// this is for the SSE version of the renderer, too. Renders the spheres in fbuffer and puts some
// blending data in fbuffer (zeros in fbuffers means there is no sphere for the specified pixel; see
// merge_rows in x86_asm.h for details)

// NOTE: this uses a modified code from DrawIt_P5
void render_spheres(Uint32 *fb, unsigned short *fbuffer,
		const Vector& in_tt, const Vector& ti, const Vector& in_tti, int thread_idx, InterlockedInt& lock)
{
	int i, j, xr, yr, dropped, backd, ii, x_start, x_end;
	Uint32 *start_fb = fb;
	Uint16 *start_fbuffer = fbuffer;
	Vector tt = in_tt, tti = in_tti;
	Vector t;
	Vector v;
	Vector lw = light.p;
	double A, Arcp/*, pdet, pgB*/;
	char context[128] = {0};
	float opacity, bopacity;
	Uint32 fcol, bfcol;
	int foundsphere;
	FilteringInfo fi;
	raycasting_ctx fsaactx;
	bool antialias = is_adaptive_fsaa();
	ML_Entry *mlbuff;
	int ml_x;
	Object* row_active[MAX_OBJECTS];
	int RA_size = 0;
	int rp = 0;

	if (antialias)
		prepare_fsaa_context(& fsaactx, ti, tti);
	
	xr = xsize_render(xres()); yr = ysize_render(yres());
	fi.camera = cur;
	fi.xinc = ti*0.5;
	fi.yinc = tti*0.5;

	ml_x = (xr-1) / ML_BUFFER_GRAN+1;
	Vector tt_start = tt;

	while ((j = lock++) < yr) {
	//for (j = thread_idx; j < yr; j += cpu.count) {
#ifdef TEX_OPTIMIZE
		mlbuff = ml_buffer + ((j/16) * ml_x);
#endif
		
		tt = tt_start + tti * j;
		fb = &start_fb[j * xr];
		fbuffer = &start_fbuffer[j * xr];
		if (cpu.count > 1) memset(fbuffer, 0, sizeof(short)*xr);
		//
		while (rp < OR_size && (obj_rows[rp].y < j || (obj_rows[rp].y == j && obj_rows[rp].type == OPENING))) {
			if (obj_rows[rp].type == OPENING) {
				row_active[RA_size++] = allobjects[obj_rows[rp].object_id];
			} else {
				Object *o = allobjects[obj_rows[rp].object_id];
				int i = 0;
				while (i < RA_size && row_active[i] != o) i++;
				if (i >= RA_size) {
					printf("What's happening here? Object %d doesn't seem to have a opening!\n",
						obj_rows[rp].object_id);
				} else {
					--RA_size;
					while (i < RA_size) {
						row_active[i] = row_active[i+1];
						i++;
					}
				}
			}
			rp++;
		}
		//
		if (RowMin[j] >= xr || RowMax[j] < 0) { // skip whole row
			RowInfo[j] = 0;
		} else {
			x_start = RowMin[j] < 0 ? 0 : RowMin[j];
			x_end   = RowMax[j] >= xr ? (xr-1) : RowMax[j];
			t = ti; t.scale(x_start);
			t += tt;
			fbuffer += x_start;
			fb += x_start;
			for (i=x_start;i<=x_end;i++,fb++,fbuffer++,t+=ti) if (*fb) {
				dropped = *fb;
				fi.ml = mlbuff + (i/16);
				if (dropped & 0x80000000) {
					//apply Adaptive AntiAliasing
					dropped &= 0x7fffffff;
					bool intersection_found;
					int buckets = 1;
					int foundid;
					fsaa_bucket bucket[FSAA_MAX_SAMPLES*2];
					bucket[0].opacity = 0.0f;
					bucket[0].weight  = 0.0f;
					bucket[0].id = -1; // bucket 0 is for the background
					for (int s = 0; s < fsaactx.count; s++) {
						Vector ray;
						ray.add2(t, fsaactx.sample[s]);
						//
						v.make_vector(ray, cur);
						A = v.lengthSqr();
						Arcp = 1.0 / A;
						fi.through = v;
						intersection_found = false;
						for (int ii = 0; ii < fsaa_info[dropped].size; ii++) {
							foundid = fsaa_info[dropped].ids[ii]-1;
							Object * obj = allobjects[foundid];
							if (obj->fastintersect(v, cur, A, context)) {
								intersection_found = true;
								int k = 0;
								while (k < buckets && bucket[k].id != foundid) k++;
								if (k == buckets) {
									fcol = obj->shade(v, cur, lw, Arcp, opacity, 
										context, 0, fi);
									bucket[k].id = foundid;
									bucket[k].weight = fsaactx.weights[s];
									bucket[k].color[0] = 0xff & (fcol >> 16);
									bucket[k].color[1] = 0xff & (fcol >>  8);
									bucket[k].color[2] = 0xff & (fcol      );
									buckets++;
								} else {
									bucket[k].weight += fsaactx.weights[s];
								}
								break;
							}
						}
						if (!intersection_found) {
							v.norm();
							Object * bestobj = NULL;
							double mdist = 1e99;
							for (int foundid = 0; foundid < RA_size; foundid++) {
								Object * obj = row_active[foundid];
								if (obj->intersect(v, cur, context)) {
									intersection_found = true;
									double x = obj->intersection_dist(context);
									if (x > 0 && x < mdist) {
										mdist = x;
										bestobj = obj;
									}
								}
								
							}
							if (bestobj) {
								int foundid = bestobj->id;
								bestobj->fastintersect(v, cur, 1, context);
								int k = 0;
								while (k < buckets && bucket[k].id != foundid) k++;
								if (k == buckets) {
									fcol = bestobj->shade(v, cur, lw, 1,
										opacity, context, 0, fi);
									bucket[k].weight = fsaactx.weights[s];
									bucket[k].color[0] = 0xff & (fcol >> 16);
									bucket[k].color[1] = 0xff & (fcol >>  8);
									bucket[k].color[2] = 0xff & (fcol      );
									bucket[k].id = foundid;
									buckets++;
								} else {
									bucket[k].weight += fsaactx.weights[s];
								}
							}
						}
						if (!intersection_found) {
							bucket[0].weight += fsaactx.weights[s];
						}
					}
					float finalcol[3] = {0.0f, 0.0f, 0.0f};
					float final_opacity = 1 - bucket[0].weight;
					if (buckets) {
						float fo_rcp = 1.0f / final_opacity;
						for (int ii = 1; ii < buckets; ii++) {
							finalcol[0] += bucket[ii].color[0] * bucket[ii].weight * fo_rcp;
							finalcol[1] += bucket[ii].color[1] * bucket[ii].weight * fo_rcp;
							finalcol[2] += bucket[ii].color[2] * bucket[ii].weight * fo_rcp;
						}
					}
					if (show_aa) {//FIXME
						*fb = 0xff0000;
						*fbuffer = 0xffff;
					} else {
						*fb = (((int)finalcol[0]) << 16) 
						    + (((int)finalcol[1]) << 8) 
						    + (((int)finalcol[2]));
						*fbuffer = (unsigned short) (final_opacity*65535);
					}
				} else {
					backd = dropped >> 16;
					dropped &= 0xffff;
					v.make_vector(t,cur); 
					A = v.lengthSqr();
					Arcp = 1.0/A;
					if (allobjects[dropped-1]->fastintersect(v, cur, A, context)) {
#ifdef FSTAT
						++good;
#endif
						fi.through = t;
						fcol = allobjects[dropped-1]->shade(v, cur, lw, Arcp, 
								opacity, context, 0, fi);
						if (opacity > 0.99)  {
							*fb=fcol;
							*fbuffer = 65535;
						} else {
							if (backd) {
								if (allobjects[backd-1]->fastintersect(v, cur, A, context)) {
									bfcol = allobjects[backd-1]->shade(
										v, cur, lw, Arcp, bopacity, context, 0, fi);
									if (bopacity>0.99) {
										*fb = blend(fcol, bfcol, opacity);
										*fbuffer = 65535;
									} else {
										*fb = blend(fcol, bfcol, opacity);
										opacity = opacity + (bopacity*(1-opacity));
										*fbuffer = (unsigned int) (opacity*65535);
									}
								} else {
									*fb=fcol;
									*fbuffer = (unsigned int) (opacity*65535);
								}
							} else {
								*fb = fcol;
								*fbuffer = (unsigned int) (opacity*65535);
							}
						}
					} else {// too bad - we'll have to iterate all the spheres to find the right one...
#ifdef FSTAT
						worse++;
#endif
						foundsphere = 0;
						double mindist = 1e99;
						Object * bestobj = NULL;
						v.norm();
						for (ii=0;ii< RA_size;ii++) {
							if (row_active[ii]->intersect(v, cur, context))
							{
								foundsphere = 1;
								double x = row_active[ii]->intersection_dist(context);
								if (x > 0 && x < mindist) {
									mindist = x;
									bestobj = row_active[ii];
								}
							}
						}
						if (foundsphere) {
							bestobj->fastintersect(v, cur, 1, context);
							fi.through = t;
							fcol = bestobj->shade(
								v, cur, lw, 1, opacity, context, 0, fi);
							if (opacity > 0.99) {
								*fb=fcol;
								*fbuffer = 65535;
							} else {
								*fb = fcol;
								*fbuffer = (unsigned int) (opacity*65535);
							}
						}

#ifdef FSTAT
						realbad += 1-foundsphere;
#endif
					}
					
				}
				if (*fbuffer) RowInfo[j] = 1;
			}
		}
	}
}


void merge_rows_p5(Uint32 *flr, Uint32 *sph, unsigned short *multi, int count)
{
	int i=0;
	do {
		flr[i] =
			 (((flr[i] & 0xff    ) * (65535-multi[i]) + (sph[i] & 0xff    ) * multi[i]) >> 16) +
			((((flr[i] & 0xff00  ) * (65535-multi[i]) + (sph[i] & 0xff00  ) * multi[i]) >> 16) & 0xff00) +
			 (((flr[i] >> 16     ) * (65535-multi[i]) + (sph[i] >> 16     ) * multi[i]) & 0xff0000) + 0x010101;
		i++;
	} while (--count);
}

void merge_buffers(Uint32 *dest, Uint32 *src, unsigned short *fbuffer)
{
	int y, xr, yr;
	xr = xsize_render(xres()); yr = ysize_render(yres());
	for (y = 0; y < yr; y++, src+=xr, dest+=xr, fbuffer+=xr) if (RowInfo[y]) {
		if (cpu.mmx2)
			merge_rows(dest, src, fbuffer, xr);
			else
			merge_rows_p5(dest, src, fbuffer, xr);
	}
	//memcpy(sph, flr, xsize_render(xres()) * ysize_render(yres()) * 4);
}

/*
	This is the SSE version of the renderer (well, this routine is merely a nasty caller)
	The idea is:

	1. Render the floor very fast, using vectorized SSE code in render_floor; use main frame buffer
	2. Render the spheres using less innermost cycle IFs, and preserving alpha data in a seperate buffer;
	   The color data is saved in `spherebuffer'
	3. If we render shadows, calculate them in seperate shadow buffer and merge them to the main frame buffer
	3. Blend the two main & sphere frame buffers into a single with merge_buffers (using MMX);
	4. For everything else use the shared code for the P5 and SSE versions (bash_preframe,
		preframe_do, postframe_do)

	The good news is, that, floor rendering is up to 5 times faster;
	The bad news are, that sphere rendering is a bit slower, and there is an additional overhead (~2%) for
	framebuffer merge. Also, pixels that are rendered with the floor renderer behind the balls are
	calculated in vain, 'coz they are invisible anyway.
*/
#ifdef ACTUALLYDISPLAY
void render_single_frame_do(SDL_Surface *p, SDL_Overlay *ov)
#else
void render_single_frame_do(void)
#endif
{
	Vector lw, tt, ti, tti;
	Vector t0;
	int xr, yr;
	Uint32 *ptr;

	xr = xsize_render(xres()); yr = ysize_render(yres());

	ptr = framebuffer;


	prof_enter(PROF_BASH_PREFRAME);		bash_preframe(lw, tt, ti, tti);			prof_leave(PROF_BASH_PREFRAME);
	prof_enter(PROF_PREFRAME_DO);		preframe_do( spherebuffer, lw, true);		prof_leave(PROF_PREFRAME_DO);

	if (BackgroundMode == BACKGROUND_MODE_VOXEL)
		voxel_frame_init(tt, ti, tti, ptr);

	
	//save tt's
	t0 = tt;
	
	if (cpu.count==1) {
		InterlockedInt i1 = 0, i2 = 0;
		render_background(ptr, xr, yr, tt, ti, tti, 0, i1);

		tt = t0;

		prof_enter(PROF_RENDER_SPHERES);
		render_spheres_init(fbuffer);
		render_spheres(spherebuffer, fbuffer, tt, ti, tti, 0, i2);
		prof_leave(PROF_RENDER_SPHERES);
		
		if (r_shadows){
			prof_enter(PROF_RENDER_SHADOWS);
			render_shadows_init(ptr, sbuffer, xr, yr, t0, ti, tti);
			render_shadows(ptr, sbuffer, xr, yr, t0, ti, tti, 0);
			prof_leave(PROF_RENDER_SHADOWS);
		}

	} else {
		class MultiThreadedMainRender : public Parallel {
			Vector local_t0, local_ti, local_tti;
			Uint32 *framebuffer, *spherebuffer;
			Uint16 *fbuffer, *sbuffer;
			int xr, yr;
			InterlockedInt for_bg, for_rt;
			public:
				MultiThreadedMainRender(const Vector& tt, 
						const Vector &ti, 
						const Vector &tti, 
						Uint16 *xfbuffer, Uint16 *xsbuffer,
						Uint32 *fb, int xr, int yr, Uint32 *sb) : for_bg(0), for_rt(0)
				{
					local_t0 = tt;
					local_ti = ti;
					local_tti = tti;
					this->xr = xr;
					this->yr = yr;
					spherebuffer = sb;
					framebuffer = fb;
					fbuffer = xfbuffer;
					sbuffer = xsbuffer;
					render_spheres_init(fbuffer);
					if (r_shadows)
						render_shadows_init(framebuffer, sbuffer, xr, yr, local_t0, ti, tti);
				}
				void entry(int thread_idx, int thread_count)
				{
					Vector t0;
					t0 = local_t0;
					render_background(framebuffer, xr, yr, t0, local_ti, local_tti, thread_idx, for_bg);
					t0 = local_t0;
					//render_spheres
					render_spheres(spherebuffer, fbuffer, t0, local_ti, local_tti, thread_idx, for_rt);
					if (r_shadows) {
						render_shadows(framebuffer, sbuffer, xr, yr, local_t0, local_ti, local_tti,
							thread_idx);
					}
				}
		} multithreaded_main_render (tt, ti, tti, fbuffer, sbuffer, ptr, xr, yr, spherebuffer);

		thread_pool.run(&multithreaded_main_render, cpu.count);
	}
	
	if (r_shadows) {
		prof_enter(PROF_RENDER_SHADOWS);
		shadows_merge(xr, yr, ptr, sbuffer);
		prof_leave(PROF_RENDER_SHADOWS);
	}

	prof_enter(PROF_MERGE_BUFFERS);
	merge_buffers(ptr, spherebuffer, fbuffer);
	prof_leave(PROF_MERGE_BUFFERS);
	
#ifdef ACTUALLYDISPLAY
	prof_enter(PROF_POSTFRAME_DO);		postframe_do(p, ov);			prof_leave(PROF_POSTFRAME_DO);
#else
	prof_enter(PROF_POSTFRAME_DO);		postframe_do();				prof_leave(PROF_POSTFRAME_DO);
#endif
}

class BackgroundObject : public Object {
public:
	double get_depth(const Vector &camera) { return 1e99; }
	bool is_visible(const Vector & camera, Vector w[3]) { return true;}
	int calculate_convex(Vector pt[], const Vector& camera) { return 0; }
	void map2screen(Uint32 *framebuffer, int color, int sides, Vector pt[],
		const Vector& camera, Vector w[3], int & min_y, int & max_y) {}

	bool fastintersect(const Vector& ray, const Vector& camera,
		const double& rls, void *IntersectContext) const
	{
		double *dist = (double *)IntersectContext;
		if (BackgroundMode == BACKGROUND_MODE_VOXEL) {
			*dist = voxel_raycast_distance(camera, camera + ray);
			return ((*dist) < 1e6);
		} else {
			dist[1] = fabs(ray.v[1]);
			if (dist[1] < DST_THRESHOLD)
				return false;
			double dp;
			dp             = ray.v[1] < 0.0f ? -daFloor : +daCeiling;
			dp            -= copysign(camera.v[1], ray.v[1]);
			dist[2] = dp/dist[1];
			//dist[0] - dist to obj; dist[1] = ydist; dist[2] = scalefactor
			dist[0] = sqrt(rls) * dist[2];
			return true;
		}
	}

	bool intersect(const Vector& ray, const Vector &camera, void *ic)
	{
		return fastintersect(ray, camera, 1.0, ic);
	}

	double intersection_dist(void *IntersectContext) const
	{
		return *(double*) IntersectContext;
	}

	Uint32 shade(Vector& ray, const Vector& camera, const Vector& ll,
		double rlsrcp, float &opacity, void *IntersectContext,
		int iteration, FilteringInfo& finfo )
	{
		if (BackgroundMode == BACKGROUND_MODE_VOXEL)
			return voxel_raytrace(camera, camera + ray);
		double scalefactor = ((double*)IntersectContext)[2];
		double cx, cy;
		cx = camera.v[0] + ray.v[0]*scalefactor;
		cy = camera.v[2] + ray.v[2]*scalefactor;
		int ysqrd_raytrace = ray.v[1] < 0.0f ? ysqrd_floor : ysqrd_ceil;
		int level = 0;
		Uint32 result = texture_handle_bilinear(cx, cy, level, ysqrd_raytrace);
		if (CVars::shadow_algo != 0) {
			float d = light.shadow_density(Vector(cx, ray.v[1] < 0.0f ? daFloor : daCeiling, cy));
			if (d > 0) {
				int mul = (int) (shadow_intensity * 256 * d);
				result = multiplycolor(result, 65535-mul);
			}
		}
		return result;
	}
	int get_best_miplevel(double x0, double z0, FilteringInfo & fi)
	{
		return TEX_S;
	}

	OBTYPE get_type(bool generic = true) const
	{
		return OB_BACKGROUND;
	}
};

static double intersection_distance(const Vector &camera, const Vector &dir)
{
	double mdist = 1e99, dist;
	double rls = dir.lengthSqr();
	Vector w[3];
	calc_grid_basics(camera, CVars::alpha, CVars::beta, w);
	for (int i = 0; i <= allobjects.size(); i++) {
		char ctx[128];
		if (allobjects[i]->is_visible(camera, w) && allobjects[i]->fastintersect(dir, camera, rls, ctx)) {
			allobjects[i]->intersect(dir, camera, ctx);
			dist = allobjects[i]->intersection_dist(ctx);
			if (dist >= 0 && dist < mdist)
				mdist = dist;
		}
	}
	return mdist;
}

static void render_single_frame_photorealistic(void *p, void *v)
{
	struct MultiThreadedPhotoRenderer : public Parallel {
		int xr, yr;
		Vector lw, tt, ti, tti;
		Vector t0;
		Uint32 *ptr;
		void *sdl_p, *sdl_v;
		InterlockedInt bucket;
		Mutex display_mutex;
		double focal_dist, fpd;
		Vector fp0, fp1, fp_center, fpnorm;
		bool my_exit;
		
		MultiThreadedPhotoRenderer()
		{
			my_exit = false;
		}

		void sample_lens_point(double &x, double &y, QMCIterator *iter)
		{
			double angle, r;
			if (iter) {
				angle = iter->next(0) * 2 * M_PI;
				r = iter->next(1);
			} else {
				angle = drandom() * 2 * M_PI;
				r = drandom();
			}
			r = sqrt(r);
			x = cos(angle) * r;
			y = sin(angle) * r;
		}

		Vector get_fisheye_ray(double x, double y)
		{
			double l = sqrt(2.0) * 0.5;
			double angleofview = (CVars::fov * M_PI_2) * 0.5;
			double zx = tan(angleofview) * l;
			double zy = zx / CVars::aspect_ratio;
			Vector R(
				-l,
				zy - 2*zy*y/yr,
				zx - 2*zx*x/xr);
			double C = (1.0+sqrt(2.0)/2);
			double B = 2*C*R[0];
			double A = R.lengthSqr();
			C = C*C - 1;
			double D = B*B - 4 * A * C;
			if (D < 0) return Vector(0.0, 0.0, 0.0);
			double x1 = (-B - sqrt(D)) / (2 * A);
			Vector n = Vector(1.0 + sqrt(2.0)/2.0, 0, 0) + R * x1;
			R.norm();
			R += n * 2.0;
			R.norm();
			return world_rotate(R, CVars::alpha, CVars::beta);
		}
		
		Uint32 solve_single(int x, int y, FilteringInfo &fi)
		{
			/* Prepare for the gathering phase */
			int r = 0, g = 0, b = 0;
			int n = CVars::dof_samples;
			if (n == 0) n = 1;
			char context[128];
			int total_weight = 0;
			
			/* Select antialiasing kernel */
			const fsaa_kernel *kernel;
			if (n < 10)
				kernel = &aaa_kernel_quincunx;
			else if (n < 16)
				kernel = &aaa_kernel_10;
			else kernel = &aaa_kernel_16;
			
			QMCIterator iter;
			bool useqmc = CVars::qmc;
			if (useqmc)
				iter = qmc.init(n);
			
			/* Prepare adaptive sampling stuff */
			int prev_r=0, prev_g=0, prev_b=0, prev_w=0;
			int expected;
			
			expected = n / 2 + n % 2;
			
			/* Determine what error we do accept */
			float error = 12.0f / pow((float) n, 0.75f);
			
			for (int i = 0; ; i++) {
				/* Check for exit conditions */
				if (i == 10 * n) break;
				if (i == expected) {
					if (i < n) {
						prev_r = r;
						prev_g = g;
						prev_b = b;
						prev_w = total_weight;
						expected = n;
					} else {
						float w1 = 1.0f / prev_w;
						float w2 = 1.0f / total_weight;
						if (fabs(prev_r * w1 - r * w2) <= error &&
						    fabs(prev_g * w1 - g * w2) <= error &&
						    fabs(prev_b * w1 - b * w2) <= error) break;
						prev_r = r;
						prev_g = g;
						prev_b = b;
						prev_w = total_weight;
						expected += n/2;
					}
				}
				
				/* Create ray */
				int ii = i % kernel->count;
				Vector ray;
				if (CVars::fisheye) {
					ray = get_fisheye_ray(x + kernel->coords[ii][0], y + kernel->coords[ii][1]);
					ray.norm();
				} else {
					ray = tt + ti * (x + kernel->coords[ii][0]) + tti * (y + kernel->coords[ii][1]);
					ray -= cur;
					ray.norm();
				}
				int weight = (int) (65536 * kernel->weights[ii]);
				total_weight += weight;
			
				/* Generate cone pinpoint */
				double xd = cur * fpnorm + fpd;
				double xdd = ray * fpnorm;
				if (xdd < 1e-6) continue;
				Vector t = cur + ray * (-xd / xdd);
				
				Vector newcam, nr;
				if (n>1) {
					double xu, xv;
					sample_lens_point(xu, xv, useqmc? &iter : NULL);
					newcam = cur + fp0 * xu + fp1 * xv;
					nr = t - newcam;
					nr.norm();
				} else {
					newcam = cur;
					nr = ray;
				}
				
				/* Setup filtering info */
				if (n > 1) fi.camera = newcam;
				fi.through = newcam+nr;

				
				Object *z = NULL;
				Object *prevz = NULL;
				double mdist = 1e99;
				double prevdist = 1e99;
				
#define TESTD(obj) \
	if (dist > 0.0f && dist < mdist) {\
		prevdist = mdist; \
		mdist = dist; \
		prevz = z; \
		z = obj; \
	} else if (dist > 0.0f && dist < prevdist) { \
		prevdist = dist; \
		prevz = obj; \
	}
				//
				double dist;
				for (int i = 0; i < spherecount; i++) {
					if (sp[i].intersect(nr, newcam, context)) {
						dist = sp[i].intersection_dist(context);
						TESTD(sp+i);
					}
				}
				for (int j = 0; j < mesh_count; j++) {
					if (mesh[j].testintersect(newcam, nr)) {
						if (CVars::g_speedup && mesh[j].sdtree) {
							Triangle *t;
							if (mesh[j].sdtree->testintersect(newcam, nr, context, &t)) {
								dist = t->intersection_dist(context);
								TESTD(t);
							}
						} else {
							for (int i = 0; i < mesh[j].triangle_count; i++) {
								Triangle * t = trio + i + (j << TRI_ID_BITS);
						
								if (!t->okplane(newcam)) {
									i += t->tri_offset;
									continue;
								}
						
								if (t->intersect(nr, newcam, context)) {
									dist = t->intersection_dist(context);
									TESTD(t);
								}
							}
						}
					}
				}
				if (BackgroundMode == BACKGROUND_MODE_VOXEL) {
					Object *z = voxel_water_object();
					if (z->intersect(nr, newcam, context)) {
						dist = z->intersection_dist(context);
						TESTD(z);
					}
				}
				Object *tt = allobjects[allobjects.size()];
				if (tt->intersect(nr, newcam, context)) {
					dist = tt->intersection_dist(context);
					TESTD(tt);
				}
				
				//
				if (!z) continue;
				z->intersect(nr, newcam, context);
				FilteringInfo finfo;
				float f = 1.0f;
				int result = z->shade(nr, newcam, lw, 1.0, f, context, 0, fi);
				if (f != 1.0f) {
					int bgresult = 0;
					float opc;
					if (prevz) {
						prevz->intersect(nr, newcam, context);
						bgresult = prevz->shade(nr, newcam, lw, 1.0, opc, context, 0, fi);
					}
					b += ((int) ((result & 0xff) * f + (bgresult & 0xff) * (1-f))) * weight;
					result >>= 8; bgresult >>= 8;
					g += ((int) ((result & 0xff) * f + (bgresult & 0xff) * (1-f))) * weight;
					result >>= 8; bgresult >>= 8;
					r += ((int) ((result & 0xff) * f + (bgresult & 0xff) * (1-f))) * weight;
				} else {
					b += (result & 0xff) * weight; result >>= 8;
					g += (result & 0xff) * weight; result >>= 8;
					r += (result & 0xff) * weight;
				}
			}
			//
			b /= total_weight;
			g /= total_weight;
			r /= total_weight;
			return (r << 16) | (g << 8) | b;
		}
		
		void entry(int thread_idx, int thread_count)
		{
			/* determine bucket size (bs) */
			int bs = yr / 10;
			bs &= 0xfff0;
			if (!bs) bs = 16;
			
			/* see how many buckets we have */
			int bxr, byr;
			bxr = ((xr - 1) / bs + 1);
			byr = ((yr - 1) / bs + 1);
			int allbucks =  bxr * byr;
			
			/* Setup filtering info */
			FilteringInfo fi;
			fi.camera = cur;
			fi.xinc = ti*0.5;
			fi.yinc = tti*0.5;

			
			/* iterate them */
			int bkt;
			int pmul = prepass_photorealistic ? 2 : 1;
			while ((bkt = bucket++) < allbucks*pmul && !my_exit) {
				bool small = prepass_photorealistic && bkt < allbucks;
				bkt %= allbucks;
				/* See what bucket we have */
				int x0 = (bkt % bxr) * bs;
				int y0 = (bkt / bxr) * bs;
				int w = min(bs, xr - x0);
				int h = min(bs, yr - y0);
				
				/* Render our bucket */
				for (int j = 0; j < h; j++) {
					int y = y0 + j;
					for (int i = 0; i < w; i++) {
						bool sixteen = i % 16 == 0 && j % 16 == 0;
						if (prepass_photorealistic && ((sixteen && !small) || (!sixteen && small))) continue;
						int x = x0 + i;
						framebuffer[xr * y + x] =
							solve_single(x, y, fi);
						if (sixteen) {
							for (int q = 0; q < 16; q++)
								for (int t = 0; t < 16; t++){
									framebuffer[xr * (y+q) + x + t] =
										framebuffer[xr * y + x];
								}
						}
					}
				}
				
				/* Display it onscreen */
#ifdef ACTUALLYDISPLAY
				if (sdl_p) {
					SDL_Surface *p = (SDL_Surface*) sdl_p;
					SDL_Overlay *ov = (SDL_Overlay*) sdl_v;
					//
					display_mutex.enter();
					gfx_update_screen(p, ov, ptr, x0, y0, w, h);
					int she = 0;
					kbd_do(&she);
					display_mutex.leave();
					if (she || !CVars::photomode) my_exit = true;
				}
#endif
			}
		}
		
		void setup_focal_dist(Vector cur, Vector ray)
		{
			ray.norm();
			focal_dist = intersection_distance(cur, ray);
			fp_center = cur + ray * (focal_dist / ray.length());
			fp0 = perpendicular(ray);
			fp1 = ray ^ fp0;
			//
			double pupil_diam = 22.4 / CVars::aperture / CVars::fov;
			//
			fp0.make_length(pupil_diam);
			fp1.make_length(pupil_diam);
			fpnorm = fp0 ^ fp1;
			fpd = - (fpnorm * fp_center);
		}
	} mt;

	mt.xr = xsize_render(xres()); mt.yr = ysize_render(yres());
	mt.ptr = framebuffer;
	mt.sdl_p = p;
	mt.sdl_v = v;
	mt.bucket = 0;
	bash_preframe(mt.lw, mt.tt, mt.ti, mt.tti);
	preframe_do( spherebuffer, mt.lw, false);
	static BackgroundObject bgnd;
	allobjects[allobjects.size()] = &bgnd;
	//
	/* setup focal distance */
	mt.setup_focal_dist(cur, mt.tt + mt.ti*(mt.xr/2.0) + mt.tti*(mt.yr/2.0) - cur);
	
	thread_pool.run(&mt, cpu.count);
}

/****************************************************************
 * This is the master function which chooses who to call to do  *
 * the actual job. Currently, the "CPU world" is divided to     *
 * SSE and non-SSE only.                                        *
 ****************************************************************/
#ifdef ACTUALLYDISPLAY
void render_single_frame(SDL_Surface *p, SDL_Overlay *ov)
{
	if (CVars::photomode) {
		render_single_frame_photorealistic(p, ov);
		prepass_photorealistic = false;
	} else {
		prepass_photorealistic = true;
		if (stereo_type) {
			Vector camera_save = cur;
			double alpha_save = CVars::alpha;
			for (stereo_mode = STEREO_MODE_LEFT; stereo_mode <= STEREO_MODE_FINAL; stereo_mode++) {
				cur = camera_save;
				CVars::alpha = alpha_save;
				camera_moved = true;
				render_single_frame_do(p, ov);
			}
		} else {
			render_single_frame_do(p, ov);
		}
	}
}

#else
void render_single_frame(void)
{
	if (vframe % 25 == 24) {printf("."); fflush(stdout);}
	if (CVars::photomode) {
		prepass_photorealistic = false;
		render_single_frame_photorealistic(NULL, NULL);
	} else {
		render_single_frame_do();
	}
}
#endif

void render_close(void)
{
	tex.close();
	int sz = MAX_TEXTURE_SIZE;
	int i = -1;
	while (sz) {
		T[++i].close();
		sz/=2;
	}
	if (stereo_buffer) {
		free(stereo_buffer);
		stereo_buffer = 0;
	}
}
