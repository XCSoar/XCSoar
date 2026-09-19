// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Loop.hpp"
#include "Queue.hpp"
#include "Event.hpp"
#include "ui/event/Idle.hpp"
#include "ui/window/TopWindow.hpp"

namespace UI {

bool
EventLoop::Get(Event &event)
{
  if (bulk) {
    if (queue.Pop(event)) {
      event.CopyText();
      return true;
    }

    /* that was the last event for now, refresh the screen now */
    if (top_window != nullptr)
      top_window->Refresh();

    bulk = false;
  }

  if (queue.Wait(event)) {
    event.CopyText();
    bulk = true;
    return true;
  }

  return false;
}

void
EventLoop::Dispatch(const Event &_event)
{
  SDL_Event event = _event.event;
  if (event.type == SDL_EVENT_TEXT_INPUT)
    event.text.text = _event.text.c_str();

  if (event.type == EVENT_CALLBACK) {
    Callback callback = (Callback)event.user.data1;
    callback(event.user.data2);
  } else if (top_window != nullptr) {
    if (top_window->OnEvent(event) &&
        _event.IsUserInput())
      ResetUserIdle();
  }
}

} // namespace UI
