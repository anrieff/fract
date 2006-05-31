/***************************************************************************
 *   Copyright (C) 2005 by Veselin Georgiev                                *
 *   vesko@workhorse                                                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include "MyGlobal.h"
#include "cpuid.h"
#include "scene.h"
#include "threads.h"


MultiThreaded *temp;
volatile int trd_counter;
#ifdef ACTUALLYDISPLAY
SDL_Thread* r_threads[MAX_CPU_COUNT];
SDL_sem* r_sem[MAX_CPU_COUNT];
SDL_sem *main_sem, *start_sem;
#else
pthread_t r_threads[MAX_CPU_COUNT];
sem_t r_sem[MAX_CPU_COUNT];
sem_t main_sem, start_sem;
#endif
volatile int cmd_thread;
Vector thread_tt, thread_ti, thread_tti;
Uint32 *thread_ptr;
int thread_xres, thread_yres;
int threads_first;




void MultiThreaded:: thread_main(Vector& t0, Vector& ti, Vector& tti, int xr, int yr, Uint32 *ptr)
{
	int i;
	thread_tt = t0;
	thread_ti = ti;
	thread_tti = tti;
	thread_xres = xr;
	thread_yres = yr;
	thread_ptr = ptr;
	if (BackgroundMode == BACKGROUND_MODE_FLOOR)
		render_spheres_init(fbuffer); // clear the buffer before any thread has begun touching it ...

	// run the threads
	for (i=0;i<cpu_count;i++) {

		if (threads_first) {
#ifdef ACTUALLYDISPLAY
			SDL_SemPost(start_sem);
#else
			sem_post(&start_sem);
#endif
		}else {
#ifdef ACTUALLYDISPLAY
			SDL_SemPost(r_sem[i]);
#else
			sem_post(r_sem+i);
#endif
		}
	}

	/* right now the threads are running like idiots :)
	*/
	threads_first = 0;
	for (i=0;i<cpu_count;i++) {
#ifdef ACTUALLYDISPLAY
		SDL_SemWait(main_sem);
#else
		sem_wait(&main_sem);
#endif
	}
	// Once all threads are ready they all become blocked in the end of their while(...) { ...} cycle
}

#ifdef ACTUALLYDISPLAY
#ifdef __GNUC__
static int __attribute__((noinline)) fract_thread_dummy(void *data)
#else
static int fract_thread_dummy(void *data)
#endif
#else
static void* __attribute__((noinline)) fract_thread_dummy(void *data)
#endif
{
//	register int thread_id_ = *((int*)data);
#if defined __GNUC__ && defined USE_ASSEMBLY
	__asm __volatile ("andl $-16,%%esp" ::: "%esp");
#endif
	temp->thread_proc();
	return 0;
}

MultiThreaded:: MultiThreaded(int cpukount, void (*run_proc)(void))
{
	int i, res;
	int tstorage[MAX_CPU_COUNT];
	temp = this;
	thread_proc = run_proc;
	cpu_count = cpukount;
	trd_counter = 0;
	threads_first = 1;
	for (i=0;i<cpu_count;i++) {
#ifdef ACTUALLYDISPLAY
		r_sem[i] = SDL_CreateSemaphore(0); // create n LOCKED semaphores
		res = (r_sem[i] == NULL);
#else
		res = sem_init(r_sem + i, 0, 0); // create n LOCKED semaphores
#endif
		if (res) printf("error creating %d-th semaphore\n", i);
	}
#ifdef ACTUALLYDISPLAY
	main_sem = SDL_CreateSemaphore(0); // create a semaphore for the main thread
	res = (main_sem == NULL);
#else
	res = sem_init(&main_sem, 0, 0);  // create a semaphore for the main thread
#endif
	if (res) printf("Unable to create the main semaphore\n");
#ifdef ACTUALLYDISPLAY
	start_sem = SDL_CreateSemaphore(0);  // create a semaphore for the rendering union
	res = (start_sem == NULL);
#else
	res = sem_init(&start_sem, 0, 0);    // create a semaphore for the rendering union
#endif
	if (res) printf("Unable to create the start semaphore\n");

	for (i=0;i<cpu_count;i++) {
		tstorage[i] = i;
#ifdef ACTUALLYDISPLAY
		r_threads[i] = SDL_CreateThread(fract_thread_dummy, &tstorage[i]); // create and run n threads
		res = (r_threads[i] == NULL);
#else
		res = pthread_create(r_threads + i, NULL, fract_thread_dummy, &tstorage[i]);
#endif
		if (res) printf("Error creating %d-th thread\n", i);
	}
	cmd_thread = THREAD_RUN_CMD;
}


MultiThreaded:: ~MultiThreaded(void)
{
	int i;
	cmd_thread = THREAD_QUIT_CMD; // tell 'em to quit
#ifdef ACTUALLYDISPLAY
	SDL_Delay(35);
#endif
	for (i=0;i<cpu_count;i++) // unleash the power :)
#ifdef ACTUALLYDISPLAY
		SDL_SemPost(r_sem[i]);
#else
		sem_post(r_sem + i );
#endif
	for (i=0;i<cpu_count;i++) {
#ifdef ACTUALLYDISPLAY
		SDL_WaitThread(r_threads[i], NULL); // wait for completion (=free resources)
#else
		pthread_join(r_threads[i], NULL);
#endif
	}

#ifdef ACTUALLYDISPLAY
	for (i=0;i<cpu_count;i++)
		SDL_DestroySemaphore(r_sem[i]); // destroy thread semaphores
	SDL_DestroySemaphore(main_sem); // destroy main semaphore
	SDL_DestroySemaphore(start_sem);
#else
	for (i=0;i<cpu_count;i++)
		sem_destroy(r_sem+i);
	sem_destroy(&main_sem);
	sem_destroy(&start_sem);

#endif
}

static volatile int lockers[MAX_BARRIERS][MAX_CPU_COUNT];

Barrier::Barrier()
{
	id = -1;
}

void Barrier::checkin(int barrier_id, int thread_idx, int threads_count)
{
	id = barrier_id;
	idx = thread_idx;
	count = threads_count;
}

void Barrier::checkout()
{
	int x = ++lockers[id][idx];
	while (1) {
		bool good = true;
		for (int i = 0; i < count; i++)
			if (lockers[id][i] != x) {
				good = false;
				break;
			}
		if (good) break;
	}
}

void multithreaded_memset(void *data, unsigned fill_pattern, size_t size, int thread_index, int threads_count)
{
	unsigned * p = (unsigned *)data;
	size_t ending = thread_index < threads_count - 1 ? (thread_index+1) * (size / threads_count) : size;
	for (size_t i = thread_index * (size / threads_count); i < ending; i ++)
		p[i] = fill_pattern;
	// for optimal performance check what code does you compiler generate. This loop should be compiled to a
	// single "rep stosd" instruction
}

#define threads1
#include "x86_asm.h"
#undef threads1
