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
#include "Math/Constants.h"
#include "NMEA/Info.h"
#include "NMEA/Derived.hpp"

// Sun radius in degrees (?)
#define SUN_DIAMETER 0.53
// Atmospheric refraction degrees
#define AIR_REFRACTION 34.0/60.0

#include <math.h>

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
double
SunEphemeris::FNday(int y, int m, int d, float h)
{
  long int luku = -7 * (y + (m + 9) / 12) / 4 + 275 * m / 9 + d;
  // type casting necessary on PC DOS and TClite to avoid overflow
  luku += (long int)y * 367;

  return (double)luku - 730531.5 + h / 24.0;
}

/**
 * The function below returns an angle in the range 0 to 2*PI
 * @param x Angle to be converted
 * @return an angle in the range 0 to 2*PI
 * @see http://www.sci.fi/~benefon/azimalt.cpp
 */
double
SunEphemeris::FNrange(double x)
{
  // QUESTION TB: DEG_TO_RAD?!

  double b = 0.5 * x / M_PI;
  double a = 2.0 * M_PI * (b - (long)(b));

  if (a < 0)
    a = 2.0 * M_PI + a;

  return a;
}

/**
 * Calculating the hourangle
 * @param lat Latitude
 * @param declin Declination
 * @return The hourangle
 */
// TODO TB: find explanations/links for this and following
double
SunEphemeris::f0(double lat, double declin)
{
  double fo, dfo;

  // Correction: different sign at S HS
  dfo = DEG_TO_RAD * (0.5 * SUN_DIAMETER + AIR_REFRACTION);

  if (lat < 0.0)
    dfo = -dfo;

  fo = tan(declin + dfo) * tan(lat * DEG_TO_RAD);

  if (fo > 0.99999)
    // to avoid overflow
    fo = 1.0;

  fo = asin(fo) + M_PI / 2.0;

  return fo;
}

/**
 * Calculating the hourangle for twilight times
 * @param lat Latitude
 * @param declin Declination
 * @return The hourangle for twilight times
 */
double
SunEphemeris::f1(double lat, double declin)
{
  double fi, df1;

  // Correction: different sign at S HS
  df1 = DEG_TO_RAD * 6.0;

  if (lat < 0.0)
    df1 = -df1;

  fi = tan(declin + df1) * tan(lat * DEG_TO_RAD);

  if (fi > 0.99999)
    // to avoid overflow
    fi = 1.0;

  fi = asin(fi) + M_PI / 2.0;

  return fi;
}

/**
 * Find the ecliptic longitude of the Sun
 * @return The ecliptic longitude of the Sun
 */
double
SunEphemeris::FNsun(double d)
{
  //   mean longitude of the Sun
  L = FNrange(280.461 * DEG_TO_RAD + .9856474 * DEG_TO_RAD * d);

  //   mean anomaly of the Sun
  g = FNrange(357.528 * DEG_TO_RAD + .9856003 * DEG_TO_RAD * d);

  //   Ecliptic longitude of the Sun
  return FNrange(L + 1.915 * DEG_TO_RAD * sin(g) + .02 * DEG_TO_RAD
      * sin(2 * g));
}

/**
 * Calculates all sun-related important times
 * depending on time of year and location
 * @param location Location to be used in calculation
 * @param Basic NMEA_INFO for current date
 * @param Calculated DERIVED_INFO (not yet used)
 * @param TimeZone The timezone
 * @return Always 0
 */
int
SunEphemeris::CalcSunTimes(const GEOPOINT &Location, const NMEA_INFO &Basic,
    const DERIVED_INFO &Calculated, const double TimeZone)
{
  (void)Calculated;

  //float intz;
  double d, lambda;
  double obliq, alpha, delta, LL, equation, ha, hb, twx;
  int Year, Month, Day, Hour;

  // testing

  // JG Removed simulator conditional code, since GPS_INFO now set up
  // from system time.

  Month = Basic.Month;
  Year = Basic.Year;
  Day = Basic.Day;
  Hour = ((int)Basic.Time) / 3600;
  Hour = (Hour % 24);

  d = FNday(Year, Month, Day, (float)Hour);

  // Use FNsun to find the ecliptic longitude of the Sun
  lambda = FNsun(d);

  // Obliquity of the ecliptic
  obliq = 23.439 * DEG_TO_RAD - .0000004 * DEG_TO_RAD * d;

  //   Find the RA and DEC of the Sun
  alpha = atan2(cos(obliq) * sin(lambda), cos(lambda));
  delta = asin(sin(obliq) * sin(lambda));

  // Find the Equation of Time in minutes
  // Correction suggested by David Smith
  LL = L - alpha;
  if (L < M_PI)
    LL += 2.0 * M_PI;
  equation = 1440.0 * (1.0 - LL / M_PI / 2.0);
  ha = f0(Location.Latitude, delta);
  hb = f1(Location.Latitude, delta);
  // length of twilight in radians
  twx = hb - ha;
  // length of twilight in hours
  twx = 12.0 * twx / M_PI;

  //printf("ha= %.2f   hb= %.2f \n",ha,hb);

  // Conversion of angle to hours and minutes
  daylen = RAD_TO_DEG * ha / 7.5;

  if (daylen < 0.0001)
    // arctic winter
    daylen = 0.0;

  riset = 12.0 - 12.0 * ha / M_PI + TimeZone - Location.Longitude / 15.0
      + equation / 60.0;

  settm = 12.0 + 12.0 * ha / M_PI + TimeZone - Location.Longitude / 15.0
      + equation / 60.0;

  noont = riset + 12.0 * ha / M_PI;
  altmax = 90.0 + delta * RAD_TO_DEG - Location.Latitude;

  // Correction for S HS suggested by David Smith
  // to express altitude as degrees from the N horizon
  if (Location.Latitude < delta * RAD_TO_DEG)
    altmax = 180.0 - altmax;

  // morning twilight begin
  twam = riset - twx;
  // evening twilight end
  twpm = settm + twx;

  if (riset > 24.0)
    riset -= 24.0;

  if (settm > 24.0)
    settm -= 24.0;

  /*
      puts("\n Sunrise and set");
      puts("===============");

      printf("  year  : %d \n",(int)y);
      printf("  month : %d \n",(int)m);
      printf("  day   : %d \n\n",(int)day);
      printf("Days since Y2K :  %d \n",(int)d);

      printf("Latitude :  %3.1f, longitude: %3.1f, timezone: %3.1f \n",(float)latit,(float)longit,(float)tzone);
      printf("Declination   :  %.2f \n",delta * RAD_TO_DEG);
      printf("Daylength     : "); showhrmn(daylen); puts(" hours \n");
      printf("Civil twilight: ");
      showhrmn(twam); puts("");
      printf("Sunrise       : ");
      showhrmn(riset); puts("");

      printf("Sun altitude ");
      // Amendment by D. Smith
      printf(" %.2f degr",altmax);
      printf(latit>=0.0 ? " South" : " North");
      printf(" at noontime "); showhrmn(noont); puts("");
      printf("Sunset        : ");
      showhrmn(settm);  puts("");
      printf("Civil twilight: ");
      showhrmn(twpm);  puts("\n");
  */
  // QUESTION TB: why not just void?

  return 0;
}
