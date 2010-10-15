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
  const fixed b = x / fixed_two_pi;
  fixed a = fixed_two_pi * (b - trunc(b));

  return Angle::radians(a).as_bearing();
}

/**
 * Calculating the hourangle
 * @param lat Latitude
 * @param declin Declination
 * @return The hourangle
 */
// TODO TB: find explanations/links for this and following
fixed
SunEphemeris::f0(Angle lat, fixed declin)
{
  fixed fo, dfo;

  // Correction: different sign at southern hemisphere
  dfo = fixed_deg_to_rad * (SUN_DIAMETER / 2 + AIR_REFRACTION);

  if (negative(lat.value_degrees()))
    dfo = -dfo;

  fo = tan(declin + dfo) * tan(lat.value_degrees() * fixed_deg_to_rad);

  if (fo > fixed(0.99999))
    // to avoid overflow
    fo = fixed_one;

  fo = asin(fo) + fixed_half_pi;

  return fo;
}

/**
 * Calculating the hourangle for twilight times
 * @param lat Latitude
 * @param declin Declination
 * @return The hourangle for twilight times
 */
fixed
SunEphemeris::f1(Angle lat, fixed declin)
{
  fixed fi, df1;

  // Correction: different sign at southern hemisphere
  df1 = fixed_deg_to_rad * 6;

  if (negative(lat.value_degrees()))
    df1 = -df1;

  fi = tan(declin + df1) * tan(lat.value_degrees() * fixed_deg_to_rad);

  if (fi > fixed(0.99999))
    // to avoid overflow
    fi = fixed_one;

  fi = asin(fi) + fixed_half_pi;

  return fi;
}

/**
 * Find the ecliptic longitude of the Sun
 * @return The ecliptic longitude of the Sun
 */
fixed
SunEphemeris::GetEclipticLongitude(fixed d)
{
  //   mean longitude of the Sun
  L = FNrange(fixed_deg_to_rad * (fixed(280.461) +
                                  fixed(.9856474) * d)).value_radians();

  //   mean anomaly of the Sun
  g = FNrange(fixed_deg_to_rad * (fixed(357.528) +
                                  fixed(.9856003) * d)).value_radians();

  //   Ecliptic longitude of the Sun
  return FNrange(L + fixed(1.915) * fixed_deg_to_rad * sin(g) +
                 fixed(.02) * fixed_deg_to_rad * sin(2 * g)).value_radians();
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
fixed
SunEphemeris::CalcSunTimes(const GeoPoint &Location,
                           const BrokenDateTime &date_time,
                           const fixed TimeZone)
{
  //float intz;
  fixed DaysToJ2000, Lambda;
  Angle Obliquity;
  fixed Alpha, Delta, LL, equation, HourAngle, HourAngleTwilight, TwilightHours;
  int Year, Month, Day, Hour;

  // testing

  // JG Removed simulator conditional code, since GPS_INFO now set up
  // from system time.

  Month = date_time.month;
  Year = date_time.year;
  Day = date_time.day;
  Hour = date_time.hour % 24;

  DaysToJ2000 = FNday(Year, Month, Day, fixed(Hour));

  // Use GetEclipticLongitude to find the ecliptic longitude of the Sun
  Lambda = GetEclipticLongitude(DaysToJ2000);

  // Obliquity of the ecliptic
  Obliquity = Angle::degrees(fixed(23.439) - fixed(.0000004) * DaysToJ2000);

  // Find the RA and DEC of the Sun
  Alpha = atan2(Obliquity.cos() * sin(Lambda), cos(Lambda));
  Delta = asin(Obliquity.sin() * sin(Lambda));

  // Find the Equation of Time in minutes
  // Correction suggested by David Smith
  LL = L - Alpha;
  if (L < fixed_pi)
    LL += fixed_two_pi;

  equation = fixed(1440) * (fixed_one - LL / fixed_pi / 2);

  HourAngle = f0(Location.Latitude, Delta);
  HourAngleTwilight = f1(Location.Latitude, Delta);

  // length of twilight in radians
  TwilightHours = HourAngleTwilight - HourAngle;
  // length of twilight in hours
  TwilightHours = 12 * TwilightHours / fixed_pi;

  //printf("ha= %.2f   hb= %.2f \n",ha,hb);

  // Conversion of angle to hours and minutes
  DayLength = fixed_rad_to_deg * HourAngle / fixed(7.5);

  if (DayLength < fixed(0.0001))
    // arctic winter
    DayLength = fixed_zero;

  TimeOfSunRise = fixed(12) - fixed(12) * HourAngle / fixed_pi + TimeZone
    - Location.Longitude.value_degrees() / 15 + equation / 60;

  TimeOfSunSet = fixed(12) + fixed(12) * HourAngle / fixed_pi + TimeZone
    - Location.Longitude.value_degrees() / 15 + equation / 60;

  TimeOfNoon = TimeOfSunRise + fixed(12) * HourAngle / fixed_pi;
  altmax = fixed(90) + Delta * fixed_rad_to_deg - Location.Latitude.value_degrees();

  // Correction for southern hemisphere suggested by David Smith
  // to express altitude as degrees from the N horizon
  if (Location.Latitude.value_degrees() < Delta * fixed_rad_to_deg)
    altmax = fixed(180) - altmax;

  // morning twilight begin
  MorningTwilight = TimeOfSunRise - TwilightHours;
  // evening twilight end
  EveningTwilight = TimeOfSunSet + TwilightHours;

  if (TimeOfSunRise > fixed(24))
    TimeOfSunRise -= fixed(24);

  if (TimeOfSunSet > fixed(24))
    TimeOfSunSet -= fixed(24);

  // return TimeOfSunSet since this is most commonly what is requested
  return TimeOfSunSet;
}
