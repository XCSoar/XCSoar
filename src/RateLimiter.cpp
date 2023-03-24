// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RateLimiter.hpp"

#include <cassert>

RateLimiter::RateLimiter(std::chrono::steady_clock::duration _period,
                         std::chrono::steady_clock::duration _delay) noexcept
  :period(_period - _delay), delay(_delay)
{
  assert(_period >= _delay);
}

void
RateLimiter::Trigger()
{
  if (timer.IsPending())
    return;

  std::chrono::steady_clock::duration schedule = delay;
  const auto elapsed = clock.Elapsed();
  if (elapsed.count() >= 0 && elapsed < period)
    schedule += period - elapsed;

  timer.Schedule(schedule);
}

void
RateLimiter::OnTimer()
{
  clock.Update();
  Run();
}
