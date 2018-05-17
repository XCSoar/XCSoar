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

#ifndef XCSOAR_THREAD_STOPPABLE_THREAD_HPP
#define XCSOAR_THREAD_STOPPABLE_THREAD_HPP

#include "Thread/Thread.hpp"
#include "Thread/Trigger.hpp"

/**
 * A thread which can be stopped from the outside.  Implementers must
 * check is_stopped().
 */
class StoppableThread : public Thread {
  Trigger stop_trigger;

public:
  StoppableThread(const char *_name):Thread(_name) {}

  bool Start() {
    stop_trigger.Reset();
    return Thread::Start();
  }

  /**
   * Triggers thread shutdown.  Call Thread::Join() after this to wait
   * synchronously for the thread to exit.
   */
  void BeginStop() {
    stop_trigger.Signal();
  }

protected:
  /**
   * Check this thread has received the "Stop" comand.
   */
  bool CheckStopped() const {
    return stop_trigger.Test();
  }

  /**
   * Wait until the "Stop" command is received.
   *
   * @return true if the "Stop" command was received, false on timeout
   */
  bool WaitForStopped(unsigned timeout_ms) {
    return stop_trigger.Wait(timeout_ms);
  }
};

#endif
