/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

const AIRCRAFT_STATE
ToAircraftState(const MoreData &info, const DerivedInfo &calculated)
{
  AIRCRAFT_STATE aircraft;

  /* SPEED_STATE */
  aircraft.Speed = info.ground_speed;
  aircraft.TrueAirspeed = info.true_airspeed;
  aircraft.IndicatedAirspeed = info.indicated_airspeed;

  /* ALTITUDE_STATE */
  aircraft.NavAltitude = info.NavAltitude;
  aircraft.working_band_fraction = calculated.thermal_band.working_band_fraction;
  aircraft.AltitudeAGL = calculated.altitude_agl;

  /* VARIO_INFO */
  aircraft.vario = info.BruttoVario;
  aircraft.netto_vario = info.netto_vario;

  /* FLYING_STATE */
  (FlyingState &)aircraft = calculated.flight;

  /* AIRCRAFT_STATE */
  aircraft.Time = info.time;
  aircraft.Location = info.location;
  aircraft.track = info.track;
  aircraft.Gload = info.acceleration.g_load;
  aircraft.wind = calculated.wind;

  return aircraft;
}
