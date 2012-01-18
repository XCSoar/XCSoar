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
#include "Util/Macros.hpp"
#include "Util/StringUtil.hpp"
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

int
main(int argc, char **argv)
{
  plan_tests(25);

  TestAltitude();
  TestRelativeAltitude();
  TestSpeed();

  return exit_status();
}
