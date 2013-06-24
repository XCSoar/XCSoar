/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#ifndef XCSOAR_THREAD_COND_HPP
#define XCSOAR_THREAD_COND_HPP

#include "Thread/Mutex.hpp"

#include <pthread.h>
#include <sys/time.h>

/**
 * This class wraps a POSIX pthread_cond_t.
 */
class Cond {
  pthread_cond_t cond;

public:
#if !defined(__BIONIC__) || !defined(__clang__)
  constexpr
#endif
  Cond():cond(PTHREAD_COND_INITIALIZER) {}

  Cond(const Cond &other) = delete;
  Cond &operator=(const Cond &other) = delete;

public:
  /**
   * Waits until this object is signalled with signal().
   *
   * @param mutex a mutex which is already locked, which is unlocked
   * atomically while we wait
   */
  void Wait(PosixMutex &mutex) {
    pthread_cond_wait(&cond, &mutex.mutex);
  }

  void Wait(Mutex &mutex) {
    TemporaryUnlock unlock(mutex);
    Wait(mutex.mutex);
  }

  /**
   * Waits until this object is signalled with signal().
   *
   * @param mutex a mutex which is already locked, which is unlocked
   * atomically while we wait
   * @param timeout_ms the maximum number of milliseconds to wait
   * @return true if this object was triggered, false if the timeout
   * has expired
   */
  bool Wait(PosixMutex &mutex, unsigned timeout_ms) {
    struct timeval now;
    gettimeofday(&now, NULL);
    long future_us = now.tv_usec + timeout_ms * 1000;

    struct timespec timeout;
    timeout.tv_sec = now.tv_sec + future_us / 1000000;
    timeout.tv_nsec = (future_us % 1000000) * 1000;

    return pthread_cond_timedwait(&cond, &mutex.mutex, &timeout) == 0;
  }

  bool Wait(Mutex &mutex, unsigned timeout_ms) {
    TemporaryUnlock unlock(mutex);
    return Wait(mutex.mutex, timeout_ms);
  }

  void Signal() {
    pthread_cond_signal(&cond);
  }

  void Broadcast() {
    pthread_cond_broadcast(&cond);
  }
};

#endif
