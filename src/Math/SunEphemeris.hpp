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
#ifndef SUN_EPHEMERIS_HPP
#define SUN_EPHEMERIS_HPP

#include "Math/fixed.hpp"
#include "Math/Angle.hpp"
#include "Compiler.h"

struct GeoPoint;
struct BrokenDateTime;

/**
 * Sun ephemeris model, used largely for calculations of sunset times
 */
class SunEphemeris
{
  /**
   * Get the days to J2000
   * FNday only works between 1901 to 2099 - see Meeus chapter 7
   * @param y Year
   * @param m Month
   * @param d Day
   * @param h UT in decimal hours
   * @return days to J2000
   * @see http://www.sci.fi/~benefon/azimalt.cpp
   */
  gcc_const
  static fixed FNday(const BrokenDateTime &date_time);

  /**
   * Calculating the hourangle
   * @param lat Latitude
   * @param declin Declination
   * @return The hourangle
   */
  gcc_const
  static Angle GetHourAngle(Angle lat, Angle declin);

  /**
   * Calculating the hourangle for twilight times
   * @param lat Latitude
   * @param declin Declination
   * @return The hourangle for twilight times
   */
  gcc_const
  static Angle GetHourAngleTwilight(Angle lat, Angle declin);

  /**
   * Find the ecliptic longitude of the Sun
   * @return The ecliptic longitude of the Sun
   */
  gcc_pure
  static Angle GetEclipticLongitude(fixed d, Angle l);

  gcc_pure
  static Angle GetMeanSunLongitude(fixed d);

public:
  struct Result {
    fixed day_length, morning_twilight, evening_twilight;
    fixed time_of_noon, time_of_sunset, time_of_sunrise;
    Angle azimuth;
  };

  /**
   * Calculates all sun-related important times
   * depending on time of year and location
   * @param Location Location to be used in calculation
   * @param Basic NMEA_INFO for current date
   * @param Calculated DERIVED_INFO (not yet used)
   * @param TimeZone The timezone
   * @return Sunset time
   */
  static Result CalcSunTimes(const GeoPoint &location,
                             const BrokenDateTime &date_time, fixed time_zone);
};

#endif
