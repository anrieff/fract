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
}

Event::~Event()
{
	pthread_cond_destroy(&c);
	pthread_mutex_destroy(&m);
}

void Event::wait(void)
{
	pthread_mutex_lock(&m);
	pthread_cond_wait(&c, &m);
	pthread_mutex_unlock(&m);
}

void Event::signal(void)
{
	pthread_cond_signal(&c);
}

bool Event::needs_signalling_once = false;

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
	my_thread_proc(info);
	return NULL;
}

void new_thread(pthread_t *handle, ThreadInfoStruct *info)
{
	pthread_create(handle, NULL, posix_thread_proc, info);
	pthread_detach(*handle);
}

 
#endif

