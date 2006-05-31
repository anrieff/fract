/***************************************************************************
 *   Copyright (C) 2004 by Veselin Georgiev                                *
 *   vesko@ViruS                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *-------------------------------------------------------------------------*
 * Handles the multithreading stuff                                        *
 ***************************************************************************/
#ifndef __THREADS_H__
#define __THREADS_H__
#include "MyTypes.h"
#include "MyGlobal.h"
#include "cpuid.h"
#include "vector3.h"

#ifdef ACTUALLYDISPLAY
#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>
#else
#include <pthread.h>
#include <semaphore.h>
#endif



#define THREAD_RUN_CMD 		1
#define THREAD_QUIT_CMD 	2
#define MAX_CPU_COUNT 64
#define MAX_BARRIERS 16

// declarations:
void render_floor_sse(Uint32*, int, int, Vector, const Vector&, Vector, int);
void render_floor_p5(Uint32*, int, int, Vector, const Vector&, Vector, int);
void render_spheres(Uint32*, Uint16*, Vector, const Vector&, Vector, int);
void render_spheres_init(Uint16*);
/*#ifdef ACTUALLYDISPLAY
#ifdef __GNUC__
static int __attribute__((noinline)) fract_thread_dummy(void *data);
#else
static int fract_thread_dummy(void *data);
#endif
#else
static void* __attribute__((noinline)) fract_thread_dummy(void *data);
#endif*/
#ifndef RENDER_CPP
extern int cpu_count;
extern Uint32 spherebuffer[];
extern unsigned short fbuffer[];
#endif

extern volatile int trd_counter;
#ifdef ACTUALLYDISPLAY
extern SDL_Thread* r_threads[MAX_CPU_COUNT];
extern SDL_sem* r_sem[MAX_CPU_COUNT];
extern SDL_sem *main_sem, *start_sem;
#else
extern pthread_t r_threads[MAX_CPU_COUNT];
extern sem_t r_sem[MAX_CPU_COUNT];
extern sem_t main_sem, start_sem;
#endif
extern volatile int cmd_thread;
extern Vector thread_tt, thread_ti, thread_tti;
extern Uint32 *thread_ptr;
extern int thread_xres, thread_yres;


/** @class MultiThreaded
 *  @date Aug 10, 2005
 *  @author Veselin Georgiev
 *  @short Provides Multithreading functionality for paralelized rendering
 *
 *  Usage: Create a new Multithreaded object with the thread count needed
 *         and pass a rendering procedure which will do the work you require.
 *         For example of such rendering procedure, consult render.cpp,
 *         it is called `fract_thread'. You should merely copy it and modify
 *         to fit your case.
 * 
 *         To render a particular frame, load whatever parameters you need and
 *         call the thread_main() method. It will wakeup the threads for a single
 *         frame
 *
 *         When you're done, destroy the object.
 *
 *         Note that this class is not very generic, and you should modify 
 *         the thread_main method to use it for something else
 *
*/ 
class MultiThreaded {
	
public:
	void (*thread_proc) (void);
	
	/** initialize the MultiThreaded class.
	*** @param cpukount How many threads to create
	*** @param run_proc Which procedure should the threads execute 
	**/
	MultiThreaded(int cpukount, void (*run_proc)(void));
	
	/// sends the threads a command to finish and free resources
	~MultiThreaded(void);
	
	/** Render a single frame. The function merely sets the rendering context
	*** (vectors, resolution, etc.) and commands the threads to begin render.
	*** Returns immediately after all threads have finished
	*** 
	*** @param t0  the Point of the Up-Left corner of the Vector projection grid
	*** @param ti  Column increment
	*** @param tti Row increment
	*** @param xr  Virtual X resolution (can be larger than physical, i.e. when antialiasing)
	*** @param yr  Virtual Y resolution (can be larger than physical, i.e. when antialiasing)
	*** @param ptr Pointer to the frame buffer
	***/
	void thread_main(Vector& t0, Vector& ti, Vector& tti, int xr, int yr, Uint32 *ptr);
};

/**
 * @class Barrier
 * @author Veselin Georgiev
 * @date 2005-10-14
 * --
 * @brief Implements a simple multithreaded barrier
 *
 * Barriers are used when you want to ensure that a given section of code has been
 * executed by all threads and continue execution only after that.
 * Points at code where threads wait for completion of the other threads are called
 * Barrier Points.
 *
 * Example usage:
 * @Example
 * @code
 *
 * void foo(int thread_idx, int threads_count) {
 *	
 *	...
 *	// CODE 1
 *	...
 *	Barrier b;
 *	b.checkin(BARRIER_ID_FOO, thread_idx, threads_count);
 *	...
 *	// CODE 2
 *	...
 *	b.checkout(BARRIER_ID_FOO, thread_idx, threads_count);
 *	...
 *	// CODE 3
 *	...
 * }
 * @endcode
 *
 * CODE 3 will be executed only after all threads have had executed CODE2 (and therefore, CODE1)
 * Barrier ids are simple integers that are uniquie for each barrier used in your code.
 * It is recommended to use the BARRIER_ID_ENUM after the class
*/ 
class Barrier {
	int id;
	int idx, count;
public:
	Barrier();
	void checkin(int barrier_id, int thread_idx, int threads_count);
	void checkout();
};

enum BARRIER_ID_ENUM {
	VOXEL_BARRIER1 = 0,
	VOXEL_BARRIER2,
	VOXEL_LIGHT_OUTER,
	VOXEL_LIGHT_INNER1,
	VOXEL_LIGHT_INNER2
};

/**
 * multithreaded_memset - peforms setting a large area of memory on multiple CPUs. 
 * @param data - the starting address of the memory area
 * @param fill_pattern - the 4byte pattern to fill the memory with
 * @param size - the amount of memory (in 4-byte words) to fill, i.e. the number of bytes / 4
 * @param thread_index - the zero based thread index
 * @param threads_count - the number of CPUs currently exploiting
 *
 * @example
 * Use it in multithreaded code in the following fashion (assuming you have thread_index and threads_count)
 * Substitute:
 * @code
 * memset(a, 0x3f, xres * yres * 4)
 * @endcode
 * With:
 * @code
 * multithreaded_memset(a, 0x3f3f3f3f, xres * yres, thread_index, threads_count)
 * @endcode
 *
 * @note this function doesn't guarantee that the memory fill will be completed when it returns, since only
 *       "yours" call has returned, so you have to keep the sync with other threads yourself
*/ 
void multithreaded_memset(void *data, unsigned fill_pattern, size_t size, int thread_index, int threads_count);

/**
 * atomic_add
 * Adds `val' to the integer, pointed by `addr', in an interlocked operation.
 * Returns the value of `addr' before the addition
*/  
int atomic_add(volatile int *addr, int val);

#endif // __THREADS_H__
