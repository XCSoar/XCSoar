// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "thread/SuspensibleThread.hpp"

/**
 * A thread which performs regular work in background.
 */
class WorkerThread : public SuspensibleThread {
  using Duration = std::chrono::steady_clock::duration;

  Cond trigger_cond;
  bool trigger_flag = false;

  const Duration period_min, idle_min, delay;

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
               Duration period_min={}, Duration idle_min={},
               Duration delay={}) noexcept;

  /**
   * Wakes up the thread to do work, calls tick().
   */
  void Trigger() noexcept {
    const std::lock_guard lock{mutex};
    if (!trigger_flag) {
      trigger_flag = true;
      trigger_cond.notify_one();
    }
  }

  /**
   * Suspend execution until Resume() is called.
   */
  void BeginSuspend() noexcept {
    const std::lock_guard lock{mutex};
    _BeginSuspend();
  }

  /**
   * Like BeginSuspend(), but expects the mutex to be locked already.
   */
  void _BeginSuspend() noexcept {
    SuspensibleThread::_BeginSuspend();
    trigger_cond.notify_one();
  }

  void Suspend() noexcept {
    std::unique_lock lock{mutex};
    _BeginSuspend();
    _WaitUntilSuspended(lock);
  }

  /**
   * Triggers thread shutdown.  Call Thread::Join() after this to wait
   * synchronously for the thread to exit.
   */
  void BeginStop() noexcept {
    const std::lock_guard lock{mutex};
    SuspensibleThread::_BeginStop();
    trigger_cond.notify_one();
  }

protected:
  virtual void Run() noexcept;

  /**
   * Implement this to do the actual work.
   */
  virtual void Tick() noexcept = 0;
};
