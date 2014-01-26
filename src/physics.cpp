/***************************************************************************
 *   Copyright (C) 2006 by Veselin Georgiev                                *
 *   anrieff@mailg.com (convert to gmail)                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/


#include "physics.h"
#include "array.h"

static Array<PhysicsHook*> hooks;

void physics_preprocess_hooks(double time)
{
	for (int i = 0; i < hooks.size(); i++)
		if (hooks[i])
			hooks[i]->preprocess(time);
}

void physics_postprocess_hooks(double time)
{
	for (int i = 0; i < hooks.size(); i++)
		if (hooks[i])
			hooks[i]->postprocess(time);
}

void physics_add_hook(PhysicsHook * hook)
{
	hooks += hook;
}
