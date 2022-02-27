/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#ifndef XCSOAR_EVENT_QUEUE_HPP
#define XCSOAR_EVENT_QUEUE_HPP

#ifdef ANDROID
#include "android/Queue.hpp"
#elif defined(USE_POLL_EVENT)
#include "poll/Queue.hpp"
#elif defined(ENABLE_SDL)
#include "sdl/Queue.hpp"
#elif defined(_WIN32)
#include "windows/Queue.hpp"
#else
#error No EventQueue implementation
#endif

namespace UI {

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

#endif
