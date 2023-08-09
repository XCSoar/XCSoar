// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TaskMinTarget.hpp"
#include "Task/Ordered/Points/StartPoint.hpp"

double
TaskMinTarget::f(const double p) noexcept
{
  // set task targets
  set_range(p);

  res = tm.glide_solution(aircraft);
  return (res.time_elapsed - t_remaining).count();
}

inline bool
TaskMinTarget::valid([[maybe_unused]] const double tp) const noexcept
{
  //  const double ff = f(tp);
  return res.IsOk(); // && (ff>= -tolerance*2);
}

double
TaskMinTarget::search(const double tp) noexcept
{
  if (!tm.has_targets())
    // don't bother if nothing to adjust
    return tp;

  force_current = false;
  /// @todo if search fails, force current
  const auto p = find_zero(tp);
  if (valid(p)) {
    return p;
  } else {
    force_current = true;
    return find_zero(tp);
  }
}

inline void
TaskMinTarget::set_range(const double p) noexcept
{
  tm.set_range(p, force_current);
  tp_start.ScanDistanceRemaining(aircraft.location);
}
