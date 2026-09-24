// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifdef ANDROID
#include "android/Queue.hpp"
#elif defined(USE_POLL_EVENT)
#include "poll/Queue.hpp"
#elif defined(ENABLE_SDL)
#include "sdl/Queue.hpp"
#else
#error No EventQueue implementation
#endif

namespace UI {

#ifndef USE_POLL_EVENT

/**
 * Stub for backends which do not receive signals through the event
 * loop; see poll/Queue.hpp.
 */
static inline void
BlockSignals() noexcept
{
}

#endif

/**
 * Suspend the EventQueue and resume it at the end of the scope.  This
 * is useful while a subprocess runs and we're waiting for it.
 */
class ScopeSuspendEventQueue {
  EventQueue &event_queue;

public:
  explicit ScopeSuspendEventQueue(EventQueue &_event_queue) noexcept
    :event_queue(_event_queue) {
    event_queue.Suspend();
  }

  ~ScopeSuspendEventQueue() noexcept {
    event_queue.Resume();
  }

  ScopeSuspendEventQueue(const ScopeSuspendEventQueue &) = delete;
  ScopeSuspendEventQueue &operator=(const ScopeSuspendEventQueue &) = delete;
};

} // namespace UI
