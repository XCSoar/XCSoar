// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "thread/Thread.hpp"
#include "Mutex.hxx"
#include "Cond.hxx"

/**
 * A thread which can be suspended and stopped from the outside.
 * Implementers must check CheckStopped().
 */
class SuspensibleThread : public Thread {
protected:
  Mutex mutex;

  Cond command_trigger, client_trigger;

private:
  bool stop_received, suspend_received, suspended;

public:
  SuspensibleThread(const char *_name) noexcept:Thread(_name) {}

  void Start(bool suspended=false);

  /**
   * Triggers thread shutdown.  Call Thread::Join() after this to wait
   * synchronously for the thread to exit.
   */
  void BeginStop() noexcept;

  void BeginSuspend() noexcept;

  void WaitUntilSuspended() noexcept;

  void Suspend() noexcept;

  void Resume() noexcept;

protected:
  /**
   * Like BeginStop(), but expects the mutex to be locked
   * already.
   */
  void _BeginStop() noexcept;

  /**
   * Like BeginSuspend(), but expects the mutex to be locked
   * already.
   */
  void _BeginSuspend() noexcept;

  /**
   * Like WaitUntilSuspended(), but expects the mutex to be locked
   * already.
   */
  void _WaitUntilSuspended(std::unique_lock<Mutex> &lock) noexcept;

  /**
   * Has a suspend or stop command been received?
   */
  [[gnu::pure]]
  bool IsCommandPending() noexcept;

  /**
   * Like IsCommandPending(), but expects the mutex to be locked
   * already.
   */
  [[gnu::pure]]
  bool _IsCommandPending() const noexcept;

  /**
   * Handles the "suspend" and "stop" commands.
   *
   * @return true if the thread shall be stopped
   */
  bool CheckStoppedOrSuspended() noexcept;

  /**
   * Like CheckStoppedOrSuspended(), but expects the mutex to be
   * locked already.
   */
  bool _CheckStoppedOrSuspended(std::unique_lock<Mutex> &lock) noexcept;

  bool WaitForStopped(std::chrono::steady_clock::duration timeout) noexcept;

  bool WaitForStopped(unsigned timeout_ms) noexcept {
    return WaitForStopped(std::chrono::milliseconds(timeout_ms));
  }

  /**
   * Like WaitForStopped(), but expects the mutex to be locked
   * already.
   */
  bool _WaitForStopped(std::unique_lock<Mutex> &lock,
                       std::chrono::steady_clock::duration timeout) noexcept;

  bool _WaitForStopped(std::unique_lock<Mutex> &lock,
                       unsigned timeout_ms) noexcept {
    return _WaitForStopped(lock, std::chrono::milliseconds(timeout_ms));
  }
};
