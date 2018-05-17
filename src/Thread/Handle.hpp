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

#ifndef XCSOAR_THREAD_HANDLE_HPP
#define XCSOAR_THREAD_HANDLE_HPP

#include "Compiler.h"

#ifdef HAVE_POSIX
#include <pthread.h>
#else
#include <windows.h>
#endif

/**
 * A low-level handle for a thread.  Designed to work with existing
 * threads, such as the main thread.  Mostly useful for debugging
 * code.
 */
class ThreadHandle {
#ifdef HAVE_POSIX
  pthread_t handle;
#else
  DWORD handle;
#endif

public:
  /**
   * No initialisation.
   */
  ThreadHandle() = default;

#ifdef HAVE_POSIX
  constexpr ThreadHandle(pthread_t _handle):handle(_handle) {}
#else
  constexpr ThreadHandle(DWORD _handle):handle(_handle) {}
#endif

  /**
   * Return a handle referring to the current thread.
   */
  gcc_pure
  static const ThreadHandle GetCurrent() {
#ifdef HAVE_POSIX
    return pthread_self();
#else
    return GetCurrentThreadId();
#endif
  }

  gcc_pure
  bool operator==(const ThreadHandle &other) const {
#ifdef HAVE_POSIX
    return pthread_equal(handle, other.handle);
#else
    return handle == other.handle;
#endif
  }

  /**
   * Check if this thread is the current thread.
   */
  bool IsInside() const {
    return *this == GetCurrent();
  }
};

#endif
