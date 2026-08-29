// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Geo/Math.hpp"
#include "Geo/SpeedVector.hpp"
#include "NMEA/ThermalLocator.hpp"
#include "NMEA/ThermalProjection.hpp"
#include "TestUtil.hpp"

#include <limits>

static constexpr GeoPoint TEST_CENTRE = {
  Angle::Degrees(7), Angle::Degrees(45),
};

int
main()
{
  plan_tests(14);

  const SpeedVector wind{Angle::Degrees(90), 10};
  const auto drift_per_meter = CalculateThermalDriftPerMeter(wind, 2);
  ok1(equals(drift_per_meter.norm, 5));
  ok1(equals(drift_per_meter.bearing, 90));

  ok1(equals(ProjectThermalCore(TEST_CENTRE, 0, drift_per_meter),
              TEST_CENTRE));

  const auto expected_upwind =
    FindLatitudeLongitude(TEST_CENTRE, Angle::Degrees(270), 500);
  const auto expected_downwind =
    FindLatitudeLongitude(TEST_CENTRE, Angle::Degrees(90), 500);
  ok1(equals(ProjectThermalCore(TEST_CENTRE, 100, drift_per_meter),
              expected_upwind));
  ok1(equals(ProjectThermalCore(TEST_CENTRE, -100, drift_per_meter),
              expected_downwind));

  ok1(equals(ProjectThermalCore(TEST_CENTRE, 100,
                                SpeedVector::Zero(), 2),
              TEST_CENTRE));
  ok1(equals(ProjectThermalCore(TEST_CENTRE, 100, wind, 0),
              TEST_CENTRE));
  ok1(equals(ProjectThermalCore(TEST_CENTRE, 100, wind, -1),
              TEST_CENTRE));

  const double nan = std::numeric_limits<double>::quiet_NaN();
  ok1(equals(ProjectThermalCore(TEST_CENTRE, 100, wind, nan),
              TEST_CENTRE));
  ok1(equals(ProjectThermalCore(
                TEST_CENTRE, 100,
                SpeedVector{Angle::Degrees(90), nan}, 2),
              TEST_CENTRE));
  ok1(equals(ProjectThermalCore(
                TEST_CENTRE, 100,
                SpeedVector{Angle::Native(nan), 10}, 2),
              TEST_CENTRE));
  ok1(equals(ProjectThermalCore(
                TEST_CENTRE, 100,
                SpeedVector{Angle::Degrees(90), nan}),
              TEST_CENTRE));
  ok1(!ProjectThermalCore(GeoPoint::Invalid(), 100, drift_per_meter)
        .IsValid());

  const auto projected = ProjectThermalCore(TEST_CENTRE, 200, wind, 2);
  ThermalSource source{};
  source.location = TEST_CENTRE;
  source.ground_height = 1000;
  source.lift_rate = 2;
  ok1(equals(source.CalculateAdjustedLocation(1200, wind), projected));

  return exit_status();
}
