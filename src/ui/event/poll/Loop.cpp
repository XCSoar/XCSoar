// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Loop.hpp"
#include "Queue.hpp"
#include "../shared/Event.hpp"
#include "ui/window/TopWindow.hpp"

namespace UI {

bool
EventLoop::Get(Event &event)
{
  if (queue.IsQuit())
    return false;

  if (bulk) {
    if (queue.Pop(event))
      return true;

    /* that was the last event for now, refresh the screen now */
    if (top_window != nullptr)
      top_window->Refresh();

    bulk = false;
  }

  if (queue.Wait(event)) {
    bulk = true;
    return true;
  }

  return false;
}

void
EventLoop::Dispatch(const Event &event)
{
  if (event.type == Event::CALLBACK) {
    event.callback(event.ptr);
  } else if (top_window != nullptr && event.type != Event::NOP) {
#ifndef NON_INTERACTIVE
    top_window->OnEvent(event);
#endif
  }
}

} // namespace UI
