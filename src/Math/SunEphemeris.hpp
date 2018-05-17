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

#ifndef SUN_EPHEMERIS_HPP
#define SUN_EPHEMERIS_HPP

#include "Math/Angle.hpp"

struct GeoPoint;
struct BrokenDateTime;
class RoughTimeDelta;

/**
 * Sun ephemeris model, used largely for calculations of sunset times
 * @see http://www.sci.fi/~benefon/azimalt.cpp
 */
namespace SunEphemeris
{
  struct Result {
    double day_length, morning_twilight, evening_twilight;
    double time_of_noon, time_of_sunset, time_of_sunrise;
    Angle azimuth;
  };

  /**
   * Calculates all sun-related important times
   * depending on time of year and location
   * @param location Where?
   * @param date_time When?
   * @param time_zone UTC offset for When?
   * @return Attributes of the sun
   */
  Result CalcSunTimes(const GeoPoint &location, const BrokenDateTime &date_time,
                      RoughTimeDelta time_zone);

  /**
   * Calculates only the sun's azimuth
   * @param location Where?
   * @param date_time When?
   * @param time_zone UTC offset for When?
   * @return Attributes of the sun
   */
  Angle CalcAzimuth(const GeoPoint &location, const BrokenDateTime &date_time,
                    RoughTimeDelta time_zone);
}

#endif
