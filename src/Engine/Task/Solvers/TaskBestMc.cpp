// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TaskBestMc.hpp"
#include "Task/Ordered/Points/OrderedTaskPoint.hpp"

#include <algorithm>

// @todo only engage this class if above final glide at mc=0

TaskBestMc::TaskBestMc(TaskPoint &tp,
                       const AircraftState &_aircraft,
                       const GlideSettings &settings, const GlidePolar &_gp)
  :ZeroFinder(0.1, 10.0, TOLERANCE),
   tm(tp, settings, _gp),
   aircraft(_aircraft)
{
}

static constexpr double TINY = 0.001;

double
TaskBestMc::f(const double mc) noexcept
{
  tm.set_mc(std::max(TINY, mc));
  res = tm.glide_solution(aircraft);

  return res.altitude_difference;
}

bool
TaskBestMc::valid([[maybe_unused]] const double mc) const
{
  return res.IsOk() &&
    res.altitude_difference >= -2 * tolerance * res.vector.distance;
}

double
TaskBestMc::search(const double mc)
{
  // only search if mc zero is valid
  f(0);
  if (valid(0)) {
    auto a = find_zero(mc);
    if (valid(a))
      return a;
  }
  return mc;
}

bool
TaskBestMc::search(const double mc, double &result)
{
  // only search if mc zero is valid
  f(0);
  if (valid(0)) {
    auto a = find_zero(mc);
    if (valid(a)) {
      result = a;
      return true;
    }
  }
  result = mc;
  return false;
}
