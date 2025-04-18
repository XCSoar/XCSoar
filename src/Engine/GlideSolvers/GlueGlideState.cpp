// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "GlideState.hpp"
#include "Navigation/Aircraft.hpp"
#include "Task/Points/TaskPoint.hpp"

#include <algorithm>

#include <cassert>

GlideState
GlideState::Remaining(const TaskPoint &tp,
                      const AircraftState &aircraft,
                      const double min_h)
{
  assert(aircraft.location.IsValid());

  return GlideState(tp.GetVectorRemaining(aircraft.location),
                    std::max(min_h, tp.GetElevation()),
                    aircraft.altitude, aircraft.wind);
}
