// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DeltaTime.hpp"

#include <cassert>

FloatDuration
DeltaTime::Update(TimeStamp current_time, FloatDuration min_delta,
                  FloatDuration warp_tolerance) noexcept
{

  assert(current_time.IsDefined());
  assert(min_delta.count() >= 0);
  assert(warp_tolerance.count() >= 0);

  if (!IsDefined()) {
    /* first call */
    last_time = current_time;
    return {};
  }

  if (current_time < last_time) {
    /* time warp */

    const auto delta = last_time - current_time;
    last_time = current_time;
    return delta < warp_tolerance ? FloatDuration{0} : FloatDuration{-1};
  }

  const auto delta = current_time - last_time;
  if (delta < min_delta)
    /* difference too small, don't update "last" time stamp to let
       small differences add up eventually */
    return {};

  last_time = current_time;

  if (delta > std::chrono::hours{4})
    /* after several hours without a signal, we can assume there was
       a time warp */
    return FloatDuration{-1};

  return delta;
}
