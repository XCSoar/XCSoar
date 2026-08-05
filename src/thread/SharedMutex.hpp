// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <mutex>

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

#ifdef _WIN32

#include "WindowsSharedMutex.hxx"
using SharedMutex = WindowsSharedMutex;
using SharedLock = std::lock_guard<SharedMutex>;

#elif defined(__APPLE__) && TARGET_OS_IPHONE && !defined(__LP64__)

/**
 * iOS 6's libc++ lacks std::__1::__shared_mutex_base.  Provide the
 * SharedMutex API with exclusive locking on 32-bit iOS to avoid weak
 * deployment against unavailable C++ runtime symbols.
 */
class SharedMutex {
  std::mutex mutex;

public:
  void lock() noexcept {
    mutex.lock();
  }

  void unlock() noexcept {
    mutex.unlock();
  }

  void lock_shared() noexcept {
    mutex.lock();
  }

  void unlock_shared() noexcept {
    mutex.unlock();
  }
};

using SharedLock = std::lock_guard<SharedMutex>;

#else

#include <shared_mutex>

using SharedMutex = std::shared_mutex;
using SharedLock = std::shared_lock<SharedMutex>;

#endif
