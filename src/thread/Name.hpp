// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifdef HAVE_POSIX

#ifdef __APPLE__
#define HAVE_PTHREAD_SETNAME_NP
#else

#include <features.h>
#if defined(__GLIBC__) || defined(__BIONIC__)
#define HAVE_PTHREAD_SETNAME_NP
#endif

#endif

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
#else
  (void)name;
#endif
}
