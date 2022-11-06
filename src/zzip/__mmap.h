/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#pragma once

#include <zzip/types.h>

/*
 * DO NOT USE THIS CODE.
 *
 * It is an internal header file for zziplib that carries some inline
 * functions (or just static members) and a few defines, simply to be
 * able to reuse these across - and have everything in a specific place.
 *
 * Copyright (c) Guido Draheim, use under copyleft (LGPL,MPL)
 */

#ifdef _USE_MMAP
#if    defined ZZIP_HAVE_SYS_MMAN_H
#include <sys/mman.h>
#define USE_POSIX_MMAP 1
#elif defined ZZIP_HAVE_WINBASE_H || defined _WIN32
#include <windows.h>
#define USE_WIN32_MMAP 1
#else
#undef _USE_MMAP
#endif
#endif

/* -------------- specify MMAP function imports --------------------------- */

#if     defined  USE_POSIX_MMAP
#define USE_MMAP 1

#define _zzip_mmap(user, fd, offs, len) \
              mmap (0, len, PROT_READ, MAP_SHARED, fd, offs)
#define _zzip_munmap(user, ptr, len) \
              munmap (ptr, len)
#define _zzip_getpagesize(user) getpagesize()

#ifndef MAP_FAILED /* hpux10.20 does not have it */
#define MAP_FAILED ((void*)(-1))
#endif

#elif   defined USE_WIN32_MMAP
#define USE_MMAP 1
#ifndef MAP_FAILED
#define MAP_FAILED 0
#endif
/* we had used the plugin->sys variable for (user) but not anymore */

static size_t win32_getpagesize (void)
{ 
    SYSTEM_INFO si; GetSystemInfo (&si); 
    return si.dwAllocationGranularity; 
}
static void*  win32_mmap (long* user, int fd, zzip_off_t offs, size_t len)
{
    if (! user || *user != 1) /* || offs % getpagesize() */
	return 0;
  {
    HANDLE hFile = (HANDLE)_get_osfhandle(fd);
    HANDLE fileMapping = NULL;
    if (hFile)
	fileMapping = CreateFileMapping (hFile, 0, PAGE_READONLY, 0, 0, NULL);
    if (fileMapping != NULL)
    {
	char* p = MapViewOfFile(fileMapping, FILE_MAP_READ, 0, offs, len);
	CloseHandle (fileMapping); *user = 1;
	if (p) return p;
    } 
    return MAP_FAILED;
  }
}
static void win32_munmap (long* user, char* fd_map, size_t len)
{
    UnmapViewOfFile (fd_map);
}

#define _zzip_mmap(user, fd, offs, len) \
        win32_mmap ((long*) &(user), fd, offs, len)
#define _zzip_munmap(user, ptr, len) \
        win32_munmap ((long*) &(user), ptr, len)
#define _zzip_getpagesize(user) win32_getpagesize()

#else   /* disable */
#define USE_MMAP 0
/* USE_MAP is intentional: we expect the compiler to do some "code removal"
 * on any source code enclosed in if (USE_MMAP) {...}   i.e. the unreachable
 * branch of an if (0) {....} is not emitted to the final object binary. */

#ifndef MAP_FAILED
#define MAP_FAILED  0
#endif

#define _zzip_mmap(user, fd, offs, len) (MAP_FAILED)
#define _zzip_munmap(user, ptr, len) {}
#define _zzip_getpagesize(user) 1

#endif /* USE_MMAP defines */
