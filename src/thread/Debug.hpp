// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifdef NDEBUG

#ifdef ANDROID
static inline void
InitThreadDebug()
{
}
#endif

#ifdef ENABLE_OPENGL

static inline void
EnterDrawThread()
{
}

static inline void
LeaveDrawThread()
{
}

#endif

#else /* !NDEBUG */

#ifdef ANDROID
void
InitThreadDebug();
#endif

bool
InMainThread();

bool
InDrawThread();

#ifdef ENABLE_OPENGL

/**
 * Marks the current thread as DrawThread.  This is used on OpenGL
 * (which has no DrawThread) to allow using InDrawThread() in
 * assertions.
 */
void
EnterDrawThread();

/**
 * Undo the effect of EnterDrawThread().
 */
void
LeaveDrawThread();

#endif

#endif /* !NDEBUG */
