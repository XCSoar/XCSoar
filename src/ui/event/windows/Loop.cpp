// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Loop.hpp"
#include "Event.hpp"
#include "Queue.hpp"
#include "Asset.hpp"

namespace UI {

bool
EventLoop::Get(Event &event)
{
  return queue.Wait(event);
}

void
EventLoop::Dispatch(const Event &event)
{
  if (event.IsCallback()) {
    event.callback(event.callback_ctx);
    return;
  }

  ::TranslateMessage(&event.msg);
  ::DispatchMessage(&event.msg);
}

void
DialogEventLoop::Dispatch(Event &event)
{
  if (!event.IsCallback() && ::IsDialogMessage(dialog, &event.msg))
    return;

  EventLoop::Dispatch(event);
}

} // namespace UI
