/***************************************************************************
 *   Copyright (C) 2006 by Veselin Georgiev                                *
 *   anrieff@XXX.com (replace XXX with gmail)                              *
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
 *                                                                         *
 *  ---------------------------------------------------------------------  *
 *                                                                         *
 *    Defines sse_malloc and sse_free                                      *
 *                                                                         *
 ***************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include "memory.h"
#include "vector3.h"

// given an (possibly) unaligned pointer, return an 16-byte aligned one (possibly larger)
static inline void * perfectify(void *in)
{
	long t = reinterpret_cast<long>( in );
	t = (t&~15l)+16l;
	return reinterpret_cast<void*>( t );
}

// malloc's memory with forced 16-byte alignment
// Since malloc guarantees 8-byte alignment, we allocate 16 bytes more than
// we need. Then we perfectify() the pointer. We store the address of the
// original malloc() return just before the 16-byte boundary (that is,
// 4 or 8 bytes before the address, that is returned after)
void *sse_malloc(size_t size)
{
	void *res = malloc(size + 16);// malloc guarantees 8-byte alignment
	void *perfect = perfectify(res);
	void **store = (void**)perfect;
	*(--store) = res;
	
	return perfect;
}

void sse_free(void *ptr)
{
	void * to_free = *(((void**)ptr)-1);
	if (perfectify(to_free) == ptr) {
		free(to_free);
	} else {
		printf("Memory corruption!!!\n");
		printf("Something is wrong with sse_malloc/sse_free!\n");
	}
}

#ifdef SIMD_VECTOR
void *Vector::operator new(size_t size)     { return sse_malloc(size); }
void Vector::operator delete(void * what)   { sse_free(what); }
void *Vector::operator new[](size_t size)   { return sse_malloc(size); }
void Vector::operator delete[](void * what) { sse_free(what); }
#endif
