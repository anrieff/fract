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

#include "cxxptl.h"

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
	return InterlockedExchangeAdd((long*)addr, val);
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

static int _intern_cpucount(void)
{
	static int cpucount = -1;
	if (cpucount == -1)
		cpucount = system_get_processor_count();
	return cpucount;
}

int get_affinity_mask(bool *mask)
{
	int n = _intern_cpucount();
	if (n > MAX_CPU_COUNT) n = MAX_CPU_COUNT;
	PDWORD_PTR sysmask = 0, procmask = 0;
	if (!GetProcessAffinityMask(GetCurrentProcess(), sysmask, procmask)) return -1;
	for (int i = 0; i < n; i++) {
		mask[i] = (0 != (procmask[i / 32] & (1 << (i % 32))));
	}
	return 0;
}

int set_affinity_mask(const bool *mask)
{
	int n = _intern_cpucount();
	if (n > 32) n = 32;
	DWORD_PTR t = 0;
	for (int i = 0; i < n; i++)
		if (mask[i])
			t |= (1u << (i % 32));
	return (NULL != SetThreadAffinityMask(GetCurrentThread(), t)) ? 0 : -1;
}
 
#endif
