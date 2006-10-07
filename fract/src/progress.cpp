/***************************************************************************
 *   Copyright (C) 2006 by Veselin Georgiev                                *
 *   anrieff@mailg.com (convert to gmail)                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "fract.h"
#include "progress.h"
#include "gfx.h"
#include "scene.h"
#include "cpu.h"

#ifdef ACTUALLYDISPLAY
#include <SDL/SDL.h>
#endif

struct TaskInfo {
	TaskID id;
	double weight;
	bool conditional;
};

const TaskInfo taskinfo[] = {
	{ READOBJ  ,  50, true  },
	{ TREEBUILD, 150, true  },
	{ RCPARRAY ,  30, false },
	{ MIPGEN   ,   1, false },
	{ YUVBENCH ,  90, true  },
};

static double findbyid(TaskID id)
{
	for (unsigned i = 0; i < sizeof(taskinfo) / sizeof(taskinfo[0]); i++)
		if (id == taskinfo[i].id)
			return taskinfo[i].weight;
	return 0;
}

ProgressManager::ProgressManager()
{
	reset();
}

void ProgressManager::reset(void)
{
	n = 0;
	total = 0;
	current = 0;
	lupdate = -1;
	memset(taskdone, 0, sizeof(taskdone));
	for (unsigned i = 0; i < sizeof(taskinfo) / sizeof(taskinfo[0]); i++)
		if (taskinfo[i].conditional)
			total += taskinfo[i].weight;
}

void ProgressManager::init(void *surface)
{
	this->surface = surface;
}

void ProgressManager::add_weight(double addage)
{
	total += addage;
}

void ProgressManager::task_add(TaskID id, const char * procname)
{
	double weight=0;
	for (unsigned i = 0; i < sizeof(taskinfo) / sizeof(taskinfo[0]); i++) {
		if (id == taskinfo[i].id) {
			weight = taskinfo[i].weight;
			taskdone[i] = true;
			break;
		}
		if (taskinfo[i].conditional && !taskdone[i]) {
			taskdone[i] = true;
			current += taskinfo[i].weight;
		}
	}
	strcpy(taskname, procname);
	task_current = 0;
	task_total = weight;
	taskstart = bTime();
	update();
}

void ProgressManager::task_set(double percent)
{
	task_current = percent;
	update();
}

void ProgressManager::task_done(void)
{
	double tt = bTime() - taskstart;
	strcpy(tasknames[n], taskname);
	tasklengths[n] = tt;
	n++;
	current += task_total;
	update();
}

void ProgressManager::update(bool use_caching)
{
	if (use_caching && bTime() - lupdate < 0.04) return;
	lupdate = bTime();

#ifdef ACTUALLYDISPLAY
	intro_progress((SDL_Surface*)surface, (current + task_current * task_total) / total);
#endif
}

void ProgressManager::taskstats(void)
{
	double mintt = 1e99;
	for (int i = 0; i < n; i++) {
		if (tasklengths[i] > 0 && tasklengths[i] < mintt)
			mintt = tasklengths[i];
	}
	printf("=============================\n");
	for (int i = 0; i < n; i++) {
		printf("%s took %.2lf time (real: %.3lf seconds)\n", 
		       tasknames[i], tasklengths[i] / mintt, tasklengths[i]);
	}
	printf("=============================\n");
}

ProgressManager progressman;

Task::Task(TaskID id, const char * fn, ProgressManager * pm)
{
	this->pm = pm;
	pm->task_add(id, fn);
	steps = 1;
	cstep = 0;
	finished = false;
}

void Task::set_steps(int steps)
{
	this->steps = steps;
}

void Task::progress(double prog)
{
	if (prog < 0) prog = 0;
	if (prog > 1) prog = 1;
	pm->task_set(prog);
}

void Task::inc(int increment)
{
	cstep += increment;
	if (cstep > steps) cstep = steps;
	pm->task_set((double) cstep / steps);
}

void Task::operator++(int)
{
	inc();
}

void Task::operator++()
{
	inc();
}

void Task::operator += (int increment)
{
	inc(increment);
}

Task::~Task()
{
	finish();
}


void Task::finish(void)
{
	if (!finished) {
		finished = true;
		pm->task_done();
	}
}







void estimate_progress(void)
{
	if (!cpu.sse)
		progressman.add_weight(findbyid(RCPARRAY));
	progressman.add_weight(findbyid(MIPGEN));
}
