/***************************************************************************
 *   Copyright (C) 2004 by Veselin Georgiev                                *
 *   vesko@ViruS                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *-------------------------------------------------------------------------*
 *   Scene management stuff                                                *
 ***************************************************************************/

#include <string.h>

#include <math.h>
#include "MyTypes.h"
#include "antialias.h"
#include "bitmap.h"
#include "vector3.h"
#include "object.h"
#include "sphere.h"
#include "triangle.h"
#include "mesh.h"
#include "cmdline.h"
#include "cpu.h"
#include "fract.h"
#include "cross_vars.h"
#include "gfx.h"
#include "infinite_plane.h"
#include "profile.h"
#include "progress.h"
#include "render.h"
#include "rgb2yuv.h"
#include "saveload.h"
#include "shaders.h"
#include "scene.h"
#include "voxel.h"
#include "vector3.h"

/* ---------------------- EXPORT SECTION ---------------------------------- */
extern Vector cur;
extern int RenderMode;
extern int developer, design;
extern int threads_first;

int SceneType = FRAME_BASED;
static int def_resx = 640, def_resy = 480;
double beta_limit_low, beta_limit_high;
background_rendering_method render_background;

int scene_count = 2;
char scenes[5][256] ={
	"data/benchmark.fsv",
	"data/oldbench.fsv",
};
char dev_scene[256] = "data/benchmark.fsv";
char default_font[64] = "data/font1.bmp";
const char *introtex = "data/intro.bmp";


/* Video data: */
#ifdef ACTUALLYDISPLAY
SDL_Surface *screen;
SDL_Overlay *ovr=NULL;
Uint32 SDL_f;
#endif


// precalculated storage for 1/sqrt(sqrt(x)) lookups :)
int *sqrtsqrt;
RawImg tex;
// texture collection:

RawImg T[12]; // this allows texture sizes up to 4096 ;)
int end_tex;
int system_pp_state;

#ifdef ACTUALLYDISPLAY
SDL_Surface *get_screen(void)
{
	return screen;
}
#endif

void set_default_resolution(int newx, int newy)
{
	def_resx = newx;
	def_resy = newy;
	set_new_videomode(newx, newy);
}


void scene_check(void)
{
	if (option_exists("--voxel"))
		BackgroundMode = BACKGROUND_MODE_VOXEL;
}

/* Returns 0 on success, 1 on failure */
int scene_init(void)
{
	int r;
	switch (BackgroundMode) {
		case BACKGROUND_MODE_FLOOR:
			r = render_init();
			beta_limit_low  = -M_PI/2.0;
			beta_limit_high = +M_PI/2.0;
			render_background = render_infinite_plane;
			break;
		case BACKGROUND_MODE_VOXEL:
			developer = 1;
			beta_limit_low  = -M_PI/2.0;
			beta_limit_high = +M_PI/2.0;
			r = voxel_init();
			if (!r)
				render_background = voxel_single_frame_do;
			break;
		default:
			r = 1;
	}
	return r;
}

void scene_close(void)
{
}

#ifdef ACTUALLYDISPLAY
void scene_render_single_frame(SDL_Surface *p, SDL_Overlay *ov)
{
			render_single_frame(p, ov);
}
#else
void scene_render_single_frame(void)
{
			render_single_frame();
}
#endif

/** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * @class Scene                                                               *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
static int scene_no = -1;
Scene::Scene(const char * filename)
{
	++scene_no;
	strcpy(scenefilename, filename);
	closed = false;
	ffm = true;
}

void Scene::precalc(void)
{
	int i, sz;

	// the integrated multiplycolor + lightformulae assembly code requires SSE & MMX2
	// I don't know of a processor which supports SSE, but does NOT MMX2; nevermind I make MMX2 required here, just in case...
	
	// the assembly bilinear filtering routine requires MMX and MMX extended instruction sets.
	
	// the fast bluring routine requires only MMX
	
	// if we don't have SSE, we will precalculate the distance values. 
	// This will get us close to the calculation performance of using SSE,
	// but the integrated version will still be faster because of its integration.
	if ( !cpu.sse ) {
		if ((sqrtsqrt = (int *) malloc(MAX_DIST*sizeof(int))) == NULL) {
			printf("Cannot get memory for precalculated sqrtsqrt array!\n");
			exit(1);
		}
		// precalculate distance array (double sqrt calculations)
		for (i=0;i<MAX_DIST*PDIVIZOR;i+=PDIVIZOR) {
			sqrtsqrt[i/PDIVIZOR] = (int) (lightformulae_tiny(i+7)*65536.0);
	#ifdef ACTUALLYDISPLAY
			if ((i&(__512-1))==0) 
				intro_progress(screen, prog_dist_precalc_base + prog_dist_precalc * i / (MAX_DIST*PDIVIZOR));
	#endif
		}
		//printf("Last value: %d\n", sqrtsqrt[MAX_DIST-1]);
	} else sqrtsqrt = NULL;
	if (BackgroundMode == BACKGROUND_MODE_FLOOR) {
		// precalculate smaller (and possibly larger) textures
		//gridify_texture(tex/*, 16, 0x33ff77, 0xff1234*/);
		T[0] = tex;
		T[0].render_hack();
		sz = MAX_TEXTURE_SIZE/2;
		i=0;
		while (sz) {
			i++;
			T[i-1].shrink(T[i]);
			sz/=2;
	#ifdef ACTUALLYDISPLAY
			intro_progress(screen, prog_text_precalc_base + prog_text_precalc*i/log2(MAX_TEXTURE_SIZE)); 
	#endif
		}
		end_tex = i;
	#ifdef DEBUG_MIPMAPS
		for (int k = 0; k <= end_tex; k++)
			T[k].fill(mips_colors[k]);
	#endif
	}
	gfx_init();
			
	#ifdef ACTUALLYDISPLAY
	SDL_ShowCursor(0);
	#endif
	camera_moved = 1;
}


void Scene::init(void)
{
	char msg[100];
#ifdef ACTUALLYDISPLAY
	if (!design) {
		sprintf(msg, "Loading [%s]...", scenefilename);
		intro_progress_init(screen, msg);	
	}
#endif
	vframe = 0;
	spherecount = 0;
	mesh_count = 0;
	SceneType = FRAME_BASED;
	if (design) {
		init_fract_array();
		kbd_control = 1;
		cd_frames = 0x7fffffff;
		file_control = false;
	} else {
		int lcr = load_context(scenefilename);
		if (lcr <= 0) {
			printf("Can't open the scene file (`%s')!\n", scenefilename);
			exit(2);
		}
	}
	
	if (developer) {
		kbd_control = true;
		file_control = false;
	} else {
		kbd_control = false;
		file_control = true;
	}
	if (option_exists("--generatecoords")) {
		generate_coords();
		developer = false;
		kbd_control = false;
		file_control = true;
		SceneType = TIME_BASED;
	}
	//SaveContext("oldbench.fsv");
	//SaveCoords("oldbench.coords");
	scene_check();
	if (BackgroundMode == BACKGROUND_MODE_UNDEFINED) {
		printf("Unrecognized scene!\n");
		exit(1);
	}
	if (scene_init()) {
		printf("Unable to start scene, bailing out\n");
		exit(1);
	}
	precalc();
	
#ifdef ACTUALLYDISPLAY
	hwaccel_init();
#endif
	
}

int Scene::run(FPSWatch * watch, OutroCapturer * oc)
{
	/* "Game" Loop initialisation */
	int she=RUN_RUNNING, loop_iteration=0, lft;
	clk = get_ticks();
	if (watch)
		watch->start();
#ifdef SHOWFPS
	fpsclk = clk;
#endif
#ifdef BILINEAR_FILTER
	bilfilter = 1;
#else
	defaultconfig = 0;
#endif
/* Here follows the Game loop :) */
	lft = get_ticks();
	 while (!she && !WantToQuit) {
		 camera_moved = 0;
		 if (ffm) { ffm = false; camera_moved = 1; }
		 delta = (get_ticks()-lft)/1000.0;
		//light_angle += delta/2;
		//lx = (int) (200.0 + sin(light_angle)*200.0);
		//ly = (int) (100.0 + cos(light_angle)*100.0);
		//lz = (int)  (500.0 + cos(light_angle)*250.0);
		lft = get_ticks();
#ifdef ACTUALLYDISPLAY
		if (kbd_control) {
			kbd_do(&she);
		} else {
			kbd_tiny_do(&she);
		}
#endif
		if (file_control) {
			camera_moved = 1;
			if (!load_frame(vframe%(cd_frames?cd_frames:1), bTime(), SceneType))
				she = RUN_OK;
		}
#ifdef RECORD
		record_do(bTime());
#endif
#ifdef ACTUALLYDISPLAY
		scene_render_single_frame(screen, ovr);
#else
		scene_render_single_frame();
#endif
		if (oc) {
			oc->check(scene_no, vframe % (cd_frames != 0 ? cd_frames : 1), bTime());
		}
		if (!developer && SceneType == FRAME_BASED && vframe % cd_frames == 0) {
			if (!loop_mode) {
				she = RUN_OK;
			} else {
				if (!loops_remaining) {
					loops_remaining = -666;
					she = RUN_LOOPS_FINISHED;
				}
			loops_remaining--;
#ifdef MAKE_CHECKSUM
				if (cksum!=get_correct_cksum()) {
					if (loop_iteration>1) {
						printf("Incorrect checksum at iteration %d: It is %X, should be %X!!\n",
		       					loop_iteration, cksum, get_correct_cksum());
						she = RUN_CKSUM_FAILED;
					} else set_correct_cksum(cksum);
				}
				cksum = 0;
#endif
				loop_iteration++;
			}
		}
	}
	clk = get_ticks() - clk;
	if (watch) {
		watch->stop(vframe);
	}
	return she;
}

void Scene::close(void)
{
	if (sqrtsqrt != NULL)
		free(sqrtsqrt);
	int i;
	for (i=0;i<spherecount;i++) {
		if (sp[i].flags & MAPPED) {
			delete sp[i].tex;
		}
	}
	switch (BackgroundMode) {
		case BACKGROUND_MODE_FLOOR:
			render_close();
			break;
		case BACKGROUND_MODE_VOXEL:
			voxel_close();
			break;
	}
	mesh_close();
	triangle_close();
#ifdef ACTUALLYDISPLAY
	if (ovr) {
		SDL_FreeYUVOverlay(ovr);
		ovr = NULL;
	}
#endif	
	closed = true;
}

#ifdef ACTUALLYDISPLAY
void Scene::hwaccel_init(void)
{
	SDL_VideoInfo *vi;
	static char vdname[256];
	vi = (SDL_VideoInfo *) SDL_GetVideoInfo();
	SDL_VideoDriverName(vdname, 256);
#ifdef SHOW_VIDEO_INFO
 printf("Video Info: ");
 printf("-- video driver: [%s]\n", vdname);
 printf("->flags: 0x%X\n", screen->flags);
 printf("->hw_available: %d\n->blit_hw: %d\n->blit_hw_CC: %d\n->blit_hw_A: %d\n->blit_sw: %d\n->blit_sw_CC: %d\n->blit_sw_A: %d\n",
	vi->hw_available, vi->blit_hw, vi->blit_hw_CC, vi->blit_hw_A, vi->blit_sw, vi->blit_sw_CC, vi->blit_sw_A);
#endif
 if (!vi->hw_available)
 	printf("No hardware acceleration for surface-to-screen blits.\n");
 if (option_exists("--no-overlay")) return;
 if (!vi->hw_available || option_exists("--force-overlay")) {
	 printf("Will try to use YUV overlay%s...",(vi->hw_available?"":" to speed things up"));
	 ovr = SDL_CreateYUVOverlay(xres(), yres(), SDL_YUY2_OVERLAY, screen);
	 if (ovr == NULL) {printf("FAILED\nCan't create YUY2 overlay!"); if (option_exists("--force-overlay")) printf(" Won't use it no matter how much you --force it."); printf("\n"); }
	 else {
		 printf("OK\n");
		 if (ovr->hw_overlay || option_exists("--force-overlay")) {
			 if (ovr->hw_overlay)
				 printf("Overlay is hardware accelerated :-)\n");
			 if (!option_exists("--force-overlay"))
				 printf("To disable overlay usage, add --no-overlay to the command line.\n");
			 init_yuv_convert(0);
		 }
		 else {
			 printf("Overlay is not hardware accelerated... falling back to software surface flipping (possibly too slow!)\nHint: Check your X server configuration, try using full hardware acceleration!\n");
			 printf("If you want to force using overlay and you know what you're doing, you can accomplish it by passing\nthe --force-overlay option to the program.\n");
			 SDL_FreeYUVOverlay(ovr);
			 ovr = NULL;
		 }
	 }
 }
 printf("\n");
}
#endif

void Scene::videoinit(void)
{
#ifdef ACTUALLYDISPLAY
	if ( SDL_Init(SDL_INIT_TIMER|SDL_INIT_VIDEO) < 0) {
		printf("Unable to init SDL: %s\n", SDL_GetError());
		exit(1);
	}
	atexit(SDL_Quit);
	//-------------------------
	SDL_f = SDLFLAGS;
	if (option_exists("-w")||option_exists("--window")||option_exists("--windowed")) SDL_f &= ~SDL_FULLSCREEN;
	if (option_exists("-f")||option_exists("--fullscreen")) SDL_f |= SDL_FULLSCREEN;
	screen = SDL_SetVideoMode(def_resx * (parallel?2:1) + 8*parallel, def_resy, 32, SDL_f);
	if (screen == NULL) {
		printf("Unable to set %dx%d resolution...%s!\n", (parallel?def_resx*2+8:def_resx), def_resy, SDL_GetError());
		exit(1);
	}
		
	SDL_WM_SetCaption("Anrieff's Fractal", "");
	if (!font0.load_bmp(default_font)) { printf("Unable to load default font, bailing out\n"); exit(1); }
#endif
	if (option_exists("--xres")) {
		set_new_videomode(option_value_int("--xres"), option_value_int("--xres")/4*3);
		defaultconfig = 0;
	}
#ifdef ACTUALLYDISPLAY
	if (xres()!=def_resx) {
		screen = SDL_SetVideoMode(xres() * (parallel ? 2 : 1) + parallel*8, yres(), 32, SDL_f);
		if (screen == NULL) {
			printf("Unable to set %dx%d resolution...%s!\n", (parallel?xres()*2+8:xres()), yres(), SDL_GetError());
			exit(1);
		}
		mouse_sensitivity *= def_resx / (double) xres();
	}
	
#endif
	set_fsaa_mode(FSAA_STARTING_MODE);
	check_fsaa_param();
}

Scene::~Scene()
{
	if (!closed)
		close();
}

void Scene::videoclose(void)
{
#ifdef ACTUALLYDISPLAY
	SDL_FreeSurface(screen);
#endif
}

int saved_cpuspd = -1;
void Scene::outro(int exit_code, Uint32 * framebuffer, FPSWatch& watch, OutroCapturer * outrocapturer)
{
#ifndef ACTUALLYDISPLAY
	return;
#else	
	if (exit_code != RUN_OK && exit_code != RUN_LOOPS_FINISHED &&
			exit_code != RUN_CKSUM_FAILED) return;
	int she = 0;
#ifdef SHOW_CPU_SPEED
	int cpuspd = cpu.speed();
	saved_cpuspd = cpuspd;
#endif
#ifdef MAKE_CHECKSUM
	Uint32 cksave;
#endif
	if (SDL_f&SDL_FULLSCREEN) {
		SDL_WM_ToggleFullScreen(screen);
	}
#ifdef MAKE_CHECKSUM
	cksave = cksum;
#endif
	memcpy(framebuffer, outrocapturer->get_image(), xres()*yres()*4);
#ifdef MAKE_CHECKSUM
	cksum = cksave;
#endif
	vframe--;
	//shader_edge_glow(framebuffer, framebuffer, xres(), yres());
	shader_outro_effect(framebuffer);
	//shader_gamma_shr(framebuffer, framebuffer, xres(), yres(), 1);
	if (exit_code == 4)
		printxy(screen, framebuffer, font0, 45, 32, 0xeef9ff, 0.75, "Test complete.");
	else
		printxy(screen, framebuffer, font0, 45, 32, 0xeef9ff, 0.75, "Test complete -- %d loops.", option_value_int("--loops"));
	printxy(screen, framebuffer, font0, 45, 52, 0xeef9ff, 0.75, "This machine score:");
	//printxy(screen, framebuffer, font0, 264,52, 0x22ff33, 0.99, "%.2lf FPS.", 1000.0 * (double) vframe / (double) clk);
	int y = 72;
	for (int i = 0; i < watch.size(); i++, y += 20) {
		printxy(screen, framebuffer, font0, 45, y, 0xeef9ff, 0.75, "Scene %d: %.2lf FPS.", i + 1, 
			watch.get_data(i) / watch[i]);
	}	
	y += 10;
	printxy(screen, framebuffer, font0, 54, y, 0xffffff, 0.8, "Total: ");
	printxy(screen, framebuffer, font0, 131, y, 0x22ff33, 0.99, "%.2lf FPS", watch.total_data() / watch.total());
	y += 30;
	//printxy(screen, framebuffer, font0, 45, 72, 0xeef9ff, 0.75, "Rendering time: %.2lf seconds.", clk / 1000.0);
#ifdef SHOW_CPU_SPEED
	printxy(screen, framebuffer, font0, 45, y, 0xeef9ff, 0.66, "CPU Speed:       MHz");
	printxy(screen, framebuffer, font0,166, y, 0xff3f00, 0.95, "%5d", cpuspd);
	y += 20;
#endif
	if (cpu.count > 1) {
		printxy(screen, framebuffer, font0, 45,y, 0xeef9ff, 0.75, "Used %d-threaded rendering.", cpu.count);
		y += 20;
	}
#ifdef MAKE_CHECKSUM
	if (defaultconfig) {
		
		printxy(screen, framebuffer, font0, 45,y, 0xeef9ff, 0.75, "Expected checksum: %X", get_correct_cksum());
		y+= 20;
		printxy(screen, framebuffer, font0, 45,y, 0xeef9ff, 0.75, "  YOUR   checksum: ");
		//y+= 20;
		if (exit_code == 4)
			if (cksum==get_correct_cksum())
				printxy(screen, framebuffer, font0, 254, y, 0x22ff33, 0.75, "OK");
		else
			printxy(screen, framebuffer, font0, 254, y, 0xff4444, 0.99, "INCORRECT - %X", cksum);
		else
			if (exit_code == 8)
				printxy(p, framebuffer, font0, 254, y, 0x22ff33, 0.75, "OK (for all loops).");
		else
			printxy(screen, framebuffer, font0, 254, y, 0xff4444, 0.99, "FAILED after loop #%d",
				option_value_int("--loops")-loops_remaining);
		y += 20;
	}
#endif
	if (defaultconfig) {
	if (	 option_exists("-w")|| option_exists("--window")||option_exists("--windowed") &&
			!option_exists("-f")&&!option_exists("--fullscreen"))
		printxy(screen, framebuffer, font0, 45, y, 0xeef9ff, 0.50, "(Test ran in windowed mode)");
	}
	char VersionString[100];
	strcpy(VersionString, Fract_Version);
	strcat(VersionString, "/");
	strcat(VersionString, Mod_Instruction_Set);
	printxy(screen, framebuffer, font0, xres()-(4+11*strlen(VersionString)), yres()-18, 0xffddcc, 0.750, VersionString);
	surface_lock(screen);
	memcpy(screen->pixels, framebuffer, screen->h*screen->w*4);
	surface_unlock(screen);
	SDL_Flip(screen);
	while (!she) {
		kbd_tiny_do(&she);
		SDL_Delay(50); // don't hog CPU (help da clockers :)
	}
#endif // ifdef ACTUALLYDISPLAY
}

/** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * @class OutroCapturer                                                       *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
OutroCapturer::OutroCapturer(int scene_no, int frame, double time)
{
	image = NULL;
	wait_scene = scene_no;
	wait_frame = frame;
	wait_time = time;
	captured = false;
}

OutroCapturer::~OutroCapturer()
{
	if (image)
		delete image;
}

void OutroCapturer::check(int scene_no, int frame, double time)
{
	if (captured)
		return;
	bool action = false;
	if (SceneType == FRAME_BASED) {
		action = (scene_no == wait_scene && frame >= wait_frame);
	} else {
		action = (scene_no == wait_scene && time >= wait_time);
	}
	if (action) {
		image = new RawImg(xres(), yres(), get_frame_buffer());
		captured = true;
	}
}

Uint32 * OutroCapturer::get_image(void)
{
	if (!image) return NULL;
	return (Uint32*)image->get_data();
}
