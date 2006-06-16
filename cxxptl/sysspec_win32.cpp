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

// FUNCTIONS
int system_get_processor_count(void)
{
	SYSTEM_INFO system_info;
	GetSystemInfo(&system_info);
	return system_info.dwNumberOfProcessors;
}

int atomic_add(volatile int *addr, int val) 
{
	/*	
	__asm {
		mov		edx,	addr
		mov		eax,	val
		lock xadd	[edx],	eax
		mov		val,		eax
	}
	return val;
	*/
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
