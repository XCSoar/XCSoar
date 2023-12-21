// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Components.hpp"
#include "thread/Debug.hpp"
#include "thread/Handle.hpp"

#include <cassert>

FileCache *file_cache;
AsyncTerrainOverviewLoader *terrain_loader;

#ifndef ENABLE_OPENGL
DrawThread *draw_thread;
#endif

#ifndef NDEBUG

#ifdef ENABLE_OPENGL

static const ThreadHandle zero_thread_handle = ThreadHandle();
static ThreadHandle draw_thread_handle;

bool
InDrawThread()
{
#ifdef ENABLE_OPENGL
  return InMainThread() && draw_thread_handle.IsInside();
#else
  return draw_thread != nullptr && draw_thread->IsInside();
#endif
}

void
EnterDrawThread()
{
  assert(InMainThread());
  assert(draw_thread_handle == zero_thread_handle);

  draw_thread_handle = ThreadHandle::GetCurrent();
}

void
LeaveDrawThread()
{
  assert(InMainThread());
  assert(draw_thread_handle.IsInside());

  draw_thread_handle = zero_thread_handle;
}

#else

#include "DrawThread.hpp"

bool
InDrawThread()
{
  return draw_thread != nullptr && draw_thread->IsInside();
}

#endif

#endif
