// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device/Simulator.hpp"
#include "NMEA/Info.hpp"
#include "../Simulator.hpp"
#include "Geo/Math.hpp"

void
Simulator::Init(NMEAInfo &basic)
{
  /* just in case DeviceBlackboard::SetStartupLocation never gets
     called, set some dummy values that are better than uninitialised
     values */

  basic.location = GeoPoint::Zero();
  basic.track = Angle::Zero();
  basic.ground_speed = 0;
  basic.gps_altitude = 0;
}

void
Simulator::Touch(NMEAInfo &basic)
{
  assert(is_simulator());

  basic.UpdateClock();
  basic.alive.Update(basic.clock);
  basic.gps.simulator = true;
  basic.gps.real = false;

  basic.location_available.Update(basic.clock);
  basic.track_available.Update(basic.clock);
  basic.ground_speed_available.Update(basic.clock);
  basic.gps_altitude_available.Update(basic.clock);

  basic.time_available.Update(basic.clock);
  basic.time += std::chrono::seconds{1};
  basic.date_time_utc = basic.date_time_utc + std::chrono::seconds{1};
}

void
Simulator::Process(NMEAInfo &basic)
{
  assert(is_simulator());

  Touch(basic);

  basic.location = FindLatitudeLongitude(basic.location, basic.track,
                                         basic.ground_speed);
}
