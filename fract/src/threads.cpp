#ifdef __APPLE__
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

#include <unistd.h>
#include <mach/clock_types.h>
#include <mach/clock.h>
#include <mach/mach.h>

int system_get_processor_count(void)
{
	kern_return_t kr;
	host_basic_info_data_t basic_info;
	host_info_t info = (host_info_t)&basic_info;
	host_flavor_t flavor = HOST_BASIC_INFO;
	mach_msg_type_number_t count = HOST_BASIC_INFO_COUNT;
	kr = host_info(mach_host_self(), flavor, info, &count);
	if (kr != KERN_SUCCESS) return 1;
	return basic_info.avail_cpus;
}

 
#endif
#if defined __linux__ || defined unix
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

#include <sys/sysinfo.h>
#include <unistd.h>
 
int system_get_processor_count(void)
{
	return sysconf(_SC_NPROCESSORS_ONLN);
}

#endif
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

#include "threads.h"
#include "asmconfig.h"
#include <pthread.h>

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
}

void Barrier::checkout(void)
{
	int r = --counter;
	if (r){
		pthread_mutex_lock(&m);
		pthread_cond_wait(&c, &m);
		pthread_mutex_unlock(&m);
	} else {
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
#if defined __GNUC__ && defined USE_ASSEMBLY
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

 
#endif

#ifdef _WIN32
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

#include "threads.h"

/**
 @class Mutex
 **/
  
Mutex::Mutex()
{
	InitializeCriticalSection(&cs);
}
 
Mutex::~Mutex()
{
	DeleteCriticalSection(&cs);
}

void Mutex::enter(void)
{
	EnterCriticalSection(&cs);
}

void Mutex::leave(void)
{
	LeaveCriticalSection(&cs);
}

/**
 @class Event
 **/

Event::Event()
{
	event = CreateEvent(NULL, FALSE, FALSE, NULL);
}

Event::~Event()
{
	CloseHandle(event);
}

void Event::wait(void)
{
	WaitForSingleObject(event, INFINITE);
}

void Event::signal(void)
{
	SetEvent(event);
}

/**
 @class Barrier
 **/

Barrier::Barrier(int cpu_count)
{
	event = CreateEvent(NULL, TRUE, FALSE, NULL);
	set_threads(cpu_count);
}

Barrier::~Barrier()
{
	CloseHandle(event);
}

void Barrier::set_threads(int cpu_count)
{
	ResetEvent(event);
	counter = cpu_count;
}

void Barrier::checkout(void)
{
	int r = --counter;
	if (r)
		WaitForSingleObject(event, INFINITE);
	else
		SetEvent(event);
}

// FUNCTIONS
int system_get_processor_count(void)
{
	SYSTEM_INFO system_info;
	GetSystemInfo(&system_info);
	return system_info.dwNumberOfProcessors;
}

int atomic_add(volatile int *addr, int val) 
{
	return InterlockedExchangeAdd((volatile long*)addr, val);
}

DWORD WINAPI win32_thread_proc(void *data)
{
	ThreadInfoStruct *info = static_cast<ThreadInfoStruct*>(data);
	my_thread_proc(info);
	return 0;
}

void new_thread(HANDLE *handle, ThreadInfoStruct *info)
{
	DWORD useless;
	*handle = CreateThread(NULL, 0, win32_thread_proc, info, 0, &useless);
	CloseHandle(*handle);
}
 
#endif
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

#include "threads.h"


int system_get_processor_count(void);
int get_processor_count(void)
{
	static bool inited = false;
	static int _thecount=1;
	if (!inited) {
		_thecount = system_get_processor_count();
		inited = true;
	}
	return _thecount;
}


void multithreaded_memset(void *data, unsigned fill_pattern, long size, int thread_index, int threads_count)
{
	unsigned * p = (unsigned *)data;
	long ending = thread_index < threads_count - 1 ? (thread_index+1) * (size / threads_count) : size;
	for (long i = thread_index * (size / threads_count); i < ending; i ++)
		p[i] = fill_pattern;
	// for optimal performance check what code does you compiler generate. This loop should be compiled to a
	// single "rep stosd" instruction
}
 
/**
 @class ThreadPool
 **/
 
void ThreadPool::one_more_thread(void)
{
	ThreadInfoStruct& ti = info[active_count];
	ti.thread_index = active_count;
	ti.state = THREAD_CREATING;
	ti.counter = &counter;
	ti.thread_pool_event = &thread_pool_event;
	ti.execute_class = NULL;
	ti.waiting = &waiting;
	new_thread(&ti.thread, &ti);
	
	++active_count;
	for (int i = active_count-1; i >= 0; i--)
		info[i].thread_count = active_count;
}

ThreadPool::ThreadPool()
{
	active_count = 0;
	counter = 0;
}

ThreadPool::~ThreadPool()
{
	killall_threads();
}

void ThreadPool::killall_threads(void)
{
	for (; active_count> 0; active_count --) {
		ThreadInfoStruct& i = info[active_count-1];
		while (i.state != THREAD_SLEEPING) relent();
		i.state = THREAD_EXITING;
		i.myevent.signal();
		while (i.state != THREAD_DEAD) relent();
	}
}

void ThreadPool::run(Parallel *para, int threads_count)
{
	if (threads_count == 1) {
		para->entry(0, 1);
		return;
	}
	while (active_count < threads_count) 
		one_more_thread();
	int n = threads_count;
	
	waiting = true;
	counter = n;
	for (int i = 0; i < n; i++) {
		info[i].thread_index = i;
		info[i].thread_count = n;
		info[i].execute_class = para;
		while (info[i].state != THREAD_SLEEPING) {
			relent();
		}
		info[i].state = THREAD_RUNNING;
		info[i].myevent.signal();
	}
	thread_pool_event.wait();
	waiting = false;
	
	// round robin all threads until they come to rest
	while (1) {
		bool good = true;
		for (int i = 0; i < n; i++) if (info[i].state != THREAD_SLEEPING) good = false;
		if (good) break;
		relent();
	}
}

void ThreadPool::preload_threads(int count)
{
	if (active_count < count)
		run(NULL, count);
}


/**
 thread function (my_thread_proc)
 **/
void my_thread_proc(ThreadInfoStruct *info)
{
	bool she = false;
	do {

		info->state = THREAD_SLEEPING;
		info->myevent.wait();
		switch (info->state) {
			case THREAD_EXITING: she = true; break;
			case THREAD_RUNNING:
			{
				if (info->execute_class) info->execute_class->entry(info->thread_index, info->thread_count);
				int res = --(*(info->counter));
				if (!res) {
					info->thread_pool_event->signal();
					do {
						relent();
					} while (*(info->waiting));
				}
				break;
			}
			default: break;
		}
	} while (!she);
	info->state = THREAD_DEAD;
}


