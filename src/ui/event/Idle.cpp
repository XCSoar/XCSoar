// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Idle.hpp"
#include "time/PeriodClock.hpp"

static PeriodClock user_idle_clock;

bool
IsUserIdle(unsigned duration_ms) noexcept
{
  return user_idle_clock.Check(std::chrono::milliseconds(duration_ms));
}

/**
 * Acts as if the user had just interacted with XCSoar.
 */
void
ResetUserIdle() noexcept
{
  user_idle_clock.Update();
}
