// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "thread/Debug.hpp"
#include "thread/Handle.hpp"

#ifndef NDEBUG

#ifdef ANDROID
/* on Android, XCSoar's "main" thread is different from the process
   main thread */
static ThreadHandle main_thread;
#else
static ThreadHandle main_thread = ThreadHandle::GetCurrent();
#endif

#ifdef ANDROID

void
InitThreadDebug()
{
  main_thread = ThreadHandle::GetCurrent();
}

#endif

bool
InMainThread()
{
  return main_thread.IsInside();
}

#endif /* !NDEBUG */
