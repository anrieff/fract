#if defined __linux__ || defined unix || defined __APPLE__
/***************************************************************************
 *   Copyright (C) 2006 by Veselin Georgiev                                *
 *   anrieff@mxgail.com (change to gmail)                                  *
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

#include "cxxptl.h"
#include <pthread.h>

#ifndef PTHREAD_MUTEX_RECURSIVE_NP
#	define PTHREAD_MUTEX_RECURSIVE_NP PTHREAD_MUTEX_RECURSIVE
#endif

/**
 @class Mutex
 **/
 
Mutex::Mutex()
{
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
	pthread_mutex_init(&m, &attr);
	pthread_mutexattr_destroy(&attr);
}

Mutex::~Mutex()
{
	pthread_mutex_destroy(&m);
}
 
void Mutex::enter(void)
{
	pthread_mutex_lock(&m);
}

void Mutex::leave(void)
{
	pthread_mutex_unlock(&m);
}
 
/**
 @class Event
 **/

Event::Event()
{
	pthread_mutex_init(&m, NULL);
	pthread_cond_init(&c, NULL);
	state = 0;
}

Event::~Event()
{
	pthread_cond_destroy(&c);
	pthread_mutex_destroy(&m);
}

void Event::wait(void)
{
	pthread_mutex_lock(&m);
	if (state == 1) {
		state = 0;
		pthread_mutex_unlock(&m);
		return;
	}
	pthread_cond_wait(&c, &m);
	state = 0;
	pthread_mutex_unlock(&m);
}

void Event::signal(void)
{
	pthread_mutex_lock(&m);
	state = 1;
	pthread_mutex_unlock(&m);
	pthread_cond_signal(&c);
}

/**
 @class Barrier
 **/

Barrier::Barrier(int cpu_count)
{
	pthread_mutex_init(&m, NULL);
	pthread_cond_init(&c, NULL);
	set_threads(cpu_count);
}

Barrier::~Barrier()
{
	pthread_cond_destroy(&c);
	pthread_mutex_destroy(&m);
}

void Barrier::set_threads(int cpu_count)
{
	counter = cpu_count;
	state = 0;
}

void Barrier::checkout(void)
{
	int r = --counter;
	if (r){
		pthread_mutex_lock(&m);
		if (state == 1) {
			pthread_mutex_unlock(&m);
			return;
		}
		pthread_cond_wait(&c, &m);
		pthread_mutex_unlock(&m);
	} else {
		pthread_mutex_lock(&m);
		state = 1;
		pthread_mutex_unlock(&m);
		pthread_cond_broadcast(&c);
	}
}

// FUNCTIONS

int atomic_add(volatile int *addr, int val) 
{
	__asm__ __volatile__(
			"lock; xadd	%0,	%1\n"
	:"=r"(val), "=m"(*addr)
	:"0"(val), "m"(*addr)
			    );
	return val;
}
//
void* posix_thread_proc(void *data)
{
	ThreadInfoStruct *info = static_cast<ThreadInfoStruct*>(data);
	// fix broken pthreads implementations, where this proc's frame
	// is not 16 byte aligned...
	//
#if defined __GNUC__
	__asm __volatile("andl	$-16,	%%esp" ::: "%esp");
#endif
	//
	my_thread_proc(info);
	return NULL;
}

void new_thread(pthread_t *handle, ThreadInfoStruct *info)
{
	pthread_create(handle, NULL, posix_thread_proc, info);
	pthread_detach(*handle);
}

#ifndef DONT_HAVE_AFFINITY
int system_get_processor_count(void);
static int _intern_cpucount(void)
{
	static int cpucount = -1;
	if (cpucount == -1)
		cpucount = system_get_processor_count();
	return cpucount;
}
int get_affinity_mask(bool* mask)
{
	cpu_set_t s;
	pthread_t th = pthread_self();
	int retval = pthread_getaffinity_np(th, sizeof(s), &s);
	if (retval) return retval;
	int n = _intern_cpucount();
	for (int i = 0; i < n; i++)
		mask[i] = CPU_ISSET(i, &s);
	return 0;
}
int set_affinity_mask(const bool* mask)
{
	cpu_set_t s;
	pthread_t th = pthread_self();
	int n = _intern_cpucount();
	CPU_ZERO(&s);
	for (int i = 0; i < n; i++)
		if (mask[i])
			CPU_SET(i, &s);
	return pthread_setaffinity_np(th, sizeof(s), &s);
}
void set_best_affinity(int thread_index, bool* mask)
{
	int n = _intern_cpucount();
	thread_index %= n;
	mask[thread_index] = true;
}
#else
int get_affinity_mask(bool*) { return -1; }
int set_affinity_mask(const bool*) { return -1; }
void set_best_affinity(int thread_index, bool* mask) {}
#endif
 
#endif

