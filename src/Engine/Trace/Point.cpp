// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Point.hpp"
#include "NMEA/MoreData.hpp"
#include "Navigation/Aircraft.hpp"
#include "Math/Util.hpp"

TracePoint::TracePoint(const MoreData &basic)
  :SearchPoint(basic.location),
   time(basic.time.Cast<Time>()),
   altitude(basic.nav_altitude),
   vario(basic.netto_vario),
   engine_noise_level(basic.engine_noise_level_available
                      ? basic.engine_noise_level
                      : 0u),
   drift_factor(Sigmoid(basic.nav_altitude / 100) * 256)
{
}

TracePoint::TracePoint(const AircraftState &state):
  SearchPoint(state.location),
  time(state.time.Cast<Time>()),
  altitude(state.altitude),
  vario(state.netto_vario),
  engine_noise_level(0),
  drift_factor(Sigmoid(state.altitude_agl / 100) * 256)
{
}
