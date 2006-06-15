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

#ifndef __CXXPTL_H__
#define __CXXPTL_H__
 
/**
 * CXXPTL - C++ Portable Thread Library
 * @File   cxxptl.h
 * @Brief  Crossplatform C++ Lib for multithreading and synchronization
 * @Author Veselin Georgiev
 * @Date   2006-06-15
 * 
 * OS Support: 
 *
 *  + POSIX (Linux, BSD, some unices) ... using pthreads
 *  + Win32                           ... using Win32 API
 *
 * @version 0.1.0	- Initial writing
 * @version 0.1.1	- Converted to simple Makefile project
 *
 * Last changed: 2006-06-15
*/
 
//
// Some defines first:
//
/// How many CPUs we support
#define MAX_CPU_COUNT	64

/// How many different Barrier Points we support (@see Barrier)
#define MAX_BARRIERS	64


// 
// Some useful functions:
//

/// returns the number of processors available on the system
int get_processor_count(void);

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
void multithreaded_memset(void *data, unsigned fill_pattern, long size, int thread_index, int threads_count);

/**
 * atomic_add
 * Adds `val' to the integer, pointed by `addr', in an interlocked operation.
 * Returns the value of `addr' before the addition
*/  
int atomic_add(volatile int *addr, int val);

//
// OK, the classes next
//

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
*/ 
class Barrier {
	int id;
	int idx, count;
public:
	Barrier();
	void checkin(int barrier_id, int thread_idx, int threads_count);
	void checkout();
};


/// Simple interlocked variable, with atomic increment and decrementing operators
class InterlockedInt {
	volatile int data;
public:
	InterlockedInt() {}
	InterlockedInt(int val) { set(val); }
	inline void set(int x) { data = x; }
	inline int get(void) { return data; }
	inline int operator ++ () { return 1 + atomic_add(&data, 1); }
	inline int operator ++ (int) { return atomic_add(&data, 1); }
	inline int operator -- () { return -1 + atomic_add(&data, -1); }
	inline int operator -- (int) { return atomic_add(&data, -1); }
	
	/// adds the given value to the variable and returns the value before the addition
	inline int add(int value) { return atomic_add(&data, value); }
};

#ifdef _WIN32
//	Win32
#include <windows.h>
class Mutex {
	CRITICAL_SECTION cs;
public:
	Mutex();
	~Mutex();
	void enter(void);
	void leave(void);
};

class Event {
	HANDLE event;
public:
	Event(void);
	~Event(void);
	void wait(void);
	void signal(void);
	static bool needs_signalling_once;
};

typedef HANDLE ThreadID;
#define relent() Sleep(0)
#else
//	Assuming Linux, APPLE or other POSIX
#include <pthread.h>
class Mutex {
	pthread_mutex_t m;
public:
	Mutex();
	~Mutex();
	void enter(void);
	void leave(void);
};

class Event {
	pthread_mutex_t m;
	pthread_cond_t c;
public:
	Event(void);
	~Event(void);
	void wait(void);
	void signal(void);
	static bool needs_signalling_once;
};

typedef pthread_t ThreadID;
#define relent() sched_yield()
#endif // _WIN32


class Parallel {
	
public:
	virtual void entry(int thread_index, int threads_count) = 0;
	virtual ~Parallel() {}
};

enum ThreadState {
	THREAD_CREATING=100,
	THREAD_SLEEPING,
	THREAD_RUNNING,
	THREAD_EXITING,
	THREAD_DEAD
};


struct ThreadInfoStruct {
	int thread_index, thread_count;
	Event myevent, *thread_pool_event;
	ThreadID thread;
	volatile ThreadState state;
	Parallel *execute_class;
	InterlockedInt *counter;
	bool volatile * waiting;
};

class ThreadPool {
	ThreadInfoStruct info[MAX_CPU_COUNT];
	int active_count;
	InterlockedInt counter;
	Event thread_pool_event;
	volatile bool waiting;
	
	void one_more_thread(void);
public:
	ThreadPool();
	~ThreadPool();
	
	void preload_threads(int count);
	
	void run(Parallel *what, int threads_count);
	
	void killall_threads(void);
};

void new_thread(ThreadID* handle, ThreadInfoStruct *);
void my_thread_proc(ThreadInfoStruct *);


#endif
