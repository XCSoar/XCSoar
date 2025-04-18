// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "thread/Thread.hpp"
#include "thread/Mutex.hxx"
#include "Cond.hxx"

/**
 * A thread which waits for work in background.  It is similar to
 * #WorkerThread, but more light-weight and provides a mutex for
 * copying parameters into the thread.
 */
class StandbyThread : private Thread {
protected:
  /**
   * A mutex that must be locked before any of the attributes or
   * methods are acessed.
   */
  Mutex mutex;

private:
  Cond cond;

  /**
   * Is the thread alive?  Unlike Thread::IsDefined(), this one is
   * thread-safe.
   */
  bool alive = false;

  /**
   * Is work pending?  This flag gets cleared by the thread as soon as
   * it starts working.
   */
  bool pending = false;

  /**
   * Is the thread currently working, i.e. inside Tick()?
   */
  bool busy = false;

  /**
   * This flag asks the thread to stop.
   */
  bool stop = false;

public:
  explicit StandbyThread(const char *_name);

  /**
   * This destructor verifies that the thread has been stopped.
   */
  ~StandbyThread();

private:
  void TriggerCommand() {
    cond.notify_one();
  }

  void TriggerDone() {
    cond.notify_one();
  }

protected:
  using Thread::SetLowPriority;
  using Thread::SetIdlePriority;

  /**
   * Wakes up the thread to do work, calls Tick().  If the thread is
   * not already running, it is launched.  Must not be called while
   * the thread is busy.
   *
   * Caller must lock the mutex.
   *
   * Throws on error.
   */
  void Trigger();

  /**
   * Same as Trigger(), but automatically lock and unlock the mutex.
   *
   * Caller must not lock the mutex.
   *
   * Throws on error.
   */
  void LockTrigger() {
    const std::lock_guard lock{mutex};
    Trigger();
  }

  /**
   * Is the thread currently working (i.e. inside Tick())?
   *
   * Caller must lock the mutex.
   */
  [[gnu::pure]]
  bool IsBusy() const {
    return pending || busy;
  }

  /**
   * Was the thread asked to stop?  The Tick() implementation should
   * use this to check whether to cancel the operation.
   *
   * Caller must lock the mutex.
   */
  [[gnu::pure]]
  bool IsStopped() const {
    return stop;
  }

  /**
   * Send the "stop" command to the thread.
   *
   * Caller must lock the mutex.
   */
  void StopAsync();

  /**
   * Wait until the current job is done.
   *
   * Caller must lock the mutex.
   */
  void WaitDone(std::unique_lock<Mutex> &lock) noexcept;

  /**
   * Same as WaitDone(), but automatically lock and unlock the mutex.
   *
   * Caller must not lock the mutex.
   */
  void LockWaitDone() {
    std::unique_lock lock{mutex};
    WaitDone(lock);
  }

  /**
   * Wait until the thread has exited.
   *
   * Caller must lock the mutex.
   */
  void WaitStopped();

  /**
   * Stop the thread synchronously.
   *
   * Caller must lock the mutex.
   */
  void Stop() {
    StopAsync();
    WaitStopped();
  }

  void LockStop() {
    const std::lock_guard lock{mutex};
    Stop();
  }

  /**
   * Implement this to do the actual work.  The mutex will be locked,
   * but you should unlock it while doing real work (and re-lock it
   * before returning), or the calling thread will block.
   */
  virtual void Tick() noexcept = 0;

private:
  void Run() noexcept override;
};
