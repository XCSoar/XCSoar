// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Notify.hpp"
#include "Globals.hpp"
#include "Queue.hpp"

namespace UI {

Notify::Notify(CallbackFunction _callback) noexcept
  :callback(std::move(_callback))
{
}

void
Notify::SendNotification() noexcept
{
  if (pending.exchange(true, std::memory_order_relaxed))
    return;

  if (event_queue != nullptr)
    event_queue->InjectCall(Callback, this);
}

void
Notify::ClearNotification() noexcept
{
  /* Always purge first.  Gating on pending races with SendNotification
     (pending cleared before InjectCall), which left dangling InjectCall
     entries on the GDI EventQueue after #2663. */
  if (event_queue != nullptr)
    event_queue->Purge(Callback, this);

  pending.store(false, std::memory_order_relaxed);
}

void
Notify::RunNotification() noexcept
{
  if (!pending.exchange(false, std::memory_order_relaxed))
    return;

  if (callback)
    callback();
}

void
Notify::Callback(void *ctx) noexcept
{
  Notify &notify = *(Notify *)ctx;
  notify.RunNotification();
}

} // namespace UI
