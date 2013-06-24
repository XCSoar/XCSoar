/*
 * Copyright (C) 2009-2013 Max Kellermann <max@duempel.org>
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

#ifndef XCSOAR_THREAD_POSIX_MUTEX_HXX
#define XCSOAR_THREAD_POSIX_MUTEX_HXX

#include <pthread.h>

/**
 * Low-level wrapper for a pthread_mutex_t.
 */
class PosixMutex {
  pthread_mutex_t mutex;

  friend class Cond;

public:
  /**
   * Create a "fast" mutex.
   */
#if !defined(__BIONIC__) || !defined(__clang__)
  constexpr
#endif
  PosixMutex():mutex(PTHREAD_MUTEX_INITIALIZER) {}

  PosixMutex(const PosixMutex &other) = delete;
  PosixMutex &operator=(const PosixMutex &other) = delete;

  void Lock() {
    pthread_mutex_lock(&mutex);
  }

  bool TryLock() {
    return pthread_mutex_trylock(&mutex) == 0;
  }

  void Unlock() {
    pthread_mutex_unlock(&mutex);
  }
};

#endif
