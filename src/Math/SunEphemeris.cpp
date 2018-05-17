// C program calculating the sunrise and sunset for
// the current date and a fixed location(latitude,longitude)
// Note, twilight calculation gives insufficient accuracy of results
// Jarmo Lammi 1999 - 2001
// Last update July 21st, 2001
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

#include "Math/SunEphemeris.hpp"
#include "Geo/GeoPoint.hpp"
#include "Time/BrokenDateTime.hpp"
#include "Time/RoughTime.hpp"

/** Sun radius in degrees (?) */
static constexpr double SUN_DIAMETER = 0.53;

// Atmospheric refraction degrees
static constexpr double AIR_REFRACTION = 34.0 / 60.0;

namespace SunEphemeris
{
  /**
   * Get the days to J2000
   * FNday only works between 1901 to 2099 - see Meeus chapter 7
   * @param y Year
   * @param m Month
   * @param d Day
   * @param h UT in decimal hours
   * @return days to J2000
   */
  gcc_const
  double FNday(const BrokenDateTime &date_time);

  /**
   * Calculating the hourangle
   * @param lat Latitude
   * @param declin Declination
   * @return The hourangle
   */
  gcc_const
  Angle GetHourAngle(Angle lat, Angle declin);

  /**
   * Calculating the hourangle for twilight times
   * @param lat Latitude
   * @param declin Declination
   * @return The hourangle for twilight times
   */
  gcc_const
  Angle GetHourAngleTwilight(Angle lat, Angle declin);

  /**
   * Find the ecliptic longitude of the Sun
   * @return The ecliptic longitude of the Sun
   */
  gcc_pure
  Angle GetEclipticLongitude(double d, Angle l);

  gcc_pure
  Angle GetMeanSunLongitude(double d);
}

double
SunEphemeris::FNday(const BrokenDateTime &date_time)
{
  assert(date_time.IsPlausible());

  long int luku = -7 * (date_time.year + (date_time.month + 9) / 12) / 4 +
                  275 * date_time.month / 9 + date_time.day +
                  (long int)date_time.year * 367;

  return double(luku) - 730531.5 + (date_time.hour % 24) / 24.;
}

Angle
SunEphemeris::GetHourAngle(Angle lat, Angle declin)
{
  Angle dfo = Angle::Degrees(SUN_DIAMETER / 2 + AIR_REFRACTION);

  // Correction: different sign at southern hemisphere
  if (lat.IsNegative())
    dfo.Flip();

  auto fo = (declin + dfo).tan() * lat.tan();
  return Angle::asin(fo) + Angle::QuarterCircle();
}

Angle
SunEphemeris::GetHourAngleTwilight(Angle lat, Angle declin)
{
  Angle df1 = Angle::Degrees(6);

  // Correction: different sign at southern hemisphere
  if (lat.IsNegative())
    df1.Flip();

  auto fi = (declin + df1).tan() * lat.tan();
  return Angle::asin(fi) + Angle::QuarterCircle();
}

Angle
SunEphemeris::GetEclipticLongitude(double d, Angle L)
{
  //   mean anomaly of the Sun
  Angle g = Angle::Degrees(357.528 + .9856003 * d).AsBearing();

  //   Ecliptic longitude of the Sun
  return (Angle::Degrees(1.915) * g.sin() + L +
          Angle::Degrees(.02) * (g * 2).sin()).AsBearing();
}

Angle
SunEphemeris::GetMeanSunLongitude(double d)
{
  // mean longitude of the Sun
  return Angle::Degrees(280.461 + .9856474 * d).AsBearing();
}

/**
 * Calculates the sun's azimuth at a given location and time
 * @param Location azimuth at what location
 * @param time azimuth at what time
 * @param dec precalculated declination angle
 * @return sun's azimuth
 * @see http://www.providence.edu/mcs/rbg/java/sungraph.htm
 */
static Angle
CalculateAzimuth(const GeoPoint &Location, const BrokenTime &time,
                 const RoughTimeDelta time_zone, const Angle dec)
{
  assert(time.IsPlausible());

  auto T = time.GetSecondOfDay() / 3600. - 12.
    + time_zone.AsMinutes() / 60.;
  Angle t = Angle::Degrees(15) * T;

  return -Angle::FromXY(Location.latitude.cos() * dec.sin() -
                        Location.latitude.sin() * dec.cos() * t.cos(),
                        dec.cos() * t.sin());
}

SunEphemeris::Result
SunEphemeris::CalcSunTimes(const GeoPoint &location,
                           const BrokenDateTime &date_time,
                           const RoughTimeDelta time_zone)
{
  Result result;

  assert(date_time.IsPlausible());

  auto days_to_j2000 = FNday(date_time);

  Angle l = GetMeanSunLongitude(days_to_j2000);

  // Use GetEclipticLongitude to find the ecliptic longitude of the Sun
  Angle lambda = GetEclipticLongitude(days_to_j2000, l);

  // Obliquity of the ecliptic
  Angle obliquity = Angle::Degrees(23.439 - .0000004 * days_to_j2000);

  // Find the RA and DEC of the Sun
  Angle alpha = Angle::FromXY(lambda.cos(), obliquity.cos() * lambda.sin());
  Angle delta = Angle::asin(obliquity.sin() * lambda.sin());

  // Find the Equation of Time in minutes
  // Correction suggested by David Smith
  Angle ll = l - alpha;
  if (l < Angle::HalfCircle())
    ll += Angle::FullCircle();

  auto equation = 1440 * (1 - ll / Angle::FullCircle());

  Angle hour_angle = GetHourAngle(location.latitude, delta);
  Angle hour_angle_twilight = GetHourAngleTwilight(location.latitude, delta);

  result.azimuth = CalculateAzimuth(location, date_time, time_zone, delta);

  // length of twilight in hours
  auto twilight_hours = (hour_angle_twilight - hour_angle).Hours();

  // Conversion of angle to hours and minutes
  result.day_length = 2 * hour_angle.Hours();

  if (result.day_length < 0.0001)
    // arctic winter
    result.day_length = 0;

  result.time_of_sunrise = 12. - hour_angle.Hours()
    + time_zone.AsMinutes() / 60.
    - location.longitude.Degrees() / 15. + equation / 60.;

  if (result.time_of_sunrise > 24)
    result.time_of_sunrise -= 24;

  result.time_of_sunset = result.time_of_sunrise + result.day_length;
  result.time_of_noon = result.time_of_sunrise + hour_angle.Hours();

  // morning twilight begin
  result.morning_twilight = result.time_of_sunrise - twilight_hours;
  // evening twilight end
  result.evening_twilight = result.time_of_sunset + twilight_hours;

  return result;
}

Angle
SunEphemeris::CalcAzimuth(const GeoPoint &location,
                          const BrokenDateTime &date_time,
                          const RoughTimeDelta time_zone)
{
  assert(date_time.IsPlausible());

  auto days_to_j2000 = FNday(date_time);

  Angle l = GetMeanSunLongitude(days_to_j2000);

  // Use GetEclipticLongitude to find the ecliptic longitude of the Sun
  Angle lambda = GetEclipticLongitude(days_to_j2000, l);

  // Obliquity of the ecliptic
  Angle obliquity = Angle::Degrees(23.439 - .0000004 * days_to_j2000);

  // Find the DEC of the Sun
  Angle delta = Angle::asin(obliquity.sin() * lambda.sin());

  return CalculateAzimuth(location, date_time, time_zone, delta);
}
