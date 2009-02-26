/***************************************************************************
 *   Copyright (C) 2004 by Veselin Georgiev                                *
 *   vesko@ViruS                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *-------------------------------------------------------------------------*
 *   Scene management stuff
 ***************************************************************************/

#ifndef __SCENE_H__
#define __SCENE_H__


#include "MyGlobal.h"
#include "MyTypes.h"
#include "vector3.h"
#include "bitmap.h"
#include "fract.h"
#include "threads.h"
// The various Render modes

#define BACKGROUND_MODE_FLOOR		0
#define BACKGROUND_MODE_VOXEL		1
#define BACKGROUND_MODE_UNDEFINED	999

#define RUN_RUNNING		0
#define RUN_CANCELLED		1
#define RUN_OK			4
#define RUN_LOOPS_FINISHED	8
#define RUN_CKSUM_FAILED	16

#define FRAME_BASED		0
#define TIME_BASED		1

extern int SceneType;
extern int BackgroundMode;
extern int scene_count;
extern char scenes[5][256];
extern char dev_scene[256];
extern int *sqrtsqrt;
extern RawImg tex;
extern RawImg T[12]; // this allows texture sizes up to 4096 ;)
extern int end_tex;
extern Uint32 SDL_f;
extern int system_pp_state;
extern int overlay_size_divisor; // a quick hack, until I make ConvertRGB2YV12 planar routines.

/*
 the common type of procedure that can render floor, heightfield or anything
 that can act as a 'background'. Spheres can be applied on top of it.
*/
typedef void (*background_rendering_method)(Uint32*, int, int, Vector&, Vector&, Vector&, int, InterlockedInt&);

extern background_rendering_method render_background;

void scene_check(void);
int scene_init(void);
void scene_close(void);
void set_default_resolution(int, int);

// Log the current frame rendering time to some small static array
void log_frame_time(double frameTime);

// get the recorded frame times from lgo_frame_time.
// Write results in times[], which is arr_size entries long.
// returns the number of records written.
int get_frame_log_times(double times[], int arr_size);

#ifdef ACTUALLYDISPLAY
void scene_render_single_frame(SDL_Surface *p, SDL_Overlay *ov);
SDL_Surface *get_screen(void);
#else
void scene_render_single_frame(void);
#endif

/**
 * @class StopWatch
 * @author Veselin Georgiev
 * @date 2005-12-25
 * @brief Timer used to measure benchmark times
 * 
 * Create a new StopWatch. Prior to running some calculation, call start().
 * When done, call stop()
 * To get the execution time, call get_last_time()
 * To get the number of "laps" (start-stop pairs), call size()
 * To get the time for a particular "lap" use the [] operator
 * Get the accumulate time with total()
 * The class is templatized, so you can store additional data
 * along with the lap times
 * Get the accumulate of the data with total_data()
 */
template <class datatype>
class StopWatch {
	datatype data[12];
	double times[12];
	double temp;
	bool is_running;
	int _size;
	public:
		StopWatch()
		{
			_size = 0;
			is_running = false;
		}
		void start(void)
		{
			is_running = true;
			times[_size] = get_ticks()/1000.0;
		}
		void stop(void)
		{
			times[_size] = get_ticks()/1000.0 - times[_size];
			is_running = false;
			_size++;
		}
		void stop(const datatype& t)
		{
			times[_size] = get_ticks()/1000.0 - times[_size];
			data[_size] = t;
			is_running = false;
			_size++;
		}
		double get_last_time() const
		{
			return times[_size-1];
		}
		int size() const
		{
			return _size;
		}
		double operator [] (const int idx) const
		{
			return times[idx];
		}
		datatype get_data(const int idx) const
		{
			return data[idx];
		}
		bool running()const
		{
			return is_running;
		}
		//
		double total() const
		{
			double sum = 0.0;
			
			for (int i = 0; i < _size; i++) {
				sum += times[i];
			}
			return sum;
		}		
		datatype total_data() const
		{
			datatype sum = 0;
			for (int i = 0; i < _size; i++)
				sum += data[i];
			return sum;
		}
};
typedef StopWatch<int> FPSWatch;
class OutroCapturer;

/**
 * @class Scene
 * @author Veselin Georgiev
 * @date 2005-12-25
 * @brief Scene management class.
 * 
 * This class is used to load scenes, precalculate associated resources, render sequences
 * and then free the allocated resources. Typical usage:
 *
 * @code
 * Scene scene("MyScene.fsv");
 * scene.init();
 * scene.run();
 * scene.close();
 * @endcode
 *
 * optionally, you can pass a pointer to a FPSWatch to run() which will cause the time to
 * be measured
*/
class Scene {
	char scenefilename[256];
	bool kbd_control, file_control;
	bool closed;
	void precalc(void);
	void hwaccel_init(void);
	bool ffm;
public:
	Scene(const char * filename);
	void init(void);
	int run(FPSWatch * = NULL, OutroCapturer * = NULL);
	void close(void);
	static void videoinit(void);
	static void videoclose(void);
	static void outro(int exit_code, Uint32 * framebuffer, FPSWatch&, OutroCapturer *);
	~Scene();
};

class OutroCapturer {
	RawImg *image;
	bool captured;
	int wait_scene, wait_frame;
	double wait_time;
public:
	OutroCapturer(int scene_no, int frame, double time);
	~OutroCapturer();
	void check(int scene_no, int frame, double time);
	Uint32 * get_image(void);
};

#endif // __SCENE_H__
