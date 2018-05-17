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

#ifndef XCSOAR_JOB_THREAD_HPP
#define XCSOAR_JOB_THREAD_HPP

#include "Thread/Thread.hpp"
#include "Operation/ThreadedOperationEnvironment.hpp"

#include <atomic>
#include <exception>

class Job;

/**
 * Base class for offloading a job into a separate thread.  There, it
 * can be controlled with an OperationEnvironment object.  The
 * specified OperationEnvironment will be wrapped to be thread-safe,
 * i.e. its methods will be called in the main thread.
 */
class JobThread : protected Thread, protected ThreadedOperationEnvironment {
  Job &job;

  /**
   * Is the thread currently running?
   */
  std::atomic<bool> running;

  /**
   * Was the thread running when we last checked?  This is used in
   * OnNotification() to check whether to call OnComplete().
   */
  bool was_running;

  std::exception_ptr exception;

public:
  JobThread(OperationEnvironment &_env, Job &_job)
    :ThreadedOperationEnvironment(_env), job(_job),
     running(false), was_running(false) {}

  bool Start();

  /**
   * Wait until the thread finishes and rethrow exceptions that may
   * have occurred there.
   */
  void Join();

  using ThreadedOperationEnvironment::Cancel;

protected:
  /* virtual methods from class Thread */
  void Run() override;

  /* virtual methods from class DelayedNotify */
  void OnNotification() override;

  /**
   * Implement this method.  It is run in the main thread.
   */
  virtual void OnComplete() = 0;
};

#endif
