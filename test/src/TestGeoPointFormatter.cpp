// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Formatter/GeoPointFormatter.hpp"
#include "util/Macros.hpp"
#include "util/StringAPI.hxx"
#include "TestUtil.hpp"

static bool
StringIsEqualWildcard(const char *string1, const char *string2,
                      char wildcard = _T('*'))
{
  while (*string1 == *string2 ||
         *string1 == wildcard ||
         *string2 == wildcard) {
    if (*string1 == _T('\0') && *string2 == _T('\0'))
      return true;

    string1++;
    string2++;
  }
  return false;
}

int main()
{
  plan_tests(14);

  char buffer[256];
  GeoPoint location1(Angle::Degrees(8.466322),
                     Angle::Degrees(49.487153));

  GeoPoint location2(Angle::Degrees(-70.011667),
                     Angle::Degrees(-32.653333));

  // Test DD.ddddd
  FormatGeoPoint(location1, buffer, ARRAY_SIZE(buffer),
                 CoordinateFormat::DD_DDDDD);
  ok1(StringIsEqual(buffer, _T("49.48715° N 008.46632° E")));

  FormatGeoPoint(location2, buffer, ARRAY_SIZE(buffer),
                 CoordinateFormat::DD_DDDDD);
  ok1(StringIsEqual(buffer, _T("32.65333° S 070.01167° W")));


  // Test DDMM.mmm
  FormatGeoPoint(location1, buffer, ARRAY_SIZE(buffer),
                 CoordinateFormat::DDMM_MMM);
  ok1(StringIsEqual(buffer, _T("49°29.229' N 008°27.979' E")));

  FormatGeoPoint(location2, buffer, ARRAY_SIZE(buffer),
                 CoordinateFormat::DDMM_MMM);
  ok1(StringIsEqual(buffer, _T("32°39.200' S 070°00.700' W")));


  // Test DDMMSS
  FormatGeoPoint(location1, buffer, ARRAY_SIZE(buffer),
                 CoordinateFormat::DDMMSS);
  ok1(StringIsEqual(buffer, _T("49°29'14\" N 008°27'59\" E")));

  FormatGeoPoint(location2, buffer, ARRAY_SIZE(buffer),
                 CoordinateFormat::DDMMSS);
  ok1(StringIsEqual(buffer, _T("32°39'12\" S 070°00'42\" W")));


  // Test DDMMSS.s
  FormatGeoPoint(location1, buffer, ARRAY_SIZE(buffer),
                 CoordinateFormat::DDMMSS_S);
  ok1(StringIsEqual(buffer, _T("49°29'13.8\" N 008°27'58.8\" E")));

  FormatGeoPoint(location2, buffer, ARRAY_SIZE(buffer),
                 CoordinateFormat::DDMMSS_S);
  ok1(StringIsEqual(buffer, _T("32°39'12.0\" S 070°00'42.0\" W")));

  // Test UTM
  FormatGeoPoint(location1, buffer, ARRAY_SIZE(buffer),
                 CoordinateFormat::UTM);
  ok1(StringIsEqualWildcard(buffer, _T("32U 4613** 5481***")));

  FormatGeoPoint(location2, buffer, ARRAY_SIZE(buffer),
                 CoordinateFormat::UTM);
  ok1(StringIsEqualWildcard(buffer, _T("19H 4051** 6386***")));

  // Test separator
  FormatGeoPoint(location1, buffer, ARRAY_SIZE(buffer),
                 CoordinateFormat::DDMMSS, _T('\n'));
  ok1(StringIsEqual(buffer, _T("49°29'14\" N\n008°27'59\" E")));

  FormatGeoPoint(location2, buffer, ARRAY_SIZE(buffer),
                 CoordinateFormat::DDMMSS, _T('\n'));
  ok1(StringIsEqual(buffer, _T("32°39'12\" S\n070°00'42\" W")));

  FormatGeoPoint(location1, buffer, ARRAY_SIZE(buffer),
                 CoordinateFormat::UTM, _T('\n'));
  ok1(StringIsEqualWildcard(buffer, _T("32U\n4613**\n5481***")));

  FormatGeoPoint(location2, buffer, ARRAY_SIZE(buffer),
                 CoordinateFormat::UTM, _T('\n'));
  ok1(StringIsEqualWildcard(buffer, _T("19H\n4051**\n6386***")));


  return exit_status();
}
