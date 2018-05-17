/*
 * Copyright (C) 2009-2015 Max Kellermann <max.kellermann@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef XCSOAR_THREAD_FAST_SHARED_MUTEX_HXX
#define XCSOAR_THREAD_FAST_SHARED_MUTEX_HXX

#ifdef WIN32

#include "WindowsSharedMutex.hxx"
using FastSharedMutex = WindowsSharedMutex;

#elif defined(ANDROID) && ANDROID_MIN_SDK_VERSION < 9

#include "FallbackSharedMutex.hxx"

class FastSharedMutex : public FallbackSharedMutex {
};

#else

#include "PosixSharedMutex.hxx"
using FastSharedMutex = PosixSharedMutex;

#endif

class ScopeExclusiveLock {
  FastSharedMutex &mutex;

public:
  ScopeExclusiveLock(FastSharedMutex &_mutex):mutex(_mutex) {
    mutex.lock();
  };

  ~ScopeExclusiveLock() {
    mutex.unlock();
  }

  ScopeExclusiveLock(const ScopeExclusiveLock &other) = delete;
  ScopeExclusiveLock &operator=(const ScopeExclusiveLock &other) = delete;
};

class ScopeSharedLock {
  FastSharedMutex &mutex;

public:
  ScopeSharedLock(FastSharedMutex &_mutex):mutex(_mutex) {
    mutex.lock_shared();
  };

  ~ScopeSharedLock() {
    mutex.unlock_shared();
  }

  ScopeSharedLock(const ScopeSharedLock &other) = delete;
  ScopeSharedLock &operator=(const ScopeSharedLock &other) = delete;
};

#endif
