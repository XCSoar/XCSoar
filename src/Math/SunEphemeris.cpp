// C program calculating the sunrise and sunset for
// the current date and a fixed location(latitude,longitude)
// Note, twilight calculation gives insufficient accuracy of results
// Jarmo Lammi 1999 - 2001
// Last update July 21st, 2001
/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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
SunEphemeris::FNday(int y, int m, int d, fixed h)
{
  long int luku = -7 * (y + (m + 9) / 12) / 4 + 275 * m / 9 + d;
  // type casting necessary on PC DOS and TClite to avoid overflow
  luku += (long int)y * 367;

  return fixed(luku) - fixed(730531.5) + h / 24;
}

/**
 * The function below returns an angle in the range 0 to 2*PI
 * @param x Angle to be converted (amount of a full circle)
 * @return an angle in the range 0 to 2*PI
 * @see http://www.sci.fi/~benefon/azimalt.cpp
 */
const Angle
SunEphemeris::FNrange(fixed x)
{
  return Angle::radians(x).as_bearing();
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
  fixed fo;
  Angle dfo;

  // Correction: different sign at southern hemisphere
  dfo = Angle::degrees(SUN_DIAMETER / 2 + AIR_REFRACTION);

  if (negative(lat.value_degrees()))
    dfo.flip();

  fo = (declin + dfo).tan() * lat.tan();

  if (fo > fixed(0.99999))
    // to avoid overflow
    fo = fixed_one;

  fo = asin(fo) + fixed_half_pi;

  return Angle::radians(fo);
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
  fixed fi;
  Angle df1;

  // Correction: different sign at southern hemisphere
  df1 = Angle::degrees(fixed(6));

  if (negative(lat.value_degrees()))
    df1.flip();

  fi = (declin + df1).tan() * lat.tan();

  if (fi > fixed(0.99999))
    // to avoid overflow
    fi = fixed_one;

  fi = asin(fi) + fixed_half_pi;

  return Angle::radians(fi);
}

/**
 * Find the ecliptic longitude of the Sun
 * @return The ecliptic longitude of the Sun
 */
Angle
SunEphemeris::GetEclipticLongitude(fixed d, fixed L)
{
  //   mean anomaly of the Sun
  Angle g = Angle::degrees(fixed(357.528) + fixed(.9856003) * d).as_bearing();

  //   Ecliptic longitude of the Sun
  return FNrange(L + Angle::degrees(fixed(1.915)).value_radians() * g.sin() +
                 Angle::degrees(fixed(.02)).value_radians() *
                 (g * fixed_two).sin());
}

Angle
SunEphemeris::GetMeanSunLongitude(fixed d)
{
  // mean longitude of the Sun
  return Angle::degrees(fixed(280.461) + fixed(.9856474) * d).as_bearing();
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
  fixed DaysToJ2000;
  Angle Obliquity, Lambda, Alpha, Delta, L;
  fixed LL, equation, TwilightHours;
  Angle HourAngle, HourAngleTwilight;

  DaysToJ2000 = FNday(date_time.year, date_time.month, date_time.day,
                      fixed(date_time.hour % 24));

  L = GetMeanSunLongitude(DaysToJ2000);

  // Use GetEclipticLongitude to find the ecliptic longitude of the Sun
  Lambda = GetEclipticLongitude(DaysToJ2000, L.value_radians());

  // Obliquity of the ecliptic
  Obliquity = Angle::degrees(fixed(23.439) - fixed(.0000004) * DaysToJ2000);

  // Find the RA and DEC of the Sun
  Alpha = Angle::radians(atan2(Obliquity.cos() * Lambda.sin(), Lambda.cos()));
  Delta = Angle::radians(asin(Obliquity.sin() * Lambda.sin()));

  // Find the Equation of Time in minutes
  // Correction suggested by David Smith
  LL = L.value_radians() - Alpha.value_radians();
  if (L.value_radians() < fixed_pi)
    LL += fixed_two_pi;

  equation = fixed(1440) * (fixed_one - LL / fixed_two_pi);

  HourAngle = GetHourAngle(Location.Latitude, Delta);
  HourAngleTwilight = GetHourAngleTwilight(Location.Latitude, Delta);

  // length of twilight in hours
  TwilightHours = (HourAngleTwilight - HourAngle).value_hours();

  // Conversion of angle to hours and minutes
  DayLength = HourAngle.value_hours() * fixed_two;

  if (DayLength < fixed(0.0001))
    // arctic winter
    DayLength = fixed_zero;

  TimeOfSunRise = fixed(12) - HourAngle.value_hours() + TimeZone
    - Location.Longitude.value_degrees() / 15 + equation / 60;

  if (TimeOfSunRise > fixed(24))
    TimeOfSunRise -= fixed(24);

  TimeOfSunSet = TimeOfSunRise + DayLength;
  TimeOfNoon = TimeOfSunRise + HourAngle.value_hours();

  // morning twilight begin
  MorningTwilight = TimeOfSunRise - TwilightHours;
  // evening twilight end
  EveningTwilight = TimeOfSunSet + TwilightHours;
}
