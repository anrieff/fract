#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "MyGlobal.h"
#include "MyTypes.h"
#include "antialias.h"
#include "bitmap.h"
#include "blur.h"
#include "cmdline.h"
#include "common.h"
#include "credits.h"
#include "cpu.h"
#include "cvar.h"
#include "cvars.h"
#include "gfx.h"
#include "mesh.h"
#include "konsole.h"
#include "profile.h"
#include "progress.h"
#include "render.h"
#include "resultoutput.h"
#include "rgb2yuv.h"
#include "scene.h"
#include "sphere.h"
#include "shaders.h"
#include "shadows.h"
#include "saveload.h"
#include "triangle.h"
#include "thorus.h"
#include "tracer.h"
#include "vectormath.h"
#include "voxel.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

int BackgroundMode = BACKGROUND_MODE_FLOOR;

// the default font

Font font0, minifont;


// the framebuffer:

SSE_ALIGN(Uint32 framebuffer[RES_MAXX*RES_MAXY]);

#ifdef DEBUG_MIPMAPS
unsigned mips_colors[] = {
/*      0 -gray   1-blue    2-red     3-green   4-purple  5-yellow  6-cyan    7-white   8-blueish 9-orange */
	0x777777, 0x3333cc, 0xcc3333, 0x11ff11, 0xee22ee, 0xffff00, 0x00ffff, 0xffffff, 0x0077ff, 0xffcc22
};
#endif

// sphere collection

SSE_ALIGN(Sphere sp[MAX_SPHERES]);
int num_sides[MAX_OBJECTS];
int spherecount;

//FIXME
bool recordmode = false;
bool savevideo = false;

int vframe=0;
bool show_aa =  false;

Uint32 clk;
double daFloor = 0.0, daCeiling = 200.0;
double gX, gY;
Vector w[3], cur(30, 45, 50);
double pdlo[MAX_SPHERES];
double lft, delta;
double speed = 25;
double anglespeed = 0.2;
double mouse_sensitivity = 0.16;
double keyboard_sensitivity = 3.0;

int not_fps_written_yet = 1;
int runmode = 0;
int camera_moved;

int loop_mode = 0;
int loops_remaining=9999;
int developer = 0;
int design = 0;

double light_angle = 0;

int defaultconfig = 1;

bool WantToQuit = false;

OutroCapturer *outrocap = NULL;
bool official = false;

int user_pp_state = 0;

extern int r_shadows;
extern double play_time;
extern int cd_frames;
extern double Area_const;
extern int do_mipmap;
int kbd_control = 1;
int file_control = 0;
int use_shader = 0;
#ifdef MAKE_CHECKSUM
Uint32 cksum=0;
#endif
#ifdef DUMPPREPASS
bool wantdump;
#endif

#ifdef SHOWFPS
Uint32 fpsclk;
int fpsfrm=0;
#endif




Uint32 get_ticks(void)
{
#ifdef ACTUALLYDISPLAY
	return SDL_GetTicks();
#else
	static unsigned int start_val;
	static int start_val_inited = 0;
	unsigned int temp;
#ifdef _WIN32
	temp = GetTickCount();
#else
	struct timeval Time; 
	gettimeofday(&Time, NULL);
	temp = Time.tv_sec * 1000 + Time.tv_usec / 1000;
#endif // _WIN32
	if (!start_val_inited) {
		start_val_inited = 1;
		start_val = temp;
	}
	return temp - start_val;
#endif // ACTUALLYDISPLAY
}


double bTime(void)
{
	return ((get_ticks() - clk) / 1000.0);
}


int	   ysqrd, ysqrd_floor, ysqrd_ceil;

// this one calculates the light amount received at the given point. Used only when no SSE is available
int lform( int x, int y, int lx, int ly, int ysq)
{int z=(((int) (sqr(x-lx)+sqr(y-ly)+ysq))>>DIVIZOR_SHIFT);
 if ((unsigned) z<MAX_DIST)
 	{return sqrtsqrt[z];}
	else
	{return (int) (65536.0*lightformulae_tiny(z<<DIVIZOR_SHIFT));}
}

void init_fract_array(void)
{int i;
 double btm;
 if (BackgroundMode == BACKGROUND_MODE_FLOOR) {
 	create_fract_array(sp, &spherecount);
	for (int i = 0; i < spherecount; i++)
		sp[i].tag = OB_SPHERE + i;
 	create_triangle_array();
 } else {
	create_voxel_sphere_array(sp, &spherecount);
 }

 btm = bTime();
 for (i=0;i<spherecount;i++) {
 	sp[i].time = btm;
	}
}

void init_program(void)
{
	estimate_progress();
	common_init();
	thor_init();
	config.init();
	cvars_init();
}

void close_program(void)
{
#ifdef DEBUG
	progressman.taskstats();
#endif
	Scene::videoclose();
	gfx_close();
	bitmap_close();
	blur_close();
	cmdline_close();
	rgb2yuv_close();
	sphere_close();
	vectormath_close();
	saveload_close();
	shaders_close();
	shadows_close();
	scene_close();
	triangle_close();
	antialias_close();
	mesh_close();
	if (outrocap)
		delete outrocap;
	outrocap = NULL;
	config.finish();
}


void kbd_tiny_do(int *ShouldExit)
{
#ifdef ACTUALLYDISPLAY	
	SDL_Event e;
	if (WantToQuit) *ShouldExit = 1;
	while ( SDL_PollEvent(&e)) { // handle keyboard & mouse
		if ( e.type == SDL_QUIT ) *ShouldExit = RUN_CANCELLED;
		if ( e.type == SDL_KEYDOWN ) {
			if (e.key.keysym.sym == SDLK_ESCAPE) *ShouldExit = RUN_CANCELLED;
			if (e.key.keysym.sym == SDLK_F12) { // F12 is screenshot shortcut
				take_snapshot();
			}
			if (developer) {
				if (e.key.keysym.sym == SDLK_F11) { // F11 is bilinear filtering toggle shortcut
					CVars::bilinear = !CVars::bilinear;
				}

				if (e.key.keysym.sym == SDLK_F8) {
					switch (fsaa_mode) {
						case FSAA_MODE_NONE:     set_fsaa_mode (FSAA_MODE_4X_LO_Q); break;
						case FSAA_MODE_4X_LO_Q:  set_fsaa_mode (FSAA_MODE_4X_HI_Q); break;
						case FSAA_MODE_4X_HI_Q:  set_fsaa_mode (FSAA_MODE_ADAPT_4); break;
						case FSAA_MODE_ADAPT_4:  set_fsaa_mode (FSAA_MODE_ADAPT_16); break;
						case FSAA_MODE_ADAPT_16: set_fsaa_mode (FSAA_MODE_NONE); break;
					}
				}
			}
		}
	}
#endif
}


void move_sphere(char c)
{
	static double mmm = 1.0;
	switch (c) {
		case 'i':
		case 'j': sp[spherecount-1].pos += Vector((c=='i')*2-1.0, 0,0)*mmm;
		break;
		case 'o':
		case 'k': sp[spherecount-1].pos += Vector(0, (c=='o')*2-1.0,0)*mmm;
		break;
		case 'p':
		case 'l': sp[spherecount-1].pos += Vector(0,0,(c=='p')*2-1.0)*mmm;
		break;
		case 'u': sp[spherecount-1].d += 1.0;
		break;
		case 'h': sp[spherecount-1].d -= 1.0;
		break;
		case 'w': mmm = 10.0/mmm;
		break;
		case 'a': {
			sp[spherecount] = sp[spherecount-1];
			sp[spherecount].pos.v[1] = sp[spherecount-1].pos.v[1] + 1 + sp[spherecount-1].d*2;
			spherecount++;
			break;
		}
		case 'q': {
			for (int i = 0; i < spherecount; i++) {
				printf("Sphere %i: ", i);
				sp[i].pos.print();
				printf(" D = %.4lf\n", sp[i].d);
			}
		}
	}
}

void kbd_do(int *ShouldExit)
{
#ifdef ACTUALLYDISPLAY
	static bool mouse_captured = false;
	if (konsole.wants_exit())
		*ShouldExit = RUN_CANCELLED;
	SDL_Event e;
	Uint8 *keystate;
	int deltax, deltay;
	keystate = SDL_GetKeyState(NULL);
	if (recordmode) {
		take_snapshot();
	}
	if (WantToQuit) *ShouldExit = 1;
	if (CVars::shut_down && CVars::photomode && frame_finished) {
		take_snapshot();
		*ShouldExit = RUN_CANCELLED;
	}
	while ( SDL_PollEvent(&e)) { // handle keyboard & mouse
		if ( e.type == SDL_QUIT ) *ShouldExit = RUN_CANCELLED;
		if ( e.type == SDL_KEYDOWN ) {
			if (e.key.keysym.sym == SDLK_ESCAPE) {
				if (!(SDL_f & SDL_FULLSCREEN) && mouse_captured) {
					mouse_captured = false;
					SDL_WM_GrabInput(SDL_GRAB_OFF);
				} else 
					*ShouldExit = RUN_CANCELLED;

			}
			if (e.key.keysym.sym == SDLK_F12) { // F12 is screenshot shortcut
				take_snapshot();
			}
			if (developer) {
				if (e.key.keysym.sym == SDLK_BACKQUOTE) {
					konsole.toggle();
					continue;
				}
				bool shift = keystate[SDLK_LSHIFT] || keystate[SDLK_RSHIFT];
				if (konsole.visible()) {
					if (konsole.handle_keycode(e.key.keysym.sym, shift)) {
						if (konsole.wants_exit())
							*ShouldExit = RUN_CANCELLED;
						continue;
					}
				} else {
					if (konsole.key_bound(e.key.keysym.sym, shift))
						continue;
				}
				if (e.key.keysym.sym == SDLK_F11) { // F11 is bilinear filtering toggle shortcut
					CVars::bilinear = !CVars::bilinear;
				}
				if (e.key.keysym.sym == SDLK_F10) {
					CVars::photomode = !CVars::photomode;
				}
//HACK FIXME HAX0RED

				if (e.key.keysym.sym == SDLK_F1) {
					++voxel_rendering_method;
					voxel_rendering_method%=2;
				}
				//if (e.key.keysym.sym == SDLK_F2) recordmode = true;
				if (e.key.keysym.sym == SDLK_F3) {
					++shadow_casting_method;
					shadow_casting_method%=2;
					shadow_casting_method |= 4;
				}
				if (e.key.keysym.sym == SDLK_F4) {
					light_moving = !light_moving;
				}
				if (e.key.keysym.sym == SDLK_RETURN) {
					voxel_shoot();
				}
// END OF HAX0RED
				if (e.key.keysym.sym == SDLK_F8) {
					switch (fsaa_mode) {
						case FSAA_MODE_NONE:     set_fsaa_mode (FSAA_MODE_4X_LO_Q); break;
						case FSAA_MODE_4X_LO_Q:  set_fsaa_mode (FSAA_MODE_4X_HI_Q); break;
						case FSAA_MODE_4X_HI_Q:  set_fsaa_mode (FSAA_MODE_ADAPT_4); break;
						case FSAA_MODE_ADAPT_4:  set_fsaa_mode (FSAA_MODE_ADAPT_QUINCUNX); break;
						case FSAA_MODE_ADAPT_QUINCUNX:  set_fsaa_mode (FSAA_MODE_ADAPT_10); break;
						case FSAA_MODE_ADAPT_10:  set_fsaa_mode (FSAA_MODE_ADAPT_16); break;
						case FSAA_MODE_ADAPT_16: set_fsaa_mode (FSAA_MODE_NONE); break;
					}
				}
				if (e.key.keysym.sym == SDLK_F9) {
					show_aa = !show_aa;
				}

			}
			if (konsole.wants_exit()) *ShouldExit = RUN_CANCELLED;
			if (e.key.keysym.sym == SDLK_F5) use_shader = !use_shader;
			if (e.key.keysym.sym == SDLK_r)	{
				runmode = !runmode;
				if (runmode) speed*=10.0;
					else speed/=10.0;
			}

			//FIXME
			if (e.key.keysym.sym == SDLK_i) {
				CVars::anisotrophic = !CVars::anisotrophic;
			}
						
			if (e.key.keysym.sym == SDLK_x) {
				g_scpuabi = !g_scpuabi;
			}
			
			if (e.key.keysym.sym == SDLK_q) {
				CVars::qmc = !CVars::qmc;
			}

			if (e.key.keysym.sym == SDLK_t) {
				g_view_tracers = !g_view_tracers;
			}
			
			// /FIXME
			
			if (e.key.keysym.sym == SDLK_g) switch_gravity();
			if (e.key.keysym.sym == SDLK_m) do_mipmap = !do_mipmap;
			if (e.key.keysym.sym == SDLK_c) switch_rgbmethod();
			if (e.key.keysym.sym == SDLK_s) r_shadows = !r_shadows;
			if (e.key.keysym.sym == SDLK_t) mesh_count = 1- mesh_count;
			if (e.key.keysym.sym == SDLK_d) wantdump = !wantdump;
#ifdef BLUR
			if (e.key.keysym.sym == SDLK_b) {
				if (keystate[SDLK_LSHIFT] || keystate[SDLK_RSHIFT]) {
					cycle_blur_mode();
				} else {
					if (!apply_blur)
						blur_reinit();
					apply_blur = !apply_blur;
				}
			}
#endif
		}
		bool mouse_just_captured = false;
		if (e.type == SDL_MOUSEBUTTONDOWN && !(SDL_f & SDL_FULLSCREEN) && !mouse_captured) {
			mouse_captured = true;
			mouse_just_captured = true;
			SDL_WM_GrabInput(SDL_GRAB_ON);
		}
		if (CVars::crosshair && e.type == SDL_MOUSEBUTTONDOWN && !mouse_just_captured) {
			if (e.button.button == 1 || e.button.button == 3) {
				process_shot(cur, e.button.button == 1 ? 1 : 5);
			}
		}
	}

	if (!konsole.visible()) {
		if (keystate[SDLK_UP	])	move_camera(	0	);
		if (keystate[SDLK_DOWN	])	move_camera(	M_PI	);
		if (keystate[SDLK_LEFT	])	move_camera(-	M_PI / 2);
		if (keystate[SDLK_RIGHT	])	move_camera(	M_PI / 2);
		if (keystate[SDLK_KP2	])	rotate_camera(	0,	-1*keyboard_sensitivity);
		if (keystate[SDLK_KP4	])	rotate_camera( -1*keyboard_sensitivity,	 0);
		if (keystate[SDLK_KP6	])	rotate_camera( +1*keyboard_sensitivity,	 0);
		if (keystate[SDLK_KP8	])	rotate_camera(	0,	+1*keyboard_sensitivity);
	}
	
	SDL_GetRelativeMouseState(&deltax, &deltay);
	rotate_camera(CVars::mousespeed * mouse_sensitivity * deltax, CVars::mousespeed * -mouse_sensitivity * deltay);
	check_params();
#endif
}

void commandline_parse(void)
{
	progressman.reset();
	cpu.init();
	if (option_exists("--help") || option_exists("-h")) {
		display_usage();
		exit(0);
	}
	if (option_exists("--benchmark")) {
		init_yuv_convert(1);
		exit(0);
	}
	if (option_exists("--query")) {
		if (0 == strcmp(option_value_string("--query"), "integrity")) {
			exit(query_integrity());
		}
		printf("Warning: Unknown query: %s\n", option_value_string("--query"));
	}
	if (option_exists("--parallel") || option_exists("--para") || 
		option_exists("--anaglyph") || option_exists("--cross")) {
		defaultconfig = 0;
		stereo_type = option_exists("--anaglyph") ? STEREO_TYPE_ANAGLYPH : STEREO_TYPE_PARALLEL;
		if (stereo_type == STEREO_TYPE_PARALLEL) {
			set_default_resolution(160, 120);
			option_add("-w");
		}
		if (option_exists("--stereo-separation"))
			CVars::stereo_sep = option_value_float("--stereo-separation");
		if (option_exists("--stereo-depth"))
			stereo_depth = option_value_float("--stereo-depth");
		stereo_crossed = false;
		if (option_exists("--cross")) {
			stereo_crossed = true;
		}
	}
	if (option_exists("--developer")) {
		developer = 1;
		scene_count = 1;
		strcpy(scenes[0], dev_scene);
	}
	if (option_exists("--shadows")) {r_shadows = 1; }
	if (option_exists("--no-shadows")) {r_shadows = 0; defaultconfig = 0;}
	if (option_exists("--nothread")||option_exists("--nothreads")||
	option_exists("--no-thread")||option_exists("--no-threads")) {
		cpu.count = 1;
	}
	if (option_exists("--cpus")) set_cpus(option_value_int("--cpus"));
	if (option_exists("--design") || option_exists("--architect")) {
		design = 1;
		developer = 1;
		scene_count = 1;
		scene_check();
	}
	if (option_exists("--scene")) {
		scene_count = 1;
		strcpy(scenes[0], option_value_string("--scene"));
		defaultconfig = 0;
		outrocap = new OutroCapturer(0, 0, 0);
	}
	if (option_exists("--shader")) {
		shader_cmdline_option(option_value_string("--shader"));
		use_shader = 1;
	}
	if (option_exists("--save-video")) {
		savevideo = true;
		system("mkdir video");
		system("rm video/*.bmp");
	}
	if (defaultconfig && !developer && !design) {
		outrocap = new OutroCapturer(0, 0, 0);
		if (option_exists("--official")) {
			official = true;
		}
	}

}


int main(int argc, char *argv[])
{
	int run_result = RUN_OK;
	FPSWatch stopwatch;
	initcmdline(argc, argv);
	commandline_parse();
	init_program();
	if (option_exists("--credits")) { Scene::videoinit(); show_credits(); close_program(); return 0; }
	for (int i = 0; i < scene_count && run_result == RUN_OK; i++) {
		Scene scene(scenes[i]);
		if (i == 0) Scene::videoinit();
		scene.init();
		run_result = scene.run(&stopwatch, outrocap);
		scene.close();
	}
	if (outrocap) Scene::outro(run_result, get_frame_buffer(), stopwatch, outrocap);
	if (official && run_result == RUN_OK) generate_result_file(stopwatch);
	
	printf("%d frames drawn for %d milliseconds; this yields %.2lf FPS.\n",
	       stopwatch.total_data(), (int) (1000 * stopwatch.total()),
	       stopwatch.total_data() / stopwatch.total());
	
	if (developer || option_exists("--prof-stats")) prof_statistics();
	if (!strcmp(config.credits_shown, "no")) {
		show_credits();
		strcpy(config.credits_shown, "yes");
	}
	close_program();
	return 0;
}
