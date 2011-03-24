/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
  StoppableThread();

  bool start() {
    stop_trigger.reset();
    return Thread::start();
  }

  /**
   * Triggers thread shutdown.  Call join() after this to wait
   * synchronously for the thread to exit.
   */
  void stop() {
    stop_trigger.trigger();
  }

protected:
  bool is_stopped() {
    return stop_trigger.test();
  }

  bool wait_stopped(unsigned timeout_ms) {
    return stop_trigger.wait(timeout_ms);
  }
};

#endif
