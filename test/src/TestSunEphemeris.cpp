/* Copyright_License {

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
#include "TestUtil.hpp"

static void
test_times()
{
  const GeoPoint location(Angle::Degrees(7.7061111111111114),
                          Angle::Degrees(51.051944444444445));
  BrokenDateTime dt;
  dt.year = 2010;
  dt.month = 9;
  dt.day = 24;
  dt.hour = 8;
  dt.minute = 21;
  dt.second = 12;

  SunEphemeris::Result sun =
    SunEphemeris::CalcSunTimes(location, dt, RoughTimeDelta::FromHours(2));

  ok1(between(sun.morning_twilight, 6.88, 6.9));
  ok1(between(sun.time_of_noon, 13.3, 13.4));
  ok1(between(sun.time_of_sunset, 19.36, 19.40));
  ok1(between(sun.time_of_sunrise, 7.32, 7.41));
  ok1(between(sun.evening_twilight, 19.81, 19.82));
}

#include <stdio.h>
static void
test_times_southern()
{
  // Benalla
  const GeoPoint location(Angle::Degrees(146.00666666666666),
                          Angle::Degrees(-36.551944444444445));
  BrokenDateTime dt;
  dt.year = 2013;
  dt.month = 2;
  dt.day = 22;
  dt.hour = 8;
  dt.minute = 21;
  dt.second = 12;

  SunEphemeris::Result sun =
    SunEphemeris::CalcSunTimes(location, dt, RoughTimeDelta::FromHours(11));

  ok1(between(sun.time_of_sunrise, 6.91, 6.95));
  ok1(between(sun.time_of_sunset, 20.03, 20.04));
}

static void
test_azimuth()
{
  double test_data1[24] = {
      9.660271,
      28.185910,
      44.824283,
      59.416812,
      72.404059,
      84.420232,
      96.113747,
      108.120524,
      121.080364,
      135.609075,
      152.122820,
      170.453770,
      -170.455922,
      -152.140313,
      -135.650092,
      -121.146412,
      -108.210341,
      -96.225746,
      -84.552904,
      -72.554841,
      -59.579269,
      -44.983118,
      -28.311449,
      -9.711208
  };

  double test_data2[24] = {
      135.535117,
      121.014248,
      108.070607,
      96.083211,
      84.410044,
      72.414117,
      59.445455,
      44.866010,
      28.227658,
      9.680160,
      -9.682373,
      -28.245579,
      -44.907783,
      -59.512321,
      -72.504563,
      -84.522294,
      -96.215579,
      -108.220331,
      -121.174709,
      -135.691072,
      -152.181075,
      -170.475266,
      170.477364,
      152.198463
  };

  GeoPoint location(Angle::Degrees(7), Angle::Degrees(51));

  BrokenDateTime dt;
  dt.year = 2010;
  dt.month = 9;
  dt.day = 24;
  dt.minute = 30;
  dt.second = 0;

  for (unsigned hour = 0; hour < 24; hour++) {
    dt.hour = hour;

    SunEphemeris::Result sun =
      SunEphemeris::CalcSunTimes(location, dt, RoughTimeDelta::FromHours(0));

    ok1(equals(sun.azimuth, test_data1[hour]));
  }

  location.latitude.Flip();

  for (unsigned hour = 0; hour < 24; hour++) {
    dt.hour = hour;

    SunEphemeris::Result sun =
      SunEphemeris::CalcSunTimes(location, dt, RoughTimeDelta::FromHours(2));

    ok1(equals(sun.azimuth, test_data2[hour]));
  }
}

int main(int argc, char **argv)
{
  plan_tests(55);

  test_times();
  test_times_southern();
  test_azimuth();

  return exit_status();
}
