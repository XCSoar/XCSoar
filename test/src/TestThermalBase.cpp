// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Computer/ThermalBase.hpp"
#include "TestUtil.hpp"
#include "Geo/SpeedVector.hpp"

int main()
{
  plan_tests(3);

  GeoPoint location(Angle::Degrees(7), Angle::Degrees(45));
  double altitude(1300);
  double average(2.5);
  SpeedVector wind(Angle::Degrees(60), 20);

  GeoPoint ground_location(Angle::Zero(), Angle::Zero());
  double ground_alt;

  EstimateThermalBase(nullptr, location, altitude, average, wind,
                      ground_location, ground_alt);

  ok1(equals(ground_location.longitude.Degrees(), 7.114186));
  ok1(equals(ground_location.latitude.Degrees(), 45.046563));
  ok1(equals(ground_alt, 0));

  return exit_status();
}
