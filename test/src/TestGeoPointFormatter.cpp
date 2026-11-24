// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Formatter/GeoPointFormatter.hpp"
#include "Math/Constants.hpp"
#include "util/Macros.hpp"
#include "util/StringAPI.hxx"
#include "TestUtil.hpp"

#include <cmath>

static bool
StringIsEqualWildcard(const char *string1, const char *string2,
                      char wildcard = '*')
{
  while (*string1 == *string2 ||
         *string1 == wildcard ||
         *string2 == wildcard) {
    if (*string1 == '\0' && *string2 == '\0')
      return true;

    string1++;
    string2++;
  }
  return false;
}

int main()
{
  plan_tests(14 + 6);

  char buffer[256];
  GeoPoint location1(Angle::Degrees(8.466322),
                     Angle::Degrees(49.487153));

  GeoPoint location2(Angle::Degrees(-70.011667),
                     Angle::Degrees(-32.653333));

  const auto ten_point_one = Angle::Degrees(10.1);
  const GeoPoint minute_boundary(ten_point_one, ten_point_one);

  // Approach 10.1 degrees from below in the formatter's degree conversion.
  const auto below_minute =
    Angle::Radians(std::nextafter(10.1 / RAD_TO_DEG, 0.0));
  const GeoPoint minute_carry(below_minute, below_minute);
  const GeoPoint degree_carry(Angle::Degrees(10.999999),
                              Angle::Degrees(-10.999999));

  // Test DD.ddddd
  FormatGeoPoint(location1, buffer, ARRAY_SIZE(buffer),
                 CoordinateFormat::DD_DDDDD);
  ok1(StringIsEqual(buffer, "49.48715° N 008.46632° E"));

  FormatGeoPoint(location2, buffer, ARRAY_SIZE(buffer),
                 CoordinateFormat::DD_DDDDD);
  ok1(StringIsEqual(buffer, "32.65333° S 070.01167° W"));


  // Test DDMM.mmm
  FormatGeoPoint(location1, buffer, ARRAY_SIZE(buffer),
                 CoordinateFormat::DDMM_MMM);
  ok1(StringIsEqual(buffer, "49°29.229' N 008°27.979' E"));

  FormatGeoPoint(location2, buffer, ARRAY_SIZE(buffer),
                 CoordinateFormat::DDMM_MMM);
  ok1(StringIsEqual(buffer, "32°39.200' S 070°00.700' W"));

  FormatGeoPoint(minute_boundary, buffer, ARRAY_SIZE(buffer),
                 CoordinateFormat::DDMM_MMM);
  ok1(StringIsEqual(buffer, "10°06.000' N 010°06.000' E"));

  FormatGeoPoint(minute_carry, buffer, ARRAY_SIZE(buffer),
                 CoordinateFormat::DDMM_MMM);
  ok1(StringIsEqual(buffer, "10°06.000' N 010°06.000' E"));

  FormatGeoPoint(degree_carry, buffer, ARRAY_SIZE(buffer),
                 CoordinateFormat::DDMM_MMM);
  ok1(StringIsEqual(buffer, "11°00.000' S 011°00.000' E"));


  // Test DDMMSS
  FormatGeoPoint(location1, buffer, ARRAY_SIZE(buffer),
                 CoordinateFormat::DDMMSS);
  ok1(StringIsEqual(buffer, "49°29'14\" N 008°27'59\" E"));

  FormatGeoPoint(location2, buffer, ARRAY_SIZE(buffer),
                 CoordinateFormat::DDMMSS);
  ok1(StringIsEqual(buffer, "32°39'12\" S 070°00'42\" W"));


  // Test DDMMSS.s
  FormatGeoPoint(location1, buffer, ARRAY_SIZE(buffer),
                 CoordinateFormat::DDMMSS_S);
  ok1(StringIsEqual(buffer, "49°29'13.8\" N 008°27'58.8\" E"));

  FormatGeoPoint(location2, buffer, ARRAY_SIZE(buffer),
                 CoordinateFormat::DDMMSS_S);
  ok1(StringIsEqual(buffer, "32°39'12.0\" S 070°00'42.0\" W"));

  FormatGeoPoint(minute_boundary, buffer, ARRAY_SIZE(buffer),
                 CoordinateFormat::DDMMSS_S);
  ok1(StringIsEqual(buffer, "10°06'00.0\" N 010°06'00.0\" E"));

  FormatGeoPoint(minute_carry, buffer, ARRAY_SIZE(buffer),
                 CoordinateFormat::DDMMSS_S);
  ok1(StringIsEqual(buffer, "10°06'00.0\" N 010°06'00.0\" E"));

  FormatGeoPoint(degree_carry, buffer, ARRAY_SIZE(buffer),
                 CoordinateFormat::DDMMSS_S);
  ok1(StringIsEqual(buffer, "11°00'00.0\" S 011°00'00.0\" E"));

  // Test UTM
  FormatGeoPoint(location1, buffer, ARRAY_SIZE(buffer),
                 CoordinateFormat::UTM);
  ok1(StringIsEqualWildcard(buffer, "32U 4613** 5481***"));

  FormatGeoPoint(location2, buffer, ARRAY_SIZE(buffer),
                 CoordinateFormat::UTM);
  ok1(StringIsEqualWildcard(buffer, "19H 4051** 6386***"));

  // Test separator
  FormatGeoPoint(location1, buffer, ARRAY_SIZE(buffer),
                 CoordinateFormat::DDMMSS, '\n');
  ok1(StringIsEqual(buffer, "49°29'14\" N\n008°27'59\" E"));

  FormatGeoPoint(location2, buffer, ARRAY_SIZE(buffer),
                 CoordinateFormat::DDMMSS, '\n');
  ok1(StringIsEqual(buffer, "32°39'12\" S\n070°00'42\" W"));

  FormatGeoPoint(location1, buffer, ARRAY_SIZE(buffer),
                 CoordinateFormat::UTM, '\n');
  ok1(StringIsEqualWildcard(buffer, "32U\n4613**\n5481***"));

  FormatGeoPoint(location2, buffer, ARRAY_SIZE(buffer),
                 CoordinateFormat::UTM, '\n');
  ok1(StringIsEqualWildcard(buffer, "19H\n4051**\n6386***"));


  return exit_status();
}
