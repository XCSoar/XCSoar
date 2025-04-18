// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "thread/Thread.hpp"
#include "thread/Trigger.hpp"

/**
 * A thread which can be stopped from the outside.  Implementers must
 * check is_stopped().
 */
class StoppableThread : public Thread {
  Trigger stop_trigger;

public:
  StoppableThread(const char *_name):Thread(_name) {}

  void Start() {
    stop_trigger.Reset();
    Thread::Start();
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
