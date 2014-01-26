/***************************************************************************
 *   Copyright (C) 2006 by Veselin Georgiev                                *
 *   anrieff@mailg.com (convert to gmail)                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef __PHYSICS_H__
#define __PHYSICS_H__

/**
 * @Class	PhysicsHook
 * @Date	2006-11-15
 * @Author	Veselin Georgiev
 * @Brief	Abstract algorithm to mangle physics processing (basically
 * \brief       for inserting and removing objects from the scene)
 */

class PhysicsHook {
public:
	/// called before the main physics engine starts processing.
	/// @param time - the time of the processing
	virtual void preprocess(double time) = 0;
	
	/// called after the main physics engine has completed.
	/// @param time - the time of the processing
	virtual void postprocess(double time) = 0;
	
	virtual ~PhysicsHook() {}
};



void physics_preprocess_hooks(double time);
void physics_postprocess_hooks(double time);
void physics_add_hook(PhysicsHook *hook);

#endif // __PHYSICS_H__
