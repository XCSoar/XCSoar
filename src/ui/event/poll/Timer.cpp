// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "../Timer.hpp"
#include "../Globals.hpp"
#include "../Queue.hpp"

namespace UI {

Timer::Timer(Callback _callback) noexcept
  :timer_event(event_queue->GetEventLoop(), BIND_THIS_METHOD(OnTimer)),
   callback(std::move(_callback)) {}

void
Timer::Schedule(std::chrono::steady_clock::duration d) noexcept
{
  timer_event.Schedule(d);
}

void
Timer::SchedulePreserve(std::chrono::steady_clock::duration d) noexcept
{
  if (!IsPending())
    Schedule(d);
}

void
Timer::Cancel()
{
  timer_event.Cancel();
}

void
Timer::OnTimer() noexcept
{
  event_queue->Interrupt();

  callback();
}

} // namespace UI
