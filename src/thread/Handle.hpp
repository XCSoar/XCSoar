// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifdef HAVE_POSIX
#include <pthread.h>
#else
#include <processthreadsapi.h>
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
  constexpr ThreadHandle(pthread_t _handle) noexcept:handle(_handle) {}
#else
  constexpr ThreadHandle(DWORD _handle) noexcept:handle(_handle) {}
#endif

  /**
   * Return a handle referring to the current thread.
   */
  [[gnu::pure]]
  static ThreadHandle GetCurrent() noexcept {
#ifdef HAVE_POSIX
    return pthread_self();
#else
    return GetCurrentThreadId();
#endif
  }

  [[gnu::pure]]
  bool operator==(const ThreadHandle &other) const noexcept {
#ifdef HAVE_POSIX
    return pthread_equal(handle, other.handle);
#else
    return handle == other.handle;
#endif
  }

  /**
   * Check if this thread is the current thread.
   */
  bool IsInside() const noexcept {
    return *this == GetCurrent();
  }
};
