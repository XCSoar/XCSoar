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
  Window::CreateMessageWindow();
#endif
}

void
Notify::SendNotification() noexcept
{
  if (pending.exchange(true, std::memory_order_relaxed))
    return;

#ifdef USE_WINUSER
  SendUser(0);
#else
  event_queue->InjectCall(Callback, this);
#endif
}

void
Notify::ClearNotification() noexcept
{
  if (!pending.exchange(false, std::memory_order_relaxed))
    return;

#ifndef USE_WINUSER
  event_queue->Purge(Callback, this);
#endif
}

void
Notify::RunNotification() noexcept
{
  if (pending.exchange(false, std::memory_order_relaxed))
    callback();
}

#ifdef USE_WINUSER

bool
Notify::OnUser([[maybe_unused]] unsigned id) noexcept
{
  RunNotification();
  return true;
}

#else

void
Notify::Callback(void *ctx) noexcept
{
  Notify &notify = *(Notify *)ctx;
  notify.RunNotification();
}

#endif

} // namespace UI
