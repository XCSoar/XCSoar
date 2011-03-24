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

#ifndef XCSOAR_THREAD_WORKER_THREAD_HPP
#define XCSOAR_THREAD_WORKER_THREAD_HPP

#include "Thread/StoppableThread.hpp"
#include "Thread/Trigger.hpp"

/**
 * A thread which performs regular work in background.
 */
class WorkerThread : public StoppableThread {
  Trigger event_trigger, running;

  unsigned period_min, idle_min;

public:
  /**
   * @param period_min the minimum duration of one period [ms].  If
   * tick() is faster than that, then the thread will sleep for the
   * rest of the time.  This is can be used for rate-limiting.
   * @param idle_min the minimum sleep time after each tick().  This
   * can be used to reduce overload and lock contention on a very busy
   * and slow machine.
   */
  WorkerThread(unsigned period_min=0, unsigned idle_min=0);

  /**
   * Wakes up the thread to do work, calls tick().
   */
  void trigger() {
    event_trigger.trigger();
  }

  /**
   * Suspend execution until resume() is called.
   */
  void suspend() {
    running.reset();
  }

  /**
   * Resume execution after suspend().
   */
  void resume() {
    running.trigger();
  }

  /**
   * Triggers thread shutdown.  Call join() after this to wait
   * synchronously for the thread to exit.
   */
  void stop() {
    StoppableThread::stop();
    trigger();
    resume();
  }

protected:
  virtual void run();

  /**
   * Implement this to do the actual work.
   */
  virtual void tick() = 0;
};

#endif
