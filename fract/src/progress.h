/***************************************************************************
 *   Copyright (C) 2004 by Veselin Georgiev                                *
 *   vesko@ViruS                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef __PROGRESS_H__
#define __PROGRESS_H__
#include <stdlib.h>

#define prog_dist_precalc_base		0.0
#define prog_dist_precalc		0.5
#define prog_text_precalc_base		prog_dist_precalc
#define prog_text_precalc		0.1
#define prog_vox_init_base		(prog_text_precalc_base+prog_text_precalc)
#define prog_vox_init			0.15
#define prog_normal_precalc_base	(prog_vox_init_base+prog_vox_init)
#define prog_normal_precalc		0.25

enum TaskID {
	READOBJ,
	TREEBUILD,
	RCPARRAY,
	MIPGEN,
	HIERGEN,
	YUVBENCH,
};

/**
 * @class ProgressMeter
 * @brief A manager class that keeps track of fract's initialization routines progress
 * @note You should not use this class directly; instead, place Task objects
 *       inside routines, which require lengthy calculations
 */
class ProgressManager {
	double total, current;
	double task_total, task_current;
	char taskname[128];
	double taskstart;
	double tasklengths[96];
	char tasknames[96][128];
	bool taskdone[96];
	int n;
	double lupdate;
	void *surface;
	
	void update(bool use_caching = true);
public:
	ProgressManager();
	void init(void *surface);
	void full_reset(const char * message = NULL);
	void reset(void);
	void add_weight(double addage);
	void task_add(TaskID taskid, const char *taskname);
	void task_add_weight(double weight, const char * taskname);
	void task_set(double percent);
	void task_done(void);
	void taskstats(void);
};

extern ProgressManager progressman;

/*
 * Overview of the usage of ProgressMeter and Task classes:
 * ProgressManager keeps track of global progress. It also does GUI, among other
 * things.
 * Task is used inside the initialization functions, which consume considerable
 * amounts of CPU time.
 * Each task has some `weight' - relative amount of time it requires to complete
 * For the ProgressManager to work correctly, it must know the sum of weights
 * of all tasks, that are going to be executed. The weight of each task is
 * added via add_weight(). This must be done before actual execution of any
 * tasks. After that, each task is created as a Task object, which contains
 * the weight in its constructor. To measure current progress of tasks, you
 * should use Task::progress() or a combination of Task::set_steps()/Task::inc()
 *
 * Here is an simple example: Consider intialization functions A and B, where
 * B takes four times as much time as A to execute.
 *
 * ...
 * if (A_should_be_executed()) 
 *	progressman.add_weight(1);
 * if (B_should_be_executed())
 *	progressman.add_weight(4);
 * ....
 * void A() {
 * 	Task task(1, __FUNCTION__);
 *      ...
 * }
 *
 * void B() {
 *	Task task(4, __FUNCTION__);
 *	...
 * }
 *
 */

/**
 * @class Task
 * @brief Represents an initialization task; interfaces the ProgressMeter \
 *        to show current progress status.
 */
class Task {
	int steps;
	int cstep;
	ProgressManager *pm;
	bool finished;
public:
	Task(double weight, ProgressManager* pm = &progressman);
	Task(TaskID id, const char *fun_name, ProgressManager* pm = &progressman);
	~Task();
	
	void finish(void);
	void progress(double prog);
	void set_steps(int steps);
	void inc(int increment = 1);
	void operator += (int increment);
	void operator ++ (int);
	void operator ++ ();
};

void estimate_progress(void);

#endif //__PROGRESS_H__
