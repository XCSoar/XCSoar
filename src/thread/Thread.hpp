/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#ifndef XCSOAR_THREAD_THREAD_HPP
#define XCSOAR_THREAD_THREAD_HPP

#include "util/Compiler.h"

#ifdef HAVE_POSIX
#include <pthread.h>
#else
#include <windows.h>
#endif

#include <cassert>

/**
 * This class provides an OS independent view on a thread.
 */
class Thread {
  const char *const name;

#ifdef HAVE_POSIX
  pthread_t handle;
  bool defined;

#ifndef NDEBUG
  /**
   * The thread is currently being created.  This is a workaround for
   * IsInside(), which may return false until pthread_create() has
   * initialised the #handle.
   */
  bool creating;
#endif

#else
  HANDLE handle;
  DWORD id;
#endif

public:

#ifdef HAVE_POSIX
  Thread(const char *_name=nullptr) noexcept:name(_name), defined(false) {
#ifndef NDEBUG
    creating = false;
#endif
  }
#else
  Thread(const char *_name=nullptr) noexcept:name(_name), handle(nullptr) {}
#endif

#ifndef NDEBUG
  ~Thread() noexcept {
    /* all Thread objects must be destructed manually by calling
       Join(), to clean up */
    assert(!IsDefined());
  }
#endif

  Thread(const Thread &other) = delete;
  Thread &operator=(const Thread &other) = delete;

  bool IsDefined() const noexcept {
#ifdef HAVE_POSIX
    return defined;
#else
    return handle != nullptr;
#endif
  }

  /**
   * Check if this thread is the current thread.
   */
  bool IsInside() const noexcept {
#ifdef HAVE_POSIX
#ifdef NDEBUG
    const bool creating = false;
#endif

    return IsDefined() && (creating || pthread_equal(pthread_self(), handle));
#else
    return GetCurrentThreadId() == id;
#endif
  }

  void SetLowPriority() noexcept {
#ifndef HAVE_POSIX
    ::SetThreadPriority(handle, THREAD_PRIORITY_BELOW_NORMAL);
#endif
  }

  void SetIdlePriority() noexcept;

  bool Start() noexcept;
  void Join() noexcept;

#ifndef HAVE_POSIX
  bool Join(unsigned timeout_ms) noexcept;
#endif

protected:
  virtual void Run() noexcept = 0;

private:
#ifdef HAVE_POSIX
  static void *ThreadProc(void *lpParameter) noexcept;
#else
  static DWORD WINAPI ThreadProc(LPVOID lpParameter) noexcept;
#endif
};

#endif
