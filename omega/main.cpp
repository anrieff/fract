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
#include "pluginmanager.h"
#include "cxxptl.h"
#include "display.h"

static int def_xres = 640, def_yres = 480;
static ThreadPool thread_pool;

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
	Rgb *coltable;
	int coltable_size;
public:
	FrameBuffer f;
	
	Worker()
	{
		coltable_size = 1536;
		coltable = new Rgb [coltable_size];
		for (int i = 0; i < 256; i++) coltable[       i] = RGB(  255,     i,     0);
		for (int i = 0; i < 256; i++) coltable[ 256 + i] = RGB(255-i,   255,     0);
		for (int i = 0; i < 256; i++) coltable[ 512 + i] = RGB(    0,   255,     i);
		for (int i = 0; i < 256; i++) coltable[ 768 + i] = RGB(    0, 255-i,   255);
		for (int i = 0; i < 256; i++) coltable[1024 + i] = RGB(    i,     0,   255);
		for (int i = 0; i < 256; i++) coltable[1280 + i] = RGB(  255,     0, 255-i);
	}
	
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
		int iters = plugin->num_iters(x, y);
		if (iters == INF) 
			return 0;
		if (iters < 0) iters = 0;
		return coltable[iters % coltable_size];
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

static void do_main(void)
{
	FrameBuffer fb(def_xres, def_yres);
	Plugin *plug = get_current_plugin();
	bool newplug = true;
	View v, nv;
	Worker worker;
	worker.f.init(def_xres, def_yres);
	while (1) {
		if (newplug) {
			v.x = v.y = 0;
			v.size = 10;
			nv = plug->get_default_view();
			newplug = false;
		}
		worker.per_frame_init(nv, plug);
		thread_pool.run(&worker, cpucount);
		fb.copy(worker.f);
		gui.display(fb);
		v = nv;
		gui.update_view(nv);
		if (gui.should_exit())
			break;
	}
}

int main(void)
{
	init();
	do_main();
	finish();
	return 0;
}
