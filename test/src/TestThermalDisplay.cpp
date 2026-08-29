// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Geo/Math.hpp"
#include "Geo/SpeedVector.hpp"
#include "MapWindow/ThermalDisplay.hpp"
#include "NMEA/ThermalLocator.hpp"
#include "TestUtil.hpp"

int
main()
{
  plan_tests(9);

  ok1(ThermalDisplay::IsVisible(4000));
  ok1(!ThermalDisplay::IsVisible(4000.001));
  ok1(ThermalDisplay::IsTrafficVisible(true, 4000));
  ok1(!ThermalDisplay::IsTrafficVisible(true, 4001));
  ok1(!ThermalDisplay::IsTrafficVisible(false, 1000));

  const GeoPoint location{Angle::Degrees(7), Angle::Degrees(45)};
  ThermalSource source{};
  source.location = location;
  source.ground_height = 1000;
  source.lift_rate = 2;

  const SpeedVector wind{Angle::Degrees(90), 10};
  ok1(!ThermalDisplay::GetLocation(source, 999, wind).IsValid());
  ok1(equals(ThermalDisplay::GetLocation(source, 1000, wind), location));

  const auto expected =
    FindLatitudeLongitude(location, Angle::Degrees(270), 500);
  ok1(equals(ThermalDisplay::GetLocation(source, 1100, wind), expected));
  ok1(equals(ThermalDisplay::GetLocation(source, 1100,
                                         SpeedVector::Zero()),
              location));

  return exit_status();
}
