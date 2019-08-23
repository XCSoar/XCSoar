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

#ifndef XCSOAR_THREAD_SUSPENSIBLE_THREAD_HPP
#define XCSOAR_THREAD_SUSPENSIBLE_THREAD_HPP

#include "Util/Compiler.h"
#include "Thread/Thread.hpp"
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

  bool Start(bool suspended=false) noexcept;

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
  gcc_pure
  bool IsCommandPending() noexcept;

  /**
   * Like IsCommandPending(), but expects the mutex to be locked
   * already.
   */
  gcc_pure
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

#endif
