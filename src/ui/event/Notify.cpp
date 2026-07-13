// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Notify.hpp"
#include "Globals.hpp"
#include "Queue.hpp"

namespace UI {

Notify::Notify(CallbackFunction _callback) noexcept
  :callback(std::move(_callback))
{
#ifdef USE_WINUSER
  if (event_queue == nullptr)
    Window::CreateMessageWindow();
#endif
}

void
Notify::SendNotification() noexcept
{
  if (pending.exchange(true, std::memory_order_relaxed))
    return;

  if (event_queue != nullptr)
    event_queue->InjectCall(Callback, this);
#ifdef USE_WINUSER
  else
    SendUser(0);
#endif
}

void
Notify::ClearNotification() noexcept
{
  if (!pending.exchange(false, std::memory_order_relaxed))
    return;

  if (event_queue != nullptr)
    event_queue->Purge(Callback, this);
}

void
Notify::RunNotification() noexcept
{
  if (pending.exchange(false, std::memory_order_relaxed))
    callback();
}

void
Notify::Callback(void *ctx) noexcept
{
  Notify &notify = *(Notify *)ctx;
  notify.RunNotification();
}

#ifdef USE_WINUSER

bool
Notify::OnUser([[maybe_unused]] unsigned id) noexcept
{
  RunNotification();
  return true;
}

#endif

} // namespace UI
