// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "../Timer.hpp"
#include "../Globals.hpp"
#include "../Queue.hpp"

#include <cassert>
#include <utility>

namespace UI {

void
Timer::Schedule(std::chrono::steady_clock::duration d) noexcept
{
  Cancel();

  pending = true;
  event_queue->AddTimer(*this, d);
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
  if (std::exchange(pending, false))
    event_queue->CancelTimer(*this);
}

void
Timer::Invoke()
{
  assert(pending);
  pending = false;

  callback();
}

} // namespace UI
