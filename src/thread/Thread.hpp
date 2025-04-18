// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/Compiler.h"

#ifdef HAVE_POSIX
#include <pthread.h>
#else
#include <processthreadsapi.h>
#include <windef.h> // for HWND (needed by winbase.h)
#include <winbase.h> // for THREAD_PRIORITY_BELOW_NORMAL
#endif

#include <cassert>

/**
 * This class provides an OS independent view on a thread.
 */
class Thread {
  const char *const name;

#ifdef HAVE_POSIX
  pthread_t handle;
  bool defined = false;

#ifndef NDEBUG
  /**
   * The thread is currently being created.  This is a workaround for
   * IsInside(), which may return false until pthread_create() has
   * initialised the #handle.
   */
  bool creating = false;
#endif

#else
  HANDLE handle = nullptr;
  DWORD id;
#endif

public:

#ifdef HAVE_POSIX
  Thread(const char *_name=nullptr) noexcept:name(_name) {}
#else
  Thread(const char *_name=nullptr) noexcept:name(_name) {}
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

  /**
   * Throws on error.
   */
  void Start();

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
