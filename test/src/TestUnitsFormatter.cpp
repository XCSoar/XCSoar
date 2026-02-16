// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Formatter/Units.hpp"
#include "Units/System.hpp"
#include "Units/Units.hpp"
#include "util/Macros.hpp"
#include "util/StringAPI.hxx"
#include "Atmosphere/Pressure.hpp"
#include "TestUtil.hpp"

static void
TestAltitude()
{
  char buffer[256];

  // Test FormatAltitude()
  FormatAltitude(buffer, 1234, Unit::METER);
  ok1(StringIsEqual(buffer, _T("1234 m")));

  FormatAltitude(buffer, Units::ToSysUnit(1234, Unit::FEET), Unit::FEET);
  ok1(StringIsEqual(buffer, _T("1234 ft")));

  FormatAltitude(buffer, -1234, Unit::METER);
  ok1(StringIsEqual(buffer, _T("-1234 m")));

  FormatAltitude(buffer, Units::ToSysUnit(-1234, Unit::FEET), Unit::FEET);
  ok1(StringIsEqual(buffer, _T("-1234 ft")));

  FormatAltitude(buffer, 1234, Unit::METER, false);
  ok1(StringIsEqual(buffer, _T("1234")));

  FormatAltitude(buffer, -1234, Unit::METER, false);
  ok1(StringIsEqual(buffer, _T("-1234")));
}

static void
TestRelativeAltitude()
{
  char buffer[256];

  // Test FormatRelativeAltitude()
  FormatRelativeAltitude(buffer, 1234, Unit::METER);
  ok1(StringIsEqual(buffer, _T("+1234 m")));

  FormatRelativeAltitude(buffer, Units::ToSysUnit(1234, Unit::FEET),
                         Unit::FEET);
  ok1(StringIsEqual(buffer, _T("+1234 ft")));

  FormatRelativeAltitude(buffer, -1234, Unit::METER);
  ok1(StringIsEqual(buffer, _T("-1234 m")));

  FormatRelativeAltitude(buffer, Units::ToSysUnit(-1234, Unit::FEET),
                         Unit::FEET);
  ok1(StringIsEqual(buffer, _T("-1234 ft")));

  FormatRelativeAltitude(buffer, 1234, Unit::METER, false);
  ok1(StringIsEqual(buffer, _T("+1234")));

  FormatRelativeAltitude(buffer, -1234, Unit::METER, false);
  ok1(StringIsEqual(buffer, _T("-1234")));
}

static void
TestDistance()
{
  char buffer[256];

  // Test FormatDistance()
  FormatDistance(buffer, 123.4, Unit::METER);
  ok1(StringIsEqual(buffer, _T("123 m")));

  FormatDistance(buffer, 123.4, Unit::METER, false);
  ok1(StringIsEqual(buffer, _T("123")));

  FormatDistance(buffer, 123.4, Unit::METER, true, 1);
  ok1(StringIsEqual(buffer, _T("123.4 m")));

  FormatDistance(buffer, 123.4, Unit::METER, false, 1);
  ok1(StringIsEqual(buffer, _T("123.4")));

  FormatDistance(buffer, 123.4, Unit::METER, true, 2);
  ok1(StringIsEqual(buffer, _T("123.40 m")));

  FormatDistance(buffer, 123.4, Unit::METER, false, 2);
  ok1(StringIsEqual(buffer, _T("123.40")));

  FormatDistance(buffer, Units::ToSysUnit(123.4, Unit::KILOMETER),
                 Unit::KILOMETER);
  ok1(StringIsEqual(buffer, _T("123 km")));

  FormatDistance(buffer, Units::ToSysUnit(123.4, Unit::KILOMETER),
                 Unit::KILOMETER, false);
  ok1(StringIsEqual(buffer, _T("123")));

  FormatDistance(buffer, Units::ToSysUnit(123.4, Unit::KILOMETER),
                 Unit::KILOMETER, true, 1);
  ok1(StringIsEqual(buffer, _T("123.4 km")));

  FormatDistance(buffer, Units::ToSysUnit(123.4, Unit::KILOMETER),
                 Unit::KILOMETER, false, 1);
  ok1(StringIsEqual(buffer, _T("123.4")));

  FormatDistance(buffer, Units::ToSysUnit(123.4, Unit::NAUTICAL_MILES),
                 Unit::NAUTICAL_MILES);
  ok1(StringIsEqual(buffer, _T("123 NM")));

  FormatDistance(buffer, Units::ToSysUnit(123.4, Unit::NAUTICAL_MILES),
                 Unit::NAUTICAL_MILES, false);
  ok1(StringIsEqual(buffer, _T("123")));

  FormatDistance(buffer, Units::ToSysUnit(123.4, Unit::NAUTICAL_MILES),
                 Unit::NAUTICAL_MILES, true, 1);
  ok1(StringIsEqual(buffer, _T("123.4 NM")));

  FormatDistance(buffer, Units::ToSysUnit(123.4, Unit::NAUTICAL_MILES),
                 Unit::NAUTICAL_MILES, false, 1);
  ok1(StringIsEqual(buffer, _T("123.4")));

  FormatDistance(buffer, Units::ToSysUnit(123.4, Unit::STATUTE_MILES),
                 Unit::STATUTE_MILES);
  ok1(StringIsEqual(buffer, _T("123 mi")));

  FormatDistance(buffer, Units::ToSysUnit(123.4, Unit::STATUTE_MILES),
                 Unit::STATUTE_MILES, false);
  ok1(StringIsEqual(buffer, _T("123")));

  FormatDistance(buffer, Units::ToSysUnit(123.4, Unit::STATUTE_MILES),
                 Unit::STATUTE_MILES, true, 1);
  ok1(StringIsEqual(buffer, _T("123.4 mi")));

  FormatDistance(buffer, Units::ToSysUnit(123.4, Unit::STATUTE_MILES),
                 Unit::STATUTE_MILES, false, 1);
  ok1(StringIsEqual(buffer, _T("123.4")));
}

static void
TestSmallDistance()
{
  char buffer[256];

  // Test FormatSmallDistance()
  FormatSmallDistance(buffer, 123.4, Unit::METER);
  ok1(StringIsEqual(buffer, _T("123 m")));

  FormatSmallDistance(buffer, 123.4, Unit::METER, false);
  ok1(StringIsEqual(buffer, _T("123")));

  FormatSmallDistance(buffer, 123.4, Unit::METER, true, 1);
  ok1(StringIsEqual(buffer, _T("123.4 m")));

  FormatSmallDistance(buffer, 123.4, Unit::METER, false, 1);
  ok1(StringIsEqual(buffer, _T("123.4")));

  FormatSmallDistance(buffer, 123.4, Unit::METER, true, 2);
  ok1(StringIsEqual(buffer, _T("123.40 m")));

  FormatSmallDistance(buffer, 123.4, Unit::METER, false, 2);
  ok1(StringIsEqual(buffer, _T("123.40")));

  FormatSmallDistance(buffer, 123.4, Unit::KILOMETER);
  ok1(StringIsEqual(buffer, _T("123 m")));

  FormatSmallDistance(buffer, 123.4, Unit::KILOMETER, false);
  ok1(StringIsEqual(buffer, _T("123")));

  FormatSmallDistance(buffer, 123.4, Unit::KILOMETER, true, 1);
  ok1(StringIsEqual(buffer, _T("123.4 m")));

  FormatSmallDistance(buffer, 123.4, Unit::KILOMETER, false, 1);
  ok1(StringIsEqual(buffer, _T("123.4")));

  FormatSmallDistance(buffer, Units::ToSysUnit(123.4, Unit::FEET),
                      Unit::NAUTICAL_MILES);
  ok1(StringIsEqual(buffer, _T("123 ft")));

  FormatSmallDistance(buffer, Units::ToSysUnit(123.4, Unit::FEET),
                      Unit::NAUTICAL_MILES, false);
  ok1(StringIsEqual(buffer, _T("123")));

  FormatSmallDistance(buffer, Units::ToSysUnit(123.4, Unit::FEET),
                      Unit::NAUTICAL_MILES, true, 1);
  ok1(StringIsEqual(buffer, _T("123.4 ft")));

  FormatSmallDistance(buffer, Units::ToSysUnit(123.4, Unit::FEET),
                      Unit::NAUTICAL_MILES, false, 1);
  ok1(StringIsEqual(buffer, _T("123.4")));

  FormatSmallDistance(buffer, Units::ToSysUnit(123.4, Unit::FEET),
                      Unit::STATUTE_MILES);
  ok1(StringIsEqual(buffer, _T("123 ft")));

  FormatSmallDistance(buffer, Units::ToSysUnit(123.4, Unit::FEET),
                      Unit::STATUTE_MILES, false);
  ok1(StringIsEqual(buffer, _T("123")));

  FormatSmallDistance(buffer, Units::ToSysUnit(123.4, Unit::FEET),
                      Unit::STATUTE_MILES, true, 1);
  ok1(StringIsEqual(buffer, _T("123.4 ft")));

  FormatSmallDistance(buffer, Units::ToSysUnit(123.4, Unit::FEET),
                      Unit::STATUTE_MILES, false, 1);
  ok1(StringIsEqual(buffer, _T("123.4")));
}

static void
TestDistanceSmart(double value, Unit unit, Unit expected_unit,
                  const char *expected_output_with_unit,
                  const char *expected_output_without_unit,
                  double small_unit_threshold = 2500,
                  double precision_threshold = 100)
{
  char buffer[256];

  ok1(FormatDistanceSmart(buffer, value, unit, true, small_unit_threshold,
                          precision_threshold) == expected_unit);
  ok1(StringIsEqual(buffer, expected_output_with_unit));

  ok1(FormatDistanceSmart(buffer, value, unit, false, small_unit_threshold,
                          precision_threshold) == expected_unit);
  ok1(StringIsEqual(buffer, expected_output_without_unit));
}

static void
TestDistanceSmart()
{
  // Test FormatDistanceSmart()

  // Test Meter
  TestDistanceSmart(0.1234, Unit::METER, Unit::METER, _T("0.12 m"),
                    _T("0.12"));

  TestDistanceSmart(1.234, Unit::METER, Unit::METER, _T("1.23 m"),
                    _T("1.23"));

  TestDistanceSmart(12.34, Unit::METER, Unit::METER, _T("12.3 m"),
                    _T("12.3"));

  TestDistanceSmart(123.4, Unit::METER, Unit::METER, _T("123 m"),
                    _T("123"));

  TestDistanceSmart(1234, Unit::METER, Unit::METER, _T("1234 m"),
                    _T("1234"));

  TestDistanceSmart(12345, Unit::METER, Unit::METER, _T("12345 m"),
                    _T("12345"));

  TestDistanceSmart(123456, Unit::METER, Unit::METER, _T("123456 m"),
                    _T("123456"));

  // Test Kilometer
  TestDistanceSmart(Units::ToSysUnit(0.1234, Unit::KILOMETER),
                    Unit::KILOMETER, Unit::METER, _T("123 m"), _T("123"));

  TestDistanceSmart(Units::ToSysUnit(1.234, Unit::KILOMETER),
                    Unit::KILOMETER, Unit::METER, _T("1234 m"), _T("1234"));

  TestDistanceSmart(Units::ToSysUnit(2.345, Unit::KILOMETER),
                    Unit::KILOMETER, Unit::METER, _T("2345 m"), _T("2345"));

  TestDistanceSmart(Units::ToSysUnit(2.634, Unit::KILOMETER),
                    Unit::KILOMETER, Unit::KILOMETER, _T("2.63 km"),
                    _T("2.63"));

  TestDistanceSmart(Units::ToSysUnit(12.34, Unit::KILOMETER),
                    Unit::KILOMETER, Unit::KILOMETER, _T("12.3 km"),
                    _T("12.3"));

  TestDistanceSmart(Units::ToSysUnit(123.4, Unit::KILOMETER),
                    Unit::KILOMETER, Unit::KILOMETER, _T("123 km"), _T("123"));

  // Test Nautical Miles
  TestDistanceSmart(Units::ToSysUnit(123.4, Unit::FEET),
                    Unit::NAUTICAL_MILES, Unit::FEET, _T("123 ft"), _T("123"));

  TestDistanceSmart(Units::ToSysUnit(1234, Unit::FEET),
                    Unit::NAUTICAL_MILES, Unit::FEET, _T("1234 ft"),
                    _T("1234"));

  TestDistanceSmart(Units::ToSysUnit(2345, Unit::FEET),
                    Unit::NAUTICAL_MILES, Unit::FEET, _T("2345 ft"),
                    _T("2345"));

  TestDistanceSmart(Units::ToSysUnit(0.61, Unit::NAUTICAL_MILES),
                    Unit::NAUTICAL_MILES, Unit::NAUTICAL_MILES, _T("0.61 NM"),
                    _T("0.61"));

  TestDistanceSmart(Units::ToSysUnit(1.234, Unit::NAUTICAL_MILES),
                    Unit::NAUTICAL_MILES, Unit::NAUTICAL_MILES, _T("1.23 NM"),
                    _T("1.23"));

  TestDistanceSmart(Units::ToSysUnit(12.34, Unit::NAUTICAL_MILES),
                    Unit::NAUTICAL_MILES, Unit::NAUTICAL_MILES, _T("12.3 NM"),
                    _T("12.3"));

  TestDistanceSmart(Units::ToSysUnit(123.4, Unit::NAUTICAL_MILES),
                    Unit::NAUTICAL_MILES, Unit::NAUTICAL_MILES, _T("123 NM"),
                    _T("123"));

  // Test Statute Miles
  TestDistanceSmart(Units::ToSysUnit(123.4, Unit::FEET),
                    Unit::STATUTE_MILES, Unit::FEET, _T("123 ft"), _T("123"));

  TestDistanceSmart(Units::ToSysUnit(1234, Unit::FEET),
                    Unit::STATUTE_MILES, Unit::FEET, _T("1234 ft"), _T("1234"));

  TestDistanceSmart(Units::ToSysUnit(2345, Unit::FEET),
                    Unit::STATUTE_MILES, Unit::FEET, _T("2345 ft"), _T("2345"));

  TestDistanceSmart(Units::ToSysUnit(0.71, Unit::STATUTE_MILES),
                    Unit::STATUTE_MILES, Unit::STATUTE_MILES, _T("0.71 mi"),
                    _T("0.71"));

  TestDistanceSmart(Units::ToSysUnit(1.234, Unit::STATUTE_MILES),
                    Unit::STATUTE_MILES, Unit::STATUTE_MILES, _T("1.23 mi"),
                    _T("1.23"));

  TestDistanceSmart(Units::ToSysUnit(12.34, Unit::STATUTE_MILES),
                    Unit::STATUTE_MILES, Unit::STATUTE_MILES, _T("12.3 mi"),
                    _T("12.3"));

  TestDistanceSmart(Units::ToSysUnit(123.4, Unit::STATUTE_MILES),
                    Unit::STATUTE_MILES, Unit::STATUTE_MILES, _T("123 mi"),
                    _T("123"));

  // Test thresholds
  TestDistanceSmart(Units::ToSysUnit(0.9, Unit::KILOMETER),
                    Unit::KILOMETER, Unit::METER, _T("900 m"), _T("900"), 1000);

  TestDistanceSmart(Units::ToSysUnit(1.1, Unit::KILOMETER),
                    Unit::KILOMETER, Unit::KILOMETER, _T("1.10 km"), _T("1.10"), 1000);

  TestDistanceSmart(Units::ToSysUnit(1.1, Unit::KILOMETER),
                    Unit::KILOMETER, Unit::KILOMETER, _T("1.1 km"), _T("1.1"), 1000, 10);

  TestDistanceSmart(Units::ToSysUnit(1.1, Unit::KILOMETER),
                    Unit::KILOMETER, Unit::KILOMETER, _T("1 km"), _T("1"), 1000, 1);

}

static void
TestSpeed()
{
  char buffer[256];

  // Test FormatSpeed()
  FormatSpeed(buffer, 23.46, Unit::METER_PER_SECOND);
  ok1(StringIsEqual(buffer, _T("23 m/s")));

  FormatSpeed(buffer, 23.46, Unit::METER_PER_SECOND, true, true);
  ok1(StringIsEqual(buffer, _T("23.5 m/s")));

  FormatSpeed(buffer, 23.46, Unit::METER_PER_SECOND, false);
  ok1(StringIsEqual(buffer, _T("23")));

  FormatSpeed(buffer, 23.46, Unit::METER_PER_SECOND, false, true);
  ok1(StringIsEqual(buffer, _T("23.5")));

  FormatSpeed(buffer, Units::ToSysUnit(123.43, Unit::KILOMETER_PER_HOUR),
              Unit::KILOMETER_PER_HOUR);
  ok1(StringIsEqual(buffer, _T("123 km/h")));

  FormatSpeed(buffer, Units::ToSysUnit(123.43, Unit::KILOMETER_PER_HOUR),
              Unit::KILOMETER_PER_HOUR, true, true);
  ok1(StringIsEqual(buffer, _T("123 km/h")));

  FormatSpeed(buffer, Units::ToSysUnit(83.43, Unit::KILOMETER_PER_HOUR),
              Unit::KILOMETER_PER_HOUR, true, true);
  ok1(StringIsEqual(buffer, _T("83.4 km/h")));

  FormatSpeed(buffer, Units::ToSysUnit(123.43, Unit::KNOTS),
              Unit::KNOTS);
  ok1(StringIsEqual(buffer, _T("123 kt")));

  FormatSpeed(buffer, Units::ToSysUnit(123.43, Unit::KNOTS), Unit::KNOTS,
              true, true);
  ok1(StringIsEqual(buffer, _T("123 kt")));

  FormatSpeed(buffer, Units::ToSysUnit(83.43, Unit::KNOTS), Unit::KNOTS,
              true, true);
  ok1(StringIsEqual(buffer, _T("83.4 kt")));

  FormatSpeed(buffer,
              Units::ToSysUnit(123.43, Unit::STATUTE_MILES_PER_HOUR),
              Unit::STATUTE_MILES_PER_HOUR);
  ok1(StringIsEqual(buffer, _T("123 mph")));

  FormatSpeed(buffer,
              Units::ToSysUnit(123.43, Unit::STATUTE_MILES_PER_HOUR),
              Unit::STATUTE_MILES_PER_HOUR, true, true);
  ok1(StringIsEqual(buffer, _T("123 mph")));

  FormatSpeed(buffer,
              Units::ToSysUnit(83.43, Unit::STATUTE_MILES_PER_HOUR),
              Unit::STATUTE_MILES_PER_HOUR, true, true);
  ok1(StringIsEqual(buffer, _T("83.4 mph")));
}

static void
TestVerticalSpeed()
{
  char buffer[256];

  // Test FormatVerticalSpeed()
  FormatVerticalSpeed(buffer, 1.42, Unit::METER_PER_SECOND);
  ok1(StringIsEqual(buffer, _T("+1.4 m/s")));

  FormatVerticalSpeed(buffer, 1.42, Unit::METER_PER_SECOND, false);
  ok1(StringIsEqual(buffer, _T("+1.4")));

  FormatVerticalSpeed(buffer, Units::ToSysUnit(2.47, Unit::KNOTS),
                      Unit::KNOTS);
  ok1(StringIsEqual(buffer, _T("+2.5 kt")));

  FormatVerticalSpeed(buffer, Units::ToSysUnit(2.47, Unit::KNOTS),
                      Unit::KNOTS, false);
  ok1(StringIsEqual(buffer, _T("+2.5")));

  FormatVerticalSpeed(buffer,
                      Units::ToSysUnit(245.4, Unit::FEET_PER_MINUTE),
                      Unit::FEET_PER_MINUTE);
  ok1(StringIsEqual(buffer, _T("+245 fpm")));

  FormatVerticalSpeed(buffer,
                      Units::ToSysUnit(245.4, Unit::FEET_PER_MINUTE),
                      Unit::FEET_PER_MINUTE, false);
  ok1(StringIsEqual(buffer, _T("+245")));
}

static void
TestTemperature()
{
  char buffer[256];

  // Test FormatTemperature()
  FormatTemperature(buffer, 293.93, Unit::KELVIN);
  ok1(StringIsEqual(buffer, _T("294 K")));

  FormatTemperature(buffer, 293.93, Unit::KELVIN, false);
  ok1(StringIsEqual(buffer, _T("294")));

  FormatTemperature(buffer,
                    Units::ToSysUnit(13.4, Unit::DEGREES_CELCIUS),
                    Unit::DEGREES_CELCIUS);
  ok1(StringIsEqual(buffer, _T("13 " DEG "C")));

  FormatTemperature(buffer,
                    Units::ToSysUnit(13.4, Unit::DEGREES_CELCIUS),
                    Unit::DEGREES_CELCIUS, false);
  ok1(StringIsEqual(buffer, _T("13")));

  FormatTemperature(buffer,
                    Units::ToSysUnit(92.7, Unit::DEGREES_FAHRENHEIT),
                    Unit::DEGREES_FAHRENHEIT);
  ok1(StringIsEqual(buffer, _T("93 " DEG "F")));

  FormatTemperature(buffer,
                    Units::ToSysUnit(92.7, Unit::DEGREES_FAHRENHEIT),
                    Unit::DEGREES_FAHRENHEIT, false);
  ok1(StringIsEqual(buffer, _T("93")));
}

static void
TestPressure()
{
  char buffer[256];

  // Test FormatPressure()
  FormatPressure(buffer, AtmosphericPressure::HectoPascal(1013.25),
                 Unit::HECTOPASCAL);
  ok1(StringIsEqual(buffer, _T("1013 hPa")));

  FormatPressure(buffer, AtmosphericPressure::HectoPascal(1013.25),
                 Unit::HECTOPASCAL, false);
  ok1(StringIsEqual(buffer, _T("1013")));

  FormatPressure(buffer, AtmosphericPressure::HectoPascal(1013.25),
                 Unit::MILLIBAR);
  ok1(StringIsEqual(buffer, _T("1013 mb")));

  FormatPressure(buffer, AtmosphericPressure::HectoPascal(1013.25),
                 Unit::MILLIBAR, false);
  ok1(StringIsEqual(buffer, _T("1013")));

  FormatPressure(buffer, AtmosphericPressure::HectoPascal(
      Units::ToSysUnit(103, Unit::TORR)), Unit::TORR);
  ok1(StringIsEqual(buffer, _T("103 mmHg")));

  FormatPressure(buffer, AtmosphericPressure::HectoPascal(
      Units::ToSysUnit(103, Unit::TORR)), Unit::TORR, false);
  ok1(StringIsEqual(buffer, _T("103")));

  FormatPressure(buffer, AtmosphericPressure::HectoPascal(
      Units::ToSysUnit(29.92, Unit::INCH_MERCURY)), Unit::INCH_MERCURY);
  ok1(StringIsEqual(buffer, _T("29.92 inHg")));

  FormatPressure(buffer, AtmosphericPressure::HectoPascal(
      Units::ToSysUnit(29.92, Unit::INCH_MERCURY)),
      Unit::INCH_MERCURY, false);
  ok1(StringIsEqual(buffer, _T("29.92")));
}

int main()
{
  plan_tests(205);

  TestAltitude();
  TestRelativeAltitude();
  TestDistance();
  TestSmallDistance();
  TestDistanceSmart();
  TestSpeed();
  TestVerticalSpeed();
  TestTemperature();
  TestPressure();

  return exit_status();
}
