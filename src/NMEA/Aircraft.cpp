// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NMEA/Aircraft.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"
#include "Navigation/Aircraft.hpp"

const AircraftState
ToAircraftState(const MoreData &info, const DerivedInfo &calculated)
{
  AircraftState aircraft;

  /* SPEED_STATE */
  aircraft.ground_speed = info.ground_speed;
  aircraft.true_airspeed = info.true_airspeed;

  /* ALTITUDE_STATE */
  aircraft.altitude = info.NavAltitudeAvailable()
    ? info.nav_altitude
    : 0.;

  aircraft.working_band_fraction = calculated.common_stats.height_fraction_working;

  aircraft.altitude_agl =
    info.NavAltitudeAvailable() && calculated.terrain_valid
    ? calculated.altitude_agl
    : 0.;

  /* VARIO_INFO */
  aircraft.vario = info.brutto_vario;
  aircraft.netto_vario = info.netto_vario;

  /* AIRCRAFT_STATE */
  aircraft.time = info.time_available ? info.time : TimeStamp::Undefined();
  aircraft.location = info.location_available
    ? info.location
    : GeoPoint::Invalid();
  aircraft.track = info.track;
  aircraft.g_load = info.acceleration.available
    ? info.acceleration.g_load
    : 1.;
  aircraft.wind = calculated.GetWindOrZero();
  aircraft.flying = calculated.flight.flying;

  return aircraft;
}
