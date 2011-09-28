// C program calculating the sunrise and sunset for
// the current date and a fixed location(latitude,longitude)
// Note, twilight calculation gives insufficient accuracy of results
// Jarmo Lammi 1999 - 2001
// Last update July 21st, 2001
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

#include "Math/SunEphemeris.hpp"
#include "Engine/Navigation/GeoPoint.hpp"
#include "DateTime.hpp"

// Sun radius in degrees (?)
#define SUN_DIAMETER fixed(0.53)
// Atmospheric refraction degrees
#define AIR_REFRACTION fixed(34.0/60.0)

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
fixed
SunEphemeris::FNday(const BrokenDateTime &date_time)
{
  assert(date_time.Plausible());

  long int luku = -7 * (date_time.year + (date_time.month + 9) / 12) / 4 +
                  275 * date_time.month / 9 + date_time.day +
                  (long int)date_time.year * 367;

  return fixed(luku) - fixed(730531.5) + fixed(date_time.hour % 24) / 24;
}

/**
 * Calculating the hourangle
 * @param lat Latitude
 * @param declin Declination
 * @return The hourangle
 */
// TODO TB: find explanations/links for this and following
Angle
SunEphemeris::GetHourAngle(Angle lat, Angle declin)
{
  Angle dfo = Angle::Degrees(SUN_DIAMETER / 2 + AIR_REFRACTION);

  // Correction: different sign at southern hemisphere
  if (negative(lat.Degrees()))
    dfo.Flip();

  fixed fo = (declin + dfo).tan() * lat.tan();
  fo = asin(fo) + fixed_half_pi;

  return Angle::Radians(fo);
}

/**
 * Calculating the hourangle for twilight times
 * @param lat Latitude
 * @param declin Declination
 * @return The hourangle for twilight times
 */
Angle
SunEphemeris::GetHourAngleTwilight(Angle lat, Angle declin)
{
  Angle df1 = Angle::Degrees(fixed(6));

  // Correction: different sign at southern hemisphere
  if (negative(lat.Degrees()))
    df1.Flip();

  fixed fi = (declin + df1).tan() * lat.tan();
  fi = asin(fi) + fixed_half_pi;

  return Angle::Radians(fi);
}

/**
 * Find the ecliptic longitude of the Sun
 * @return The ecliptic longitude of the Sun
 */
Angle
SunEphemeris::GetEclipticLongitude(fixed d, Angle L)
{
  //   mean anomaly of the Sun
  Angle g = Angle::Degrees(fixed(357.528) + fixed(.9856003) * d).AsBearing();

  //   Ecliptic longitude of the Sun
  return (Angle::Degrees(fixed(1.915)) * g.sin() + L +
          Angle::Degrees(fixed(.02)) * (g * fixed_two).sin()).AsBearing();
}

Angle
SunEphemeris::GetMeanSunLongitude(fixed d)
{
  // mean longitude of the Sun
  return Angle::Degrees(fixed(280.461) + fixed(.9856474) * d).AsBearing();
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
                 const fixed TimeZone, const Angle dec)
{
  assert(time.Plausible());

  fixed T = fixed(time.GetSecondOfDay()) / 3600 - fixed(12) + TimeZone;
  Angle t = Angle::Degrees(fixed(15)) * T;

  return Angle::Radians(-atan2(dec.cos() * t.sin(),
                               Location.latitude.cos() * dec.sin() -
                               Location.latitude.sin() * dec.cos() * t.cos()));
}

/**
 * Calculates all sun-related important times
 * depending on time of year and location
 * @param Location Location to be used in calculation
 * @param Basic NMEA_INFO for current date
 * @param Calculated DERIVED_INFO (not yet used)
 * @param TimeZone The timezone
 * @return Sunset time
 */
void
SunEphemeris::CalcSunTimes(const GeoPoint &Location,
                           const BrokenDateTime &date_time,
                           const fixed TimeZone)
{
  assert(date_time.Plausible());

  fixed DaysToJ2000 = FNday(date_time);

  Angle L = GetMeanSunLongitude(DaysToJ2000);

  // Use GetEclipticLongitude to find the ecliptic longitude of the Sun
  Angle Lambda = GetEclipticLongitude(DaysToJ2000, L);

  // Obliquity of the ecliptic
  Angle Obliquity = Angle::Degrees(fixed(23.439) - fixed(.0000004) * DaysToJ2000);

  // Find the RA and DEC of the Sun
  Angle Alpha = Angle::Radians(atan2(Obliquity.cos() * Lambda.sin(), Lambda.cos()));
  Angle Delta = Angle::Radians(asin(Obliquity.sin() * Lambda.sin()));

  // Find the Equation of Time in minutes
  // Correction suggested by David Smith
  fixed LL = (L - Alpha).Radians();
  if (L.Radians() < fixed_pi)
    LL += fixed_two_pi;

  fixed equation = fixed(1440) * (fixed_one - LL / fixed_two_pi);

  Angle HourAngle = GetHourAngle(Location.latitude, Delta);
  Angle HourAngleTwilight = GetHourAngleTwilight(Location.latitude, Delta);

  Azimuth = CalculateAzimuth(Location, date_time, TimeZone, Delta);

  // length of twilight in hours
  fixed TwilightHours = (HourAngleTwilight - HourAngle).Hours();

  // Conversion of angle to hours and minutes
  DayLength = HourAngle.Hours() * fixed_two;

  if (DayLength < fixed(0.0001))
    // arctic winter
    DayLength = fixed_zero;

  TimeOfSunRise = fixed(12) - HourAngle.Hours() + TimeZone
    - Location.longitude.Degrees() / 15 + equation / 60;

  if (TimeOfSunRise > fixed(24))
    TimeOfSunRise -= fixed(24);

  TimeOfSunSet = TimeOfSunRise + DayLength;
  TimeOfNoon = TimeOfSunRise + HourAngle.Hours();

  // morning twilight begin
  MorningTwilight = TimeOfSunRise - TwilightHours;
  // evening twilight end
  EveningTwilight = TimeOfSunSet + TwilightHours;
}
