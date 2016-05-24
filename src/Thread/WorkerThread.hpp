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

#ifndef XCSOAR_THREAD_WORKER_THREAD_HPP
#define XCSOAR_THREAD_WORKER_THREAD_HPP

#include "Thread/SuspensibleThread.hpp"

/**
 * A thread which performs regular work in background.
 */
class WorkerThread : public SuspensibleThread {
  Cond trigger_cond;
  bool trigger_flag = false;

  const unsigned period_min, idle_min, delay;

public:
  /**
   * @param period_min the minimum duration of one period [ms].  If
   * tick() is faster than that, then the thread will sleep for the
   * rest of the time.  This is can be used for rate-limiting.
   * @param idle_min the minimum sleep time after each tick().  This
   * can be used to reduce overload and lock contention on a very busy
   * and slow machine.
   * @param delay an artificial delay between Trigger() and Tick(), to
   * allow grouping consecutive Trigger() calls into one Tick()
   */
  WorkerThread(const char *_name,
               unsigned period_min=0, unsigned idle_min=0,
               unsigned delay=0);

  /**
   * Wakes up the thread to do work, calls tick().
   */
  void Trigger() {
    const ScopeLock lock(mutex);
    if (!trigger_flag) {
      trigger_flag = true;
      trigger_cond.signal();
    }
  }

  /**
   * Suspend execution until Resume() is called.
   */
  void BeginSuspend() {
    const ScopeLock lock(mutex);
    _BeginSuspend();
  }

  /**
   * Like BeginSuspend(), but expects the mutex to be locked already.
   */
  void _BeginSuspend() {
    SuspensibleThread::_BeginSuspend();
    trigger_cond.signal();
  }

  void Suspend() {
    const ScopeLock lock(mutex);
    _BeginSuspend();
    _WaitUntilSuspended();
  }

  /**
   * Triggers thread shutdown.  Call Thread::Join() after this to wait
   * synchronously for the thread to exit.
   */
  void BeginStop() {
    const ScopeLock lock(mutex);
    SuspensibleThread::_BeginStop();
    trigger_cond.signal();
  }

protected:
  virtual void Run();

  /**
   * Implement this to do the actual work.
   */
  virtual void Tick() = 0;
};

#endif
