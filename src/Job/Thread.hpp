// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "thread/Thread.hpp"
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

  /**
   * Throws on error.
   */
  void Start();

  /**
   * Wait until the thread finishes and rethrow exceptions that may
   * have occurred there.
   */
  void Join();

  using ThreadedOperationEnvironment::Cancel;
  using ThreadedOperationEnvironment::IsCancelled;

protected:
  /* virtual methods from class Thread */
  void Run() noexcept override;

  /* virtual methods from class DelayedNotify */
  void OnNotification() override;

  /**
   * Implement this method.  It is run in the main thread.
   */
  virtual void OnComplete() = 0;
};
