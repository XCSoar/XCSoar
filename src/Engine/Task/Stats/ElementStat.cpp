// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ElementStat.hpp"

void
ElementStat::Reset() noexcept
{
  location_remaining = GeoPoint::Invalid();
  vector_remaining = GeoVector::Invalid();
  next_leg_vector = GeoVector::Invalid();

  time_started = TimeStamp::Undefined();
  time_elapsed = time_remaining_now = time_remaining_start = time_planned = {};
  gradient = 0;

  remaining_effective.Reset();
  remaining.Reset();
  planned.Reset();
  travelled.Reset();
  pirker.Reset();

  solution_planned.Reset();
  solution_travelled.Reset();
  solution_remaining.Reset();
  solution_mc0.Reset();

  vario.Reset();
}

void
ElementStat::SetTimes(const FloatDuration until_start_s, const TimeStamp ts,
                      const TimeStamp time) noexcept
{
  time_started = ts;

  if (!time_started.IsDefined() || !time.IsDefined())
    /* not yet started */
    time_elapsed = {};
  else
    time_elapsed = fdim(time, ts);

  if (solution_remaining.IsOk()) {
    time_remaining_now = solution_remaining.time_elapsed;
    time_remaining_start = fdim(time_remaining_now, until_start_s);
    time_planned = time_elapsed + time_remaining_start;
  } else {
    time_remaining_now = time_remaining_start = time_planned = {};
  }
}
