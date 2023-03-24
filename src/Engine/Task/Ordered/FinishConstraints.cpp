// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FinishConstraints.hpp"
#include "Navigation/Aircraft.hpp"

void
FinishConstraints::SetDefaults()
{
  min_height = 0;
  min_height_ref = AltitudeReference::AGL;
  fai_finish = false;
}

bool
FinishConstraints::CheckHeight(const AircraftState &state,
                               const double finish_elevation) const
{
  if (min_height == 0)
    return true;

  if (min_height_ref == AltitudeReference::MSL)
    return state.altitude >= min_height;
  else
    return state.altitude >= min_height + finish_elevation;
}
