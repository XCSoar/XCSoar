// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Navigation/Aircraft.hpp"
#include "Geo/GeoVector.hpp"

AircraftState
AircraftState::GetPredictedState(FloatDuration in_time) const noexcept
{
  AircraftState state_next = *this;
  GeoVector vec(ground_speed * in_time.count(), track);
  state_next.location = vec.EndPoint(location);
  state_next.altitude += vario * in_time.count();
  return state_next;
}
