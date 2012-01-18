/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "Units/UnitsFormatter.hpp"
#include "Units/System.hpp"
#include "Units/Units.hpp"
#include "Util/Macros.hpp"
#include "Util/StringUtil.hpp"
#include "Atmosphere/Pressure.hpp"
#include "TestUtil.hpp"

static void
TestAltitude()
{
  TCHAR buffer[256];

  // Test FormatAltitude()
  Units::FormatAltitude(buffer, ARRAY_SIZE(buffer), fixed(1234), Unit::METER);
  ok1(StringIsEqual(buffer, _T("1234 m")));

  Units::FormatAltitude(buffer, ARRAY_SIZE(buffer),
                        Units::ToSysUnit(fixed(1234), Unit::FEET), Unit::FEET);
  ok1(StringIsEqual(buffer, _T("1234 ft")));

  Units::FormatAltitude(buffer, ARRAY_SIZE(buffer), fixed(-1234), Unit::METER);
  ok1(StringIsEqual(buffer, _T("-1234 m")));

  Units::FormatAltitude(buffer, ARRAY_SIZE(buffer),
                        Units::ToSysUnit(fixed(-1234), Unit::FEET), Unit::FEET);
  ok1(StringIsEqual(buffer, _T("-1234 ft")));

  Units::FormatAltitude(buffer, ARRAY_SIZE(buffer), fixed(1234),
                        Unit::METER, false);
  ok1(StringIsEqual(buffer, _T("1234")));

  Units::FormatAltitude(buffer, ARRAY_SIZE(buffer), fixed(-1234),
                        Unit::METER, false);
  ok1(StringIsEqual(buffer, _T("-1234")));
}

static void
TestRelativeAltitude()
{
  TCHAR buffer[256];

  // Test FormatRelativeAltitude()
  Units::FormatRelativeAltitude(buffer, ARRAY_SIZE(buffer), fixed(1234),
                                Unit::METER);
  ok1(StringIsEqual(buffer, _T("+1234 m")));

  Units::FormatRelativeAltitude(buffer, ARRAY_SIZE(buffer),
                                Units::ToSysUnit(fixed(1234), Unit::FEET),
                                Unit::FEET);
  ok1(StringIsEqual(buffer, _T("+1234 ft")));

  Units::FormatRelativeAltitude(buffer, ARRAY_SIZE(buffer), fixed(-1234),
                                Unit::METER);
  ok1(StringIsEqual(buffer, _T("-1234 m")));

  Units::FormatRelativeAltitude(buffer, ARRAY_SIZE(buffer),
                                Units::ToSysUnit(fixed(-1234), Unit::FEET),
                                Unit::FEET);
  ok1(StringIsEqual(buffer, _T("-1234 ft")));

  Units::FormatRelativeAltitude(buffer, ARRAY_SIZE(buffer), fixed(1234),
                        Unit::METER, false);
  ok1(StringIsEqual(buffer, _T("+1234")));

  Units::FormatRelativeAltitude(buffer, ARRAY_SIZE(buffer), fixed(-1234),
                        Unit::METER, false);
  ok1(StringIsEqual(buffer, _T("-1234")));
}

static void
TestSpeed()
{
  TCHAR buffer[256];

  // Test FormatSpeed()
  Units::FormatSpeed(buffer, ARRAY_SIZE(buffer),
                     fixed(23.46), Unit::METER_PER_SECOND);
  ok1(StringIsEqual(buffer, _T("23 m/s")));

  Units::FormatSpeed(buffer, ARRAY_SIZE(buffer),
                     fixed(23.46), Unit::METER_PER_SECOND, true, true);
  ok1(StringIsEqual(buffer, _T("23.5 m/s")));

  Units::FormatSpeed(buffer, ARRAY_SIZE(buffer),
                     fixed(23.46), Unit::METER_PER_SECOND, false);
  ok1(StringIsEqual(buffer, _T("23")));

  Units::FormatSpeed(buffer, ARRAY_SIZE(buffer),
                     fixed(23.46), Unit::METER_PER_SECOND, false, true);
  ok1(StringIsEqual(buffer, _T("23.5")));

  Units::FormatSpeed(buffer, ARRAY_SIZE(buffer),
                     Units::ToSysUnit(fixed(123.43), Unit::KILOMETER_PER_HOUR),
                     Unit::KILOMETER_PER_HOUR);
  ok1(StringIsEqual(buffer, _T("123 km/h")));

  Units::FormatSpeed(buffer, ARRAY_SIZE(buffer),
                     Units::ToSysUnit(fixed(123.43), Unit::KILOMETER_PER_HOUR),
                     Unit::KILOMETER_PER_HOUR, true, true);
  ok1(StringIsEqual(buffer, _T("123 km/h")));

  Units::FormatSpeed(buffer, ARRAY_SIZE(buffer),
                     Units::ToSysUnit(fixed(83.43), Unit::KILOMETER_PER_HOUR),
                     Unit::KILOMETER_PER_HOUR, true, true);
  ok1(StringIsEqual(buffer, _T("83.4 km/h")));

  Units::FormatSpeed(buffer, ARRAY_SIZE(buffer),
                     Units::ToSysUnit(fixed(123.43), Unit::KNOTS),
                     Unit::KNOTS);
  ok1(StringIsEqual(buffer, _T("123 kt")));

  Units::FormatSpeed(buffer, ARRAY_SIZE(buffer),
                     Units::ToSysUnit(fixed(123.43), Unit::KNOTS),
                     Unit::KNOTS, true, true);
  ok1(StringIsEqual(buffer, _T("123 kt")));

  Units::FormatSpeed(buffer, ARRAY_SIZE(buffer),
                     Units::ToSysUnit(fixed(83.43), Unit::KNOTS),
                     Unit::KNOTS, true, true);
  ok1(StringIsEqual(buffer, _T("83.4 kt")));

  Units::FormatSpeed(buffer, ARRAY_SIZE(buffer),
                     Units::ToSysUnit(fixed(123.43), Unit::STATUTE_MILES_PER_HOUR),
                     Unit::STATUTE_MILES_PER_HOUR);
  ok1(StringIsEqual(buffer, _T("123 mph")));

  Units::FormatSpeed(buffer, ARRAY_SIZE(buffer),
                     Units::ToSysUnit(fixed(123.43), Unit::STATUTE_MILES_PER_HOUR),
                     Unit::STATUTE_MILES_PER_HOUR, true, true);
  ok1(StringIsEqual(buffer, _T("123 mph")));

  Units::FormatSpeed(buffer, ARRAY_SIZE(buffer),
                     Units::ToSysUnit(fixed(83.43), Unit::STATUTE_MILES_PER_HOUR),
                     Unit::STATUTE_MILES_PER_HOUR, true, true);
  ok1(StringIsEqual(buffer, _T("83.4 mph")));
}

static void
TestVerticalSpeed()
{
  TCHAR buffer[256];

  // Test FormatVerticalSpeed()
  Units::FormatVerticalSpeed(buffer, ARRAY_SIZE(buffer),
                             fixed(1.42), Unit::METER_PER_SECOND);
  ok1(StringIsEqual(buffer, _T("+1.4 m/s")));

  Units::FormatVerticalSpeed(buffer, ARRAY_SIZE(buffer),
                             fixed(1.42), Unit::METER_PER_SECOND, false);
  ok1(StringIsEqual(buffer, _T("+1.4")));

  Units::FormatVerticalSpeed(buffer, ARRAY_SIZE(buffer),
                             Units::ToSysUnit(fixed(2.47), Unit::KNOTS),
                             Unit::KNOTS);
  ok1(StringIsEqual(buffer, _T("+2.5 kt")));

  Units::FormatVerticalSpeed(buffer, ARRAY_SIZE(buffer),
                             Units::ToSysUnit(fixed(2.47), Unit::KNOTS),
                             Unit::KNOTS, false);
  ok1(StringIsEqual(buffer, _T("+2.5")));

  Units::FormatVerticalSpeed(buffer, ARRAY_SIZE(buffer),
                             Units::ToSysUnit(fixed(245.4), Unit::FEET_PER_MINUTE),
                             Unit::FEET_PER_MINUTE);
  ok1(StringIsEqual(buffer, _T("+245 fpm")));

  Units::FormatVerticalSpeed(buffer, ARRAY_SIZE(buffer),
                             Units::ToSysUnit(fixed(245.4), Unit::FEET_PER_MINUTE),
                             Unit::FEET_PER_MINUTE, false);
  ok1(StringIsEqual(buffer, _T("+245")));
}

static void
TestTemperature()
{
  TCHAR buffer[256];

  // Test FormatTemperature()
  Units::FormatTemperature(buffer, ARRAY_SIZE(buffer),
                           fixed(293.93), Unit::KELVIN);
  ok1(StringIsEqual(buffer, _T("294 K")));

  Units::FormatTemperature(buffer, ARRAY_SIZE(buffer),
                           fixed(293.93), Unit::KELVIN, false);
  ok1(StringIsEqual(buffer, _T("294")));

  Units::FormatTemperature(buffer, ARRAY_SIZE(buffer),
                           Units::ToSysUnit(fixed(13.4), Unit::DEGREES_CELCIUS),
                           Unit::DEGREES_CELCIUS);
  ok1(StringIsEqual(buffer, _T("13 " DEG "C")));

  Units::FormatTemperature(buffer, ARRAY_SIZE(buffer),
                           Units::ToSysUnit(fixed(13.4), Unit::DEGREES_CELCIUS),
                           Unit::DEGREES_CELCIUS, false);
  ok1(StringIsEqual(buffer, _T("13")));

  Units::FormatTemperature(buffer, ARRAY_SIZE(buffer),
                           Units::ToSysUnit(fixed(92.7), Unit::DEGREES_FAHRENHEIT),
                           Unit::DEGREES_FAHRENHEIT);
  ok1(StringIsEqual(buffer, _T("93 " DEG "F")));

  Units::FormatTemperature(buffer, ARRAY_SIZE(buffer),
                           Units::ToSysUnit(fixed(92.7), Unit::DEGREES_FAHRENHEIT),
                           Unit::DEGREES_FAHRENHEIT, false);
  ok1(StringIsEqual(buffer, _T("93")));
}

static void
TestPressure()
{
  TCHAR buffer[256];

  // Test FormatPressure()
  Units::FormatPressure(buffer, ARRAY_SIZE(buffer),
                        AtmosphericPressure::HectoPascal(fixed(1013.25)),
                        Unit::HECTOPASCAL);
  ok1(StringIsEqual(buffer, _T("1013 hPa")));

  Units::FormatPressure(buffer, ARRAY_SIZE(buffer),
                        AtmosphericPressure::HectoPascal(fixed(1013.25)),
                        Unit::HECTOPASCAL, false);
  ok1(StringIsEqual(buffer, _T("1013")));

  Units::FormatPressure(buffer, ARRAY_SIZE(buffer),
                        AtmosphericPressure::HectoPascal(fixed(1013.25)),
                        Unit::MILLIBAR);
  ok1(StringIsEqual(buffer, _T("1013 mb")));

  Units::FormatPressure(buffer, ARRAY_SIZE(buffer),
                        AtmosphericPressure::HectoPascal(fixed(1013.25)),
                        Unit::MILLIBAR, false);
  ok1(StringIsEqual(buffer, _T("1013")));

  Units::FormatPressure(buffer, ARRAY_SIZE(buffer),
                        AtmosphericPressure::HectoPascal(
                            Units::ToSysUnit(fixed(103), Unit::TORR)),
                        Unit::TORR);
  ok1(StringIsEqual(buffer, _T("103 mmHg")));

  Units::FormatPressure(buffer, ARRAY_SIZE(buffer),
                        AtmosphericPressure::HectoPascal(
                            Units::ToSysUnit(fixed(103), Unit::TORR)),
                        Unit::TORR, false);
  ok1(StringIsEqual(buffer, _T("103")));

  Units::FormatPressure(buffer, ARRAY_SIZE(buffer),
                        AtmosphericPressure::HectoPascal(
                            Units::ToSysUnit(fixed(29.92), Unit::INCH_MERCURY)),
                        Unit::INCH_MERCURY);
  ok1(StringIsEqual(buffer, _T("29.92 inHg")));

  Units::FormatPressure(buffer, ARRAY_SIZE(buffer),
                        AtmosphericPressure::HectoPascal(
                            Units::ToSysUnit(fixed(29.92), Unit::INCH_MERCURY)),
                        Unit::INCH_MERCURY, false);
  ok1(StringIsEqual(buffer, _T("29.92")));
}

int
main(int argc, char **argv)
{
  plan_tests(45);

  TestAltitude();
  TestRelativeAltitude();
  TestSpeed();
  TestVerticalSpeed();
  TestTemperature();
  TestPressure();

  return exit_status();
}
