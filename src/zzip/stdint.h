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

/* this file ensures that we have some kind of typedef declarations for
   unsigned C9X typedefs. The ISO C 9X: 7.18 Integer types file is stdint.h
 */

#include <zzip/conf.h> 

/* enforce use of ifdef'd C9X entries in system headers */
#define __USE_ANSI 1
#define __USE_ISOC9X 1

#ifdef ZZIP_HAVE_STDINT_H
    /* ISO C 9X: 7.18 Integer types <stdint.h> */
#include <stdint.h>
#elif defined ZZIP_HAVE_SYS_INT_TYPES_H /*solaris*/
#include <sys/int_types.h>
#elif defined ZZIP_HAVE_INTTYPES_H /*freebsd*/
#include <inttypes.h>
#else
    typedef unsigned char uint8_t;      typedef signed char int8_t;

# if ZZIP_SIZEOF_INT && ZZIP_SIZEOF_INT == 2
    typedef unsigned int uint16_t;      typedef signed int int16_t;
# elif ZZIP_SIZEOF_SHORT && ZZIP_SIZEOF_SHORT == 2
    typedef unsigned short uint16_t;    typedef signed short int16_t;
# else
#   error unable to typedef int16_t from either int or short
    typedef unsigned short uint16_t;    typedef signed short int16_t;
# endif

# if defined ZZIP_SIZEOF_INT && ZZIP_SIZEOF_INT == 4
    typedef unsigned int uint32_t;      typedef signed int int32_t;
# elif defined ZZIP_SIZEOF_LONG && ZZIP_SIZEOF_LONG == 4
    typedef unsigned long uint32_t;     typedef signed long int32_t;
# else
#   error unable to typedef int32_t from either int or long
    typedef unsigned long uint32_t;     typedef signed long int32_t;
# endif

/* either (long long) on Unix or (__int64) on Windows */
typedef unsigned _zzip___int64 uint64_t; typedef _zzip___int64 int64_t;

# if defined ZZIP_SIZEOF_INT_P 
#  if ZZIP_SIZEOF_INT_P == ZZIP_SIZEOF_LONG+0
    typedef long intptr_t;
#  elif ZZIP_SIZEOF_INT_P == ZZIP_SIZEOF_INT+0
    typedef int intptr_t;
#  else
    typedef int64_t intptr_t;
#  endif
# endif

#endif /* ZZIP_HAVE_... */
