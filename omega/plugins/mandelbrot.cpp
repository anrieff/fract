#include <string.h>
#include "pluginmanager.h"

class Mandelbrot : public Plugin {
	int max_iters;
public:
	Mandelbrot() 
	{
		max_iters = 100;
	}
	~Mandelbrot() {}
	PluginInfo get_info() const
	{
		PluginInfo info;
		strcpy(info.name, "Mandelbrot");
		strcpy(info.description,
			"The standard mandelbrot set");
		info.priority = 10;
		return info;
	}
	int get_max_iters() const { return max_iters; }
	void set_max_iters(int x) { max_iters = x; }
	int num_iters(double x, double y) { return (int) (x*100-y*3);}
};

EXPORT_CLASS(Mandelbrot)
