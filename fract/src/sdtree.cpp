/***************************************************************************
 *   Copyright (C) 2006 by Veselin Georgiev                                *
 *   anrieff@mailg.com (convert to gmail)                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "sdtree.h"


/** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * @class SDTreeNode                                              *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
 
SDTreeNode::SDTreeNode()
{
	primitives = NULL;
	primitive_count = 0;
	left = right = NULL;
}

bool SDTreeNode::isleaf() const
{
	return primitives != NULL;
}

SDTreeNode::~SDTreeNode()
{
	if (primitives) {
		delete primitives;
		primitives = NULL;
	} else {
		delete left;
		delete right;
		left = right = NULL;
	}
}
