// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TaskGlideRequired.hpp"
#include "Task/Ordered/Points/OrderedTaskPoint.hpp"
#include "Util/Tolerances.hpp"

TaskGlideRequired::TaskGlideRequired(TaskPoint &tp,
                                     const AircraftState &_aircraft,
                                     const GlideSettings &settings,
                                     const GlidePolar &_gp)
  :ZeroFinder(-10, 10, TOLERANCE),
   tm(tp, settings, _gp), // Vopt at mc=0
   aircraft(_aircraft)
{
  tm.set_mc(0);
}

double
TaskGlideRequired::f(const double S) noexcept
{
  res = tm.glide_sink(aircraft, S);
  return res.altitude_difference;
}

double
TaskGlideRequired::search(const double S)
{
  auto a = find_zero(S);
  return a/res.v_opt;
}
