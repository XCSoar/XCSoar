// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <synchapi.h>

/**
 * This class wraps an OS specific trigger.  It is an object which one
 * thread can wait for, and another thread can wake it up.
 */
class Trigger {
  HANDLE handle;

public:
  /**
   * Initializes the trigger.
   *
   * @param name an application specific name for this trigger
   */
  Trigger():handle(::CreateEvent(nullptr, true, false, nullptr)) {}

  /**
   * Kills the trigger.
   */
  ~Trigger() {
    ::CloseHandle(handle);
  }

  Trigger(const Trigger &other) = delete;
  Trigger &operator=(const Trigger &other) = delete;

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
    return ::WaitForSingleObject(handle, timeout_ms) == WAIT_OBJECT_0;
  }

  /**
   * Checks if this object is triggered.
   * @return true if this object was triggered, false if not
   */
  [[gnu::pure]]
  bool Test() const {
    return ::WaitForSingleObject(handle, 0) == WAIT_OBJECT_0;
  }

  /**
   * Waits indefinitely until this object is triggered with trigger().
   * If this object is already triggered, this method returns
   * immediately.
   */
  void Wait() {
    Wait(INFINITE);
  }

  /**
   * Wakes up the thread waiting for the trigger.  The state of the
   * trigger is reset only if a thread was really woken up.
   */
  void Signal() {
    ::SetEvent(handle);
  }

  /**
   * Resets the trigger
   */
  void Reset() {
    ::ResetEvent(handle);
  }
};
