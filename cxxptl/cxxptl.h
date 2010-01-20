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
 * @version 0.1.2	- Added documentation
 *
 * Last changed: 2006-06-16
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

/**
 * @class Mutex
 * @brief MUTual EXclusive device
 * 
 * Mutexes are used to protect shared data from race conditions. The only two
 * operations supported are:
 *
 * enter() - acquires a lock. If some other threads currently owns the lock, 
 *           the calling thread is suspended, until the lock is released;
 * leave() - releases a previously acquired lock.
 *
 * In cxxptl, Mutexes are recursive, which means, that one thread my enter()
 * multiple times without being locked.
*/

/**
 * @class Event
 * @brief A Simple blocking event
 *
 * Events are used in inter-thread communication. They are usually used to
 * signal that some condition is met, e.g. computation is completed, etc.
 *
 * There are two operations:
 *
 * wait()ing on the condition. The thread is blocked until the condition is met;
 * signal()ling the condition. If some thread currently waits, it is woken now.
 *
 * The class behaviour follows the Win32 API style Events. If some thread
 * signal()s the condition, but no thread is currently waiting on the
 * condition, later invocation of wait() returns immediately (thus, signals()s
 * are, in a sense, "saved", and not "lost" as in pthread's API).
*/

/**
 * @class Barrier
 * @brief A simple multi-blocking event (used for barriers)
 *
 * Barriers are used for synchronization withing multithreaded procedures, e.g.
 * when some work must be completed by ALL threads before ANY thread continues
 * past some point, called barrier point. Example:
 *
 * SINGLE THREADED FUNCTION:
 * ....
 * memset(a, 0, sizeof(a)); // zero some array
 * for (int i = 0; i < n; i++) a[i] = 2*i; // do some work on the zeroed array
 * ....
 *
 * MULTI THREADED VERSION:
 *
 * static Barrier barrier; // one version for all threads
 * ....
 * multithreaded_memset(a, 0, sizeof(a)); // use all threads for the memset
 * barrier.checkout();                    // synchronize here; all threads
 *                                        // must reach this point before 
 *                                        // continuing.
 * for (int i = thread_idx; i < n; i += thread_count) a[i] = 2*i;
 * // ^ continue the computation
 *
 *
 * Class interface:
 * Constructor - accepts the number of threads, that will use the barrier
 * (might be changed later with a call to set_threads())
 *
 * checkout() - the point where threads wait for the other threads
*/

/// function: relent() - voluntarily relinquishes the CPU to other threads.


#ifdef _WIN32
//	Win32
// exclude some unnecessary stuff from windows.h:
#define WIN32_LEAN_AND_MEAN
// also, define the macros min and max. Otherwise, at least on MSVC 7.1, they get defined
// to some actual working macros by windows.h's minions, but this puzzles up std::min and std::max
#define min min
#define max max
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
};

class Barrier {
	HANDLE event;
	InterlockedInt counter;
public:
	Barrier(int cpu_count);
	~Barrier();

	void set_threads(int cpu_count);
	void checkout(void);
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
	volatile int state;
public:
	Event(void);
	~Event(void);
	void wait(void);
	void signal(void);
};

class Barrier {
	pthread_mutex_t m;
	pthread_cond_t c;
	InterlockedInt counter;
	volatile int state;
public:
	Barrier(int cpu_count);
	~Barrier();

	void set_threads(int cpu_count);
	void checkout(void);
};

typedef pthread_t ThreadID;
#define relent() sched_yield()
#endif // _WIN32

class ThreadPool;

/**
 * @class Parallel 
 * @brief Abstract "worker" class for multithreaded algorithms.
 *
 * This class is used to speed up the execution of a program, by parallelizing
 * some algorithms. If an algorithm is parallelizable (the whole task may be
 * split to smaller subtasks, which don't depend on each other), place the
 * algorithm in the entry() method of a derivate of the Parallel class and
 * (using a ThreadPool) and call ThreadPool::run() method to run the entry()
 * methods on the specified number of processors.
 * 
*/
class Parallel {
	/// The ThreadPool which perform the call to entry(). Set by
	/// ThreadPool::run().
	ThreadPool *threadpool;
public:
	/**
	 * Main thread working procedure
	 * @param thread_index  - the number of the worker, 0..threads_count-1;
	 * @param threads_count - the total count of workers.
	*/ 
	virtual void entry(int thread_index, int threads_count) = 0;
	virtual ~Parallel() {}
};

/// Threads may be in a number of states, here are some:
enum ThreadState {
	THREAD_CREATING=100,
	THREAD_SLEEPING,
	THREAD_RUNNING,
	THREAD_EXITING,
	THREAD_DEAD
};

/// Internal struct for "boss"/"worker" synchronization:
struct ThreadInfoStruct {
	int thread_index, thread_count;
	Event myevent, *thread_pool_event;
	ThreadID thread;
	volatile ThreadState state;
	Parallel *execute_class;
	InterlockedInt *counter;
	bool volatile * waiting;
};

/**
 * @class ThreadPool
 * @brief A "boss" class, owns threads and uses them to execute Parallel classes
 *
 * The ThreadPool class is intended to be used with @class Parallel derivatives.
 * It also contains and controls a pool of threads, so that new threads are
 * created only when needed (as per the run() method) and keeping them until
 * they are needed again. Threads are never killed automatically; you must 
 * either destroy the ThreadPool or call the killall_threads() method to do
 * this.
 *
 * The simplest possible example of using the ThreadPool/Parallel pair is:
 *
 * 
 * @code
 * class MyProc : public Parallel {
 * 	void entry(int thread_index, int threads_count) 
 * 	{
 * 		// do some computations
 * 	}
 * };
 * ...
 * 	ThreadPool thread_pool;
 * 	MyProc my_proc;
 * 	thread_pool.run(&my_proc, get_processor_count());
 * ...
 * @endcode
 * In the example, get_processor_count() is used to determine the optimal
 * number of threads to spawn.
*/ 
class ThreadPool {
	ThreadInfoStruct info[MAX_CPU_COUNT];
	int active_count, m_n;
	InterlockedInt counter;
	Event thread_pool_event;
	volatile bool waiting;
	
	void one_more_thread(void);
public:
	ThreadPool();
	~ThreadPool();
	
	/// preload count threads, so that future invocations of run()
	/// don't waste time in creating threads.
	void preload_threads(int count);
	
	/**
	 * @param what          - the algorithm to run;
	 * @param threads_count - on how many threads to run the algorithm.
	 *
	 * run() returns when all threads finish their work.
	 *
	 * NOTE: when called with threads_count == 1, no threads are ever
	 * created or used; the method just calls what->entry(0, 1) and returns.
	*/ 
	void run(Parallel *what, int threads_count);

	/**
	 * @param what          - the algorithm to run;
	 * @param threads_count - on how many threads to run the algorithm.
	 *
	 * run_async() returns after it has spawned the sufficient number
	 * of threads and has given them assignments. Threads will usually run
	 * even after this method returns; so the only difference from the 
	 * run() method is that run_async() does not wait for threads to 
	 * complete their work before returning.
	 *
	 * This poses a great hazard if you use run() or run_async() after a
	 * call to run_async() - you must be certain that all threads have
	 * finished their work and gone to sleep before a new invocation of
	 * run() or run_async(). To specifically enforce this, use wait()
	 *
	 * NOTE: contrary to run(), when threads_count == 1, a thread is spawned
	 * and run as usual. I.e. the thread manager does not try being smart
	*/
	void run_async(Parallel *what, int threads_count);
	
	/**
	 * waits for threads, spawned from run_async(), to come to rest
	 * NOTE: in other words, run() is equivallent to consecutive calls
	 * of run_async() and wait()
	*/
	void wait(void);
	
	/**
	 * Stops all threads and frees the resources, allocated by them
	 * NOTE: the destructor of ThreadPool does this, so you don't need to
	 * do it explicitly.
	*/
	void killall_threads(void);
};

/// platform dependant procedure to create a new thread
void new_thread(ThreadID* handle, ThreadInfoStruct *);

/// The thread proc, that ThreadPool uses for the threads, spawned by it
void my_thread_proc(ThreadInfoStruct *);

/**
 * @brief get_affinity_mask - get the affinity mask of the calling thread
 *
 * @param mask - an array of MAX_CPU_COUNT bools. Upon completion of this
 *               function, this will be the current thread affinity mask.
 *               `true' means that the thread is allowed to execute on that
 *               logical CPU.
 *               If the function fails, the mask is zeroed.
 * @retval 0 on success, negative on failure.
 */
int get_affinity_mask(bool mask[MAX_CPU_COUNT]);

/**
 * @brief set_best_affinity - selects the "best" logical processor.
 *
 * This function creates an affinity mask that confines the thread to a single
 * logical processor. A subsequent call to set_affinity_mask with the modified
 * mask is needed to actually activate the affinity. Which logical cpu is to be
 * assigned depends on the "thread_index" parameter. The algorithm ensures the
 * following points:
 *
 * 1) All logical CPUs will be used when there are N threads (with thread_index
 *    values in 0..N-1).
 * 2) In case of hyper-threading and N/2 or less threads, the assignment
 *    routine will use different physical cores (no two threads will use the
 *    same physical CPUs, thus degrading performance).
 * 3) With more than N threads, the assignment is cyclical
 *
 * Note: The reason we actually need this function is that logical processor
 * numberings are different on Win32 and Linux. This is especially important
 * because of point 2).
 *
 * @param thread_index - the index of the calling thread as per the
 *        Parallel::entry() nomenclature.
 * @param mask - an array of MAX_CPU_COUNT bools. Output only.
 */
void set_best_affinity(int thread_index, bool mask[MAX_CPU_COUNT]);

/**
 * @brief set_affinity_mask - sets the affinity mask of the calling thread
 * @param mask - an array of MAX_CPU_COUNT bools. A boolean True means
 *               that the thread will be allowed to run on that logical CPU.
 * @retval 0 on success, negative on failure.
 */
int set_affinity_mask(const bool mask[MAX_CPU_COUNT]);

#endif
