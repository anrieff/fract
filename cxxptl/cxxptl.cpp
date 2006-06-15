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
 @class Barrier
 **/
  
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
					if (Event::needs_signalling_once)
						info->thread_pool_event->signal();
					do {
						if (!Event::needs_signalling_once)
							info->thread_pool_event->signal();
						relent();
					} while (*(info->waiting));
				}
				break;
			}
		}
	} while (!she);
	info->state = THREAD_DEAD;
}


