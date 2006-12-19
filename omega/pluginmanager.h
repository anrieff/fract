/***************************************************************************
 *   Copyright (C) 2006 by Veselin Georgiev                                *
 *   anrieff@mgail.com (convert to gmail)                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
***************************************************************************/
#ifndef __PLUGINMANAGER_H__
#define __PLUGINMANAGER_H__

/// convenience const
#define INF 999666111

struct PluginInfo {
	char name[64];
	char description[128];
	int priority;
};

typedef unsigned Rgb;

#define RGB(R,G,B) ((R << 16)|(G<<8)|B)


/**
 * @struct View
 * @brief represents a partial view of the complex plane
 *
 * The (x, y) coordinates determine the central point of view
 * `size' determine the size in the x direction. E.g. the top-left
 * corner will display point (x - size, y - size * yres / xres) from
 * the complex plane, etc
*/
struct View {
	double x, y;
	double size;
};

struct IterationPoint;

/**
 * @class	Plugin
 * @brief	Abstract class, which each fractal routine must implement
*/
class Plugin {
public:
	virtual ~Plugin() {}

	/// gets info about the plugin: description, load priority...
	virtual PluginInfo get_info() const = 0;
	
	/// gets the fractal's default view
	virtual View get_default_view() const = 0;

	/// gets the maximum number of iterations per pixel
	virtual int get_max_iters() const = 0;

	/// sets the maximum number of iterations per pixel
	virtual void set_max_iters(int x) = 0;

	/**
	 * the fractal routine. Given a point (x + iy), return the
	 * resulting color (based on number of iterations, or, indeed,
	 * whatever you like).
	*/
	virtual Rgb shade(double x, double y) = 0;

	/**
	 * this routine is used to initialize a moving point in the
	 * dynamic system, defined by the fractal. 
	 * IterationPoint's ::x and ::y fields are filled in with the
	 * initial point coordinates prior to running init_point()
	 * Usually you only need to implement this method if you want
	 * to write fractal-specific data to IterationPoint::auxdata section
	*/
	virtual void init_point(IterationPoint *p) {}
	
	/// move the point by one fractal iteration
	/// @returns false if the point cannot be iterated any more
	virtual bool iterate(IterationPoint *p) = 0;
};

/**
 * @struct IterationPoint
 * @brief  Represents a point in the dynamic system
*/
struct IterationPoint {
	/// coordinates of the point
	double x, y;
	
	/// additional data, used to preserve e.g. some state information
	/// for the given point. Might not be used at all.
	char auxdata[48];
};

/// Convenience macro for an implemented plugin.
/// Given the class name, it will generate "C" extern functions that
/// allocate a new object of the implemented class and destroy it as well

#ifdef _WIN32
#	define DLLEXPORT extern "C" __declspec(dllexport)
#else
#	undef DLLEXPORT
#	define DLLEXPORT extern "C"
#endif

#define EXPORT_CLASS(name) \
        DLLEXPORT                                                             \
        Plugin* new_class_instance(void) { return new name; }                 \
        DLLEXPORT                                                             \
        void delete_class_instance(Plugin *plug)                              \
        {                                                                     \
                delete dynamic_cast<name*>(plug);                             \
        }

/// loads all plugins and returns true if successful.
bool load_plugins(void);

/// deletes plugins (with other cleanup, if necessary)
void free_plugins(void);

/// returns the number of available plugins
int get_plugin_count(void);

/// gets the plugin at the given index (zero-based)
Plugin* get_plugin(int idx);

/// gets the currently selected plugin
Plugin* get_current_plugin(void);

/// sets the plugin with the given idx to be current
void set_plugin(int idx);

#endif //__PLUGINMANAGER_H__
