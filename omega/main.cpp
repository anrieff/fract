/***************************************************************************
 *   Copyright (C) 2006 by Veselin Georgiev                                *
 *   anrieff@mgail.com (convert to gmail)                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
***************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "pluginmanager.h"
#include "cxxptl.h"
#include "display.h"

static int def_xres = 640, def_yres = 480;
static ThreadPool thread_pool;

//static int cpucount = 1;
static int cpucount = get_processor_count();

static void init(void)
{
	if (!load_plugins()) {
		printf("Error loading plugins!\n");
		exit(1);
	}
	printf("%d plugins loaded:\n", get_plugin_count());
	for (int i = 0; i < get_plugin_count(); i++) {
		Plugin *p = get_plugin(i);
		PluginInfo info = p->get_info();
		printf("%s: %s\n", info.name, info.description);
	}
	if (!gui.init(def_xres, def_yres)) {
		printf("Unable to init the GUI!\n");
		free_plugins();
		exit(1);
	}
}

static void finish(void)
{
	free_plugins();
}

class Worker : public Parallel {
	Plugin *plugin;
	InterlockedInt cline;
	volatile int abort;
	View v;
	double ysize;
public:
	FrameBuffer f;
		
	void per_frame_init(View nv, Plugin *plg)
	{
		cline = 0;
		abort = 0;
		v = nv;
		ysize = v.size * f.y / f.x;
		plugin = plg;
	}
	
	Rgb shade(int i, int j)
	{
		double x = v.x + (i - f.x/2) / (f.x/2.0) * v.size;
		double y = v.y + (j - f.y/2) / (f.x/2.0) * v.size;
		return plugin->shade(x, y);
	}
	
	void entry(int tidx, int ttotal)
	{
		int j;
		for (;;) {
			if (abort) break;
			j = cline++;
			if (j >= f.y) break;
			Rgb *line = f.data + (j * f.x);
			for (int i = 0; i < f.x; i++) {
				line[i] = shade(i, j);
			}
		}
	}
	
	~Worker() {}
};

struct IterationSample {
	IterationPoint p;
	double lx[3], ly[3];
	bool disabled;
};

class DynSysWorker : public Parallel {
	Plugin *plugin;
public:
	volatile int abort;
	volatile int idx;
	int size;
	IterationSample *data;
	DynSysWorker()
	{
		data = NULL;
	}
	
	~DynSysWorker()
	{
		if (data) delete [] data;
		data = NULL;
	}
	
	void init(Plugin *plg)
	{
		plugin = plg;
		if (data) delete [] data;
		abort = 0;
		idx = 1;
	}
	
	void entry(int tidx, int ttotal)
	{
		int lidx = -1;
		while (1) {
			if (abort) return;
			while (!abort && idx == lidx) relent();
			if (abort) return;
			
			int mi = idx;
			int ni = (mi + 1) % 3;

			for (int i = tidx; i * 64 < size; i += ttotal) {
				for (int j = 0; j < 64; j++) {
					int k = i * 64 + j;
					if (k >= size) break;
					IterationSample &s = data[k];
					if (s.disabled) continue;
					s.p.x = s.lx[mi];
					s.p.y = s.ly[mi];
					if (!plugin->iterate(&s.p))
						s.disabled = true;
					else {
						s.lx[ni] = s.p.x;
						s.ly[ni] = s.p.y;
					}
				}
			}
			lidx = mi;
		}
	}
};

static void do_main(void)
{
	FrameBuffer fb(def_xres, def_yres);
	Plugin *plug = get_current_plugin();
	bool newplug = true;
	View v, nv;
	Worker worker;
	DynSysWorker dsworker;
	int cpusdsw = cpucount-1; if (cpusdsw == 0) cpusdsw++;
	double vm2time=0;


	worker.f.init(def_xres, def_yres);
	while (1) {
		if (newplug) {
			v.x = v.y = 0;
			v.size = 10;
			nv = plug->get_default_view();
			newplug = false;
		}
		if (viewmode == 1) {
			if (viewmode_changed) {
				dsworker.abort = 1;
				thread_pool.wait(); //if needed
				viewmode_changed = false;
			}
			worker.per_frame_init(nv, plug);
			thread_pool.run(&worker, cpucount);
			fb.copy(worker.f);
		}
		if (viewmode == 2) {
			if (viewmode_changed) {
				double C = 1.0 / (fb.x/2.0) * v.size;
				dsworker.init(plug);
				dsworker.size = 0;
				dsworker.data = new IterationSample[fb.x * fb.y];
				for (int j = 0; j < fb.y; j++) {
					for (int i = 0; i < fb.x; i++) {
						double dx = (rand() % 199) / 199.0;
						double dy = (rand() % 199) / 199.0;
						IterationSample sample;
						sample.disabled = false;
						sample.p.x = v.x + (i + dx - fb.x/2) * C;
						sample.p.y = v.y + (j + dy - fb.y/2) * C;
						plug->init_point(&sample.p);
						sample.lx[0] = sample.p.x;
						sample.ly[0] = sample.p.y;
						if (!plug->iterate(&sample.p)) {
							sample.disabled = true;
						} else {
							sample.lx[1] = sample.p.x;
							sample.ly[1] = sample.p.y;
						}
						dsworker.data[dsworker.size++] = sample;
					}
				}
				thread_pool.run_async(&dsworker, cpusdsw);
				vm2time = gui.time();
				viewmode_changed = false;
			}
			fb.zero();
			double t = gui.time() - vm2time;
			if (t > 1) {
				vm2time = gui.time();
				
				dsworker.idx = (dsworker.idx + 1) % 3;
				
				t = gui.time() - vm2time;
			}
			double CC = 1.0 / v.size * fb.x / 2.0;
			int ni = dsworker.idx;
			int mi = (ni + 2) % 3;
			for (int i = 0; i < dsworker.size; i++) {
				IterationSample &s = dsworker.data[i];
				if (s.disabled) continue;
				double px = s.lx[mi] + (s.lx[ni] - s.lx[mi]) * t;
				double py = s.ly[mi] + (s.ly[ni] - s.ly[mi]) * t;
				double x = fb.x / 2.0 + (px - v.x) * CC;
				double y = fb.y / 2.0 + (py - v.y) * CC;
				if (x < 0 || y < 0 || x >= fb.x - 1 || y >= fb.y - 1) continue;
				Rgb &pix = fb.data[((int) y) * fb.x + ((int) x)];
				int lum = (pix &0xff) + 64;
				if (lum > 255) lum = 255;
				pix = RGB(lum,lum,lum);
			}
		}
		v = nv;
		gui.display(fb);
		gui.update_view(nv);
		if (gui.should_exit()) {
			if (viewmode == 2 || viewmode_changed) {
				dsworker.abort = 1;
				thread_pool.wait();
			}
			break;
		}
	}
}

int main(void)
{
	init();
	do_main();
	finish();
	return 0;
}
