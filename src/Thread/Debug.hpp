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

#ifndef XCSOAR_THREAD_DEBUG_HXX
#define XCSOAR_THREAD_DEBUG_HXX

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

#endif
