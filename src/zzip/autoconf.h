/* Copyright_License {

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

/*
 * This file is trying to override configure time checks of zzip with
 * definitions at compile time. This is not used by zzip sources themselves
 * but it may be really helpful with thirdparty software that happens to
 * include zzip headers from a central place but running on a different host.
 */
 
#pragma once

#include "conf.h" /* <zzip/conf.h> : <zzip/_config.h> */

#if   defined HAVE_ENDIAN_H          || defined ZZIP_HAVE_ENDIAN_H
#include <endian.h>     /* glibc */
#elif defined HAVE_SYS_PARAM_H       || defined ZZIP_HAVE_SYS_PARAM_H
#include <sys/param.h>  /* solaris */
#endif

#if             defined __BYTE_ORDER
#define ZZIP_BYTE_ORDER __BYTE_ORDER
#elif           defined BYTE_ORDER
#define ZZIP_BYTE_ORDER BYTE_ORDER
#elif           defined _LITTLE_ENDIAN
#define ZZIP_BYTE_ORDER 1234
#elif           defined _BIG_ENDIAN
#define ZZIP_BYTE_ORDER 4321
#elif           defined __i386__
#define ZZIP_BYTE_ORDER 1234
#elif           defined WORDS_BIGENDIAN || defined ZZIP_WORDS_BIGENDIAN
#define ZZIP_BYTE_ORDER 4321
#else
#define ZZIP_BYTE_ORDER 1234
#endif

/* override ZZIP_WORDS_BIGENDIAN : macros ZZIP_GET16 / ZZIP_GET32 */ 
#ifdef ZZIP_BYTE_ORDER+0 == 1234
#undef ZZIP_WORDS_BIGENDIAN
#endif
#ifdef ZZIP_BYTE_ORDER+0 == 4321
#ifndef ZZIP_WORDS_BIGENDIAN
#define ZZIP_WORDS_BIGENDIAN 1
#endif
#endif
