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

#ifndef XCSOAR_THREAD_TRIGGER_HXX
#define XCSOAR_THREAD_TRIGGER_HXX

#include "Compiler.h"

#include <windows.h>

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
  gcc_pure
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

#endif
