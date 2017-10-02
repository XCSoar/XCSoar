/*
 * Copyright (c) 1999-2000 Image Power, Inc. and the University of
 *   British Columbia.
 * Copyright (c) 2001-2002 Michael David Adams.
 * All rights reserved.
 */

/* __START_OF_JASPER_LICENSE__
 * 
 * JasPer License Version 2.0
 * 
 * Copyright (c) 2001-2006 Michael David Adams
 * Copyright (c) 1999-2000 Image Power, Inc.
 * Copyright (c) 1999-2000 The University of British Columbia
 * 
 * All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person (the
 * "User") obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the
 * following conditions:
 * 
 * 1.  The above copyright notices and this permission notice (which
 * includes the disclaimer below) shall be included in all copies or
 * substantial portions of the Software.
 * 
 * 2.  The name of a copyright holder shall not be used to endorse or
 * promote products derived from the Software without specific prior
 * written permission.
 * 
 * THIS DISCLAIMER OF WARRANTY CONSTITUTES AN ESSENTIAL PART OF THIS
 * LICENSE.  NO USE OF THE SOFTWARE IS AUTHORIZED HEREUNDER EXCEPT UNDER
 * THIS DISCLAIMER.  THE SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS
 * "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY RIGHTS.  IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL
 * INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING
 * FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.  NO ASSURANCES ARE
 * PROVIDED BY THE COPYRIGHT HOLDERS THAT THE SOFTWARE DOES NOT INFRINGE
 * THE PATENT OR OTHER INTELLECTUAL PROPERTY RIGHTS OF ANY OTHER ENTITY.
 * EACH COPYRIGHT HOLDER DISCLAIMS ANY LIABILITY TO THE USER FOR CLAIMS
 * BROUGHT BY ANY OTHER ENTITY BASED ON INFRINGEMENT OF INTELLECTUAL
 * PROPERTY RIGHTS OR OTHERWISE.  AS A CONDITION TO EXERCISING THE RIGHTS
 * GRANTED HEREUNDER, EACH USER HEREBY ASSUMES SOLE RESPONSIBILITY TO SECURE
 * ANY OTHER INTELLECTUAL PROPERTY RIGHTS NEEDED, IF ANY.  THE SOFTWARE
 * IS NOT FAULT-TOLERANT AND IS NOT INTENDED FOR USE IN MISSION-CRITICAL
 * SYSTEMS, SUCH AS THOSE USED IN THE OPERATION OF NUCLEAR FACILITIES,
 * AIRCRAFT NAVIGATION OR COMMUNICATION SYSTEMS, AIR TRAFFIC CONTROL
 * SYSTEMS, DIRECT LIFE SUPPORT MACHINES, OR WEAPONS SYSTEMS, IN WHICH
 * THE FAILURE OF THE SOFTWARE OR SYSTEM COULD LEAD DIRECTLY TO DEATH,
 * PERSONAL INJURY, OR SEVERE PHYSICAL OR ENVIRONMENTAL DAMAGE ("HIGH
 * RISK ACTIVITIES").  THE COPYRIGHT HOLDERS SPECIFICALLY DISCLAIM ANY
 * EXPRESS OR IMPLIED WARRANTY OF FITNESS FOR HIGH RISK ACTIVITIES.
 * 
 * __END_OF_JASPER_LICENSE__
 */

/*
 * Memory Allocator
 *
 * $Id$
 */

/******************************************************************************\
* Includes.
\******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <assert.h>

/* We need the prototype for memset. */
#include <string.h>

#include "jasper/jas_malloc.h"
#include "jasper/jas_debug.h"
#include "jasper/jas_math.h"

/******************************************************************************\
* Code.
\******************************************************************************/

#if defined(JAS_DEFAULT_MAX_MEM_USAGE)

static size_t jas_mem = 0;
static size_t jas_max_mem = JAS_DEFAULT_MAX_MEM_USAGE;

typedef struct {
	size_t size;
} jas_mb_t;

#define JAS_MB_ADJUST \
  ((sizeof(jas_mb_t) + sizeof(max_align_t) - 1) / sizeof(max_align_t))
#define JAS_MB_SIZE (JAS_MB_ADJUST * sizeof(max_align_t))

jas_mb_t *jas_get_mb(void *ptr)
{
	return JAS_CAST(jas_mb_t *, JAS_CAST(max_align_t *, ptr) - JAS_MB_ADJUST);
}

void *jas_mb_get_data(jas_mb_t *mb)
{
	return JAS_CAST(void *, JAS_CAST(max_align_t *, mb) + JAS_MB_ADJUST);
}

void jas_set_max_mem_usage(size_t max_mem)
{
	jas_max_mem = max_mem;
}

size_t jas_get_mem_usage()
{
	return jas_mem;
}

void *jas_malloc(size_t size)
{
	void *result;
	jas_mb_t *mb;
	size_t ext_size;
	size_t mem;

	JAS_DBGLOG(100, ("jas_malloc(%zu)\n", size));
#if defined(JAS_MALLOC_RETURN_NULL_PTR_FOR_ZERO_SIZE)
	if (!size) {
		return 0;
	}
#endif
	if (!jas_safe_size_add(size, JAS_MB_SIZE, &ext_size)) {
		jas_eprintf("requested memory size is too large\n");
		result = 0;
		mb = 0;
	} else if (!jas_safe_size_add(jas_mem, size, &mem) || mem > jas_max_mem) {
		jas_eprintf("maximum memory limit would be exceeded\n");
		result = 0;
		mb = 0;
	} else {
		JAS_DBGLOG(100, ("jas_malloc: ext_size=%zu\n", ext_size));
		if ((mb = malloc(ext_size))) {
			result = jas_mb_get_data(mb);
			mb->size = size;
			jas_mem = mem;
		} else {
			result = 0;
		}
	}
	JAS_DBGLOG(99, ("jas_malloc(%zu) -> %p (mb=%p)\n", size, result, mb));
	JAS_DBGLOG(102, ("max_mem=%zu; mem=%zu\n", jas_max_mem, jas_mem));
	return result;
}

void *jas_realloc(void *ptr, size_t size)
{
	void *result;
	jas_mb_t *mb;
	jas_mb_t *old_mb;
	size_t old_size;
	size_t ext_size;
	size_t mem;

	JAS_DBGLOG(100, ("jas_realloc(%p, %zu)\n", ptr, size));
	if (!ptr) {
		return jas_malloc(size);
	}
	if (ptr && !size) {
		jas_free(ptr);
	}
	if (!jas_safe_size_add(size, JAS_MB_SIZE, &ext_size)) {
		jas_eprintf("requested memory size is too large\n");
		return 0;
	}
	old_mb = jas_get_mb(ptr);
	old_size = old_mb->size;
	JAS_DBGLOG(101, ("jas_realloc: old_mb=%p; old_size=%zu\n", old_mb,
	  old_size));
	if (size > old_size) {
		if (!jas_safe_size_add(jas_mem, ext_size, &mem) || mem > jas_max_mem) {
			jas_eprintf("maximum memory limit would be exceeded\n");
			return 0;
		}
	} else {
		if (!jas_safe_size_sub(jas_mem, old_size - size, &jas_mem)) {
			jas_eprintf("heap corruption detected\n");
			abort();
		}
	}
	JAS_DBGLOG(100, ("jas_realloc: realloc(%p, %zu)\n", old_mb, ext_size));
	if (!(mb = realloc(old_mb, ext_size))) {
		result = 0;
	} else {
		result = jas_mb_get_data(mb);
		mb->size = size;
		jas_mem = mem;
	}
	JAS_DBGLOG(100, ("jas_realloc(%p, %zu) -> %p (%p)\n", ptr, size, result,
	  mb));
	JAS_DBGLOG(102, ("max_mem=%zu; mem=%zu\n", jas_max_mem, jas_mem));
	return result;
}

void jas_free(void *ptr)
{
	jas_mb_t *mb;
	size_t mem;
	size_t size;
	JAS_DBGLOG(100, ("jas_free(%p)\n", ptr));
	if (ptr) {
		mb = jas_get_mb(ptr);
		size = mb->size;
		JAS_DBGLOG(101, ("jas_free(%p) (mb=%p; size=%zu)\n", ptr, mb, size));
		if (!jas_safe_size_sub(jas_mem, size, &jas_mem)) {
			jas_eprintf("heap corruption detected\n");
			abort();
		}
		JAS_DBGLOG(100, ("jas_free: free(%p)\n", mb));
		free(mb);
	}
	JAS_DBGLOG(102, ("max_mem=%zu; mem=%zu\n", jas_max_mem, jas_mem));
}

#endif

/******************************************************************************\
* Basic memory allocation and deallocation primitives.
\******************************************************************************/

#if !defined(JAS_DEFAULT_MAX_MEM_USAGE)

void *jas_malloc(size_t size)
{
	void *result;
	JAS_DBGLOG(101, ("jas_malloc(%zu)\n", size));
	result = malloc(size);
	JAS_DBGLOG(100, ("jas_malloc(%zu) -> %p\n", size, result));
	return result;
}

void *jas_realloc(void *ptr, size_t size)
{
	void *result;
	JAS_DBGLOG(101, ("jas_realloc(%p, %zu)\n", ptr, size));
	result = realloc(ptr, size);
	JAS_DBGLOG(100, ("jas_realloc(%p, %zu) -> %p\n", ptr, size, result));
	return result;
}

void jas_free(void *ptr)
{
	JAS_DBGLOG(100, ("jas_free(%p)\n", ptr));
	free(ptr);
}

#endif

/******************************************************************************\
* Additional memory allocation and deallocation primitives
* (mainly for overflow checking).
\******************************************************************************/

void *jas_alloc2(size_t num_elements, size_t element_size)
{
	size_t size;
	if (!jas_safe_size_mul(num_elements, element_size, &size)) {
		return 0;
	}
	return jas_malloc(size);
}

void *jas_alloc3(size_t num_arrays, size_t array_size, size_t element_size)
{
	size_t size;
	if (!jas_safe_size_mul(array_size, element_size, &size) ||
	  !jas_safe_size_mul(size, num_arrays, &size)) {
		return 0;
	}
	return jas_malloc(size);
}

void *jas_realloc2(void *ptr, size_t num_elements, size_t element_size)
{
	size_t size;
	if (!jas_safe_size_mul(num_elements, element_size, &size)) {
		return 0;
	}
	return jas_realloc(ptr, size);
}

void *jas_calloc(size_t num_elements, size_t element_size)
{
	void *ptr;
	size_t size;
	if (!jas_safe_size_mul(num_elements, element_size, &size)) {
		return 0;
	}
	if (!(ptr = jas_malloc(size))) {
		return 0;
	}
	memset(ptr, 0, size);
	return ptr;
}
