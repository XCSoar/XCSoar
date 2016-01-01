/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#ifndef XCSOAR_THREAD_NAME_HPP
#define XCSOAR_THREAD_NAME_HPP

#ifdef HAVE_POSIX
#ifdef __BIONIC__
/* Bionic supports pthread_setname_np() since API level 9, but since
   we build with API level 8, we fall back to prctl(PR_SET_NAME) */
#define HAVE_PR_SET_NAME
#else
#define HAVE_PTHREAD_SETNAME_NP
#endif
#endif

#ifdef HAVE_PR_SET_NAME
#include <sys/prctl.h>
#endif

#ifdef HAVE_PTHREAD_SETNAME_NP
#include <pthread.h>
#endif

static inline void
SetThreadName(const char *name)
{
#ifdef HAVE_PTHREAD_SETNAME_NP
#ifdef __APPLE__
  pthread_setname_np(name);
#else
  pthread_setname_np(pthread_self(), name);
#endif
#elif defined(HAVE_PR_SET_NAME)
  prctl(PR_SET_NAME, (unsigned long)name, 0, 0, 0);
#else
  (void)name;
#endif
}

#endif
