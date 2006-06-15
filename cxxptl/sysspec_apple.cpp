#ifdef __APPLE__
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

#include <unistd.h>
#include <mach/clock_types.h>
#include <mach/clock.h>
#include <mach/mach.h>

int system_get_processor_count(void)
{
	kern_return_t kr;
	host_basic_info_data_t basic_info;
	host_info_t info = (host_info_t)&basic_info;
	host_flavor_t flavor = HOST_BASIC_INFO;
	mach_msg_type_number_t count = HOST_BASIC_INFO_COUNT;
	kr = host_info(mach_host_self(), flavor, info, &count);
	if (kr != KERN_SUCCESS) return 1;
	return basic_info.avail_cpus;
}

 
#endif
