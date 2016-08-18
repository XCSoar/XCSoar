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

#ifndef XCSOAR_ASYNC_JOB_RUNNER_HPP
#define XCSOAR_ASYNC_JOB_RUNNER_HPP

#include "Thread/Thread.hpp"

#include <atomic>
#include <exception>

#include <assert.h>

class Job;
class OperationEnvironment;
class ThreadedOperationEnvironment;
class Notify;

/**
 * An environment that runs a #Job in another thread.  It does not
 * wait for completion.  After creating this object, launch a job by
 * calling Start().  The object can be reused after Wait() has been
 * called for the previous #Job.
 */
class AsyncJobRunner final : private Thread {
  Job *job;
  ThreadedOperationEnvironment *env;
  Notify *notify;

  std::atomic<bool> running;

  /**
   * The exception thrown by Job::Run(), to be rethrown by Wait().
   */
  std::exception_ptr exception;

public:
  AsyncJobRunner():running(false) {}

  ~AsyncJobRunner() {
    /* force the caller to invoke Wait() */
    assert(!IsBusy());
  }

  /**
   * Is a #Job currently scheduled, running or finished?
   */
  bool IsBusy() const {
    return IsDefined();
  }

  /**
   * Has Job::Run() returned already?
   */
  bool HasFinished() const {
    assert(IsBusy());

    return !running.load(std::memory_order_relaxed);
  }

  /**
   * Start the specified #Job.  This object must be idle; to clear the
   * previous job, call Wait().
   *
   * @param env an OperationEnvironment that is passed to the #Job;
   * this class will wrap it inside a #ThreadedOperationEnvironment;
   * it must be valid until Wait() returns
   * @param notify an optional object that gets notified when the job
   * finishes
   */
  void Start(Job *job, OperationEnvironment &env, Notify *notify=NULL);

  /**
   * Cancel the current #Job.  Returns immediately; to wait for the
   * #Job to return, call Wait().
   *
   * This suppresses the notification: after this method returns, no
   * notification will be delivered.  It would be dangerous to deliver
   * the notification, because the notification handler will call
   * Wait() a second time.
   */
  void Cancel();

  /**
   * Wait synchronously for completion.  It clears the Job pointer and
   * returns ownership to the caller, who is responsible for deleting
   * it.
   *
   * If Job::Run() threw an exception, it gets rethrown by this method.
   *
   * This method must be called before this object is destructed.
   */
  Job *Wait();

private:
  /* virtual methods from class Thread */
  void Run() override;
};

#endif
