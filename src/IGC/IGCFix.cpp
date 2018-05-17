/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "IGCFix.hpp"
#include "NMEA/Info.hpp"
#include "Units/System.hpp"

bool
IGCFix::Apply(const NMEAInfo &basic)
{
  if (!basic.time_available)
    return false;

  if (!IsDefined() && !basic.location_available)
    return false;

  /* "Use A for a 3D fix and V for a 2D fix (no GPS altitude) or for
     no GPS data" */
  gps_valid = basic.location_available && basic.gps_altitude_available;

  if (basic.location_available)
    location = basic.location;

  time = basic.date_time_utc;

  gps_altitude = basic.gps_altitude_available
    ? (int)basic.gps_altitude
    : 0;

  pressure_altitude = basic.pressure_altitude_available
    ? (int)basic.pressure_altitude
    : (basic.baro_altitude_available
       /* if there's only baro altitude and no QNH, assume baro
          altitude is good enough */
       ? (int)basic.baro_altitude
       /* if all else fails, fall back to GPS altitude, to avoid
          application bugs (SeeYou is known for display errors) */
       : gps_altitude);

  ClearExtensions();

  enl = basic.engine_noise_level_available
    ? (int16_t) basic.engine_noise_level
    : -1;

  trt = basic.track_available
    ? (int16_t) basic.track.Degrees()
    : -1;

  gsp = basic.ground_speed_available
    ? (int16_t) Units::ToUserUnit(basic.ground_speed, Unit::KILOMETER_PER_HOUR)
    : -1;

  if (basic.airspeed_available) {
    ias = (int16_t) Units::ToUserUnit(basic.indicated_airspeed, Unit::KILOMETER_PER_HOUR);
    tas = (int16_t) Units::ToUserUnit(basic.true_airspeed, Unit::KILOMETER_PER_HOUR);
  }

  siu = basic.gps.satellites_used_available
    ? (int16_t) basic.gps.satellites_used
    : -1;

  return true;
}
