/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

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
    : fixed(0);

  aircraft.working_band_fraction = calculated.thermal_band.working_band_fraction;

  aircraft.altitude_agl =
    info.NavAltitudeAvailable() && calculated.terrain_valid
    ? calculated.altitude_agl
    : fixed(0);

  /* VARIO_INFO */
  aircraft.vario = info.brutto_vario;
  aircraft.netto_vario = info.netto_vario;

  /* AIRCRAFT_STATE */
  aircraft.time = info.time_available ? info.time : fixed(-1);
  aircraft.location = info.location_available
    ? info.location
    : GeoPoint::Invalid();
  aircraft.track = info.track;
  aircraft.g_load = info.acceleration.available
    ? info.acceleration.g_load
    : fixed(1);
  aircraft.wind = calculated.GetWindOrZero();
  aircraft.flying = calculated.flight.flying;

  return aircraft;
}
