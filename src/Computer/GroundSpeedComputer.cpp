// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "GroundSpeedComputer.hpp"
#include "NMEA/Info.hpp"
#include "time/Cast.hxx"

void
GroundSpeedComputer::Compute(NMEAInfo &basic)
{
  if (basic.ground_speed_available ||
      !basic.time_available || !basic.location_available) {
    if (!basic.ground_speed_available)
      basic.ground_speed = 0;

    delta_time.Reset();
    last_location_available.Clear();
    return;
  }

  if (!last_location_available)
    delta_time.Update(basic.time, {}, {});
  else if (basic.location_available.Modified(last_location_available)) {
    const auto dt = delta_time.Update(basic.time, FloatDuration{0.2},
                                      std::chrono::seconds{5});
    if (dt.count() > 0) {
      auto distance = basic.location.DistanceS(last_location);
      basic.ground_speed = distance / ToFloatSeconds(dt);
      basic.ground_speed_available = basic.location_available;
    }
  }

  last_location = basic.location;
  last_location_available = basic.location_available;
}
