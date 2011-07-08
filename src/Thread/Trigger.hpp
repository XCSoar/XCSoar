/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#ifndef XCSOAR_THREAD_TRIGGER_HXX
#define XCSOAR_THREAD_TRIGGER_HXX

#include "Util/NonCopyable.hpp"

#ifdef HAVE_POSIX
#include <pthread.h>
#include <sys/time.h>
#else
#include <windows.h>
#endif
#include <tchar.h>

/**
 * This class wraps an OS specific trigger.  It is an object which one
 * thread can wait for, and another thread can wake it up.
 */
class Trigger : private NonCopyable {
#ifdef HAVE_POSIX
  /** this mutex protects the value */
  pthread_mutex_t mutex;

  pthread_cond_t cond;

  bool manual_reset, value;
#else
  HANDLE handle;
#endif

public:
  /**
   * Initializes the trigger.
   *
   * @param name an application specific name for this trigger
   * @param _manual_reset whether trigger needs to be manually reset
   */
#ifdef HAVE_POSIX
  Trigger(bool _manual_reset = true)
    :manual_reset(_manual_reset), value(false) {
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);
  }
#else
  Trigger(bool _manual_reset = true)
    :handle(::CreateEvent(NULL, _manual_reset, false, NULL)) {}
#endif

  /**
   * Kills the trigger.
   */
  ~Trigger() {
#ifdef HAVE_POSIX
    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&mutex);
#else
    ::CloseHandle(handle);
#endif
  }

public:
  /**
   * Waits until this object is triggered with trigger().  If this
   * object is already triggered, this method returns immediately.
   *
   * @param timeout_ms the maximum number of milliseconds to wait
   * @return true if this object was triggered, false if the timeout
   * has expired
   */
  bool Wait(unsigned timeout_ms) {
#ifdef HAVE_POSIX
    bool ret;

    pthread_mutex_lock(&mutex);

    if (!value) {
      struct timeval now;
      gettimeofday(&now, NULL);
      long future_us = now.tv_usec + timeout_ms * 1000;

      struct timespec timeout;
      timeout.tv_sec = now.tv_sec + future_us / 1000000;
      timeout.tv_nsec = (future_us % 1000000) * 1000;

      ret = pthread_cond_timedwait(&cond, &mutex, &timeout) == 0 || value;
    } else
      ret = true;

    if (!manual_reset)
      value = false;

    pthread_mutex_unlock(&mutex);
    return ret;
#else
    if (::WaitForSingleObject(handle, timeout_ms) != WAIT_OBJECT_0)
      return false;
    return true;
#endif
  }

  /**
   * Checks if this object is triggered.
   * @return true if this object was triggered, false if not
   */
  bool Test(void) {
#ifdef HAVE_POSIX
    bool ret;

    pthread_mutex_lock(&mutex);
    ret = value;
    pthread_mutex_unlock(&mutex);

    return ret;
#else
    if (::WaitForSingleObject(handle, 0) != WAIT_OBJECT_0)
      return false;
    return true;
#endif
  }

  /**
   * Waits indefinitely until this object is triggered with trigger().
   * If this object is already triggered, this method returns
   * immediately.
   */
  void Wait() {
#ifdef HAVE_POSIX
    pthread_mutex_lock(&mutex);

    if (!value)
      pthread_cond_wait(&cond, &mutex);

    if (!manual_reset)
      value = false;

    pthread_mutex_unlock(&mutex);
#else
    Wait(INFINITE);
#endif
  }

  /**
   * Wakes up the thread waiting for the trigger.  The state of the
   * trigger is reset only if a thread was really woken up.
   */
  void Signal() {
#ifdef HAVE_POSIX
    pthread_mutex_lock(&mutex);

    if (!value) {
      value = true;
      pthread_cond_broadcast(&cond);
    }

    pthread_mutex_unlock(&mutex);
#else /* !HAVE_POSIX */
#if defined(_WIN32_WCE) && defined(__MINGW32__) && defined(EVENT_SET)
    /* mingw32ce < 0.59 has a bugged SetEvent() implementation in
       kfuncs.h */
    ::EventModify(handle, EVENT_SET);
#else
    ::SetEvent(handle);
#endif
#endif /* !HAVE_POSIX */
  }

  /**
   * Wakes up the thread waiting for the trigger, and immediately
   * resets the state of the trigger.
   */
  void Pulse() {
#ifdef HAVE_POSIX
    pthread_cond_broadcast(&cond);
#else
    ::PulseEvent(handle);
#endif
  }

  /**
   * Resets the trigger
   */
  void Reset() {
#ifdef HAVE_POSIX
    pthread_mutex_lock(&mutex);
    value = false;
    pthread_mutex_unlock(&mutex);
#else /* !HAVE_POSIX */
#if defined(_WIN32_WCE) && defined(__MINGW32__) && defined(EVENT_RESET)
    /* mingw32ce < 0.59 has a bugged SetEvent() implementation in
       kfuncs.h */
    ::EventModify(handle, EVENT_RESET);
#else
    ::ResetEvent(handle);
#endif
#endif /* !HAVE_POSIX */
  }
};

#endif
