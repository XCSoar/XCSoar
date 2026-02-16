// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WaypointReaderWinPilot.hpp"
#include "Units/System.hpp"
#include "Waypoint/Waypoints.hpp"
#include "util/NumberParser.hxx"
#include "util/StringSplit.hxx"

#include <math.h>

static std::string_view
NextColumn(std::string_view &line) noexcept
{
  auto [value, rest] = Split(line, ',');
  line = rest;
  return value;
}

static bool
ParseAngle(std::string_view src, Angle &dest, const bool lat)
{
  // Two format variants:
  // 51:47.841N (DD:MM.mmm // the usual)
  // 51:23:45N (DD:MM:SS)

  bool negative = false;
  if (src.ends_with(lat ? 'S' : 'W'))
    negative = true;
  else if (!src.ends_with(lat ? 'N' : 'E'))
    return false;

  src.remove_suffix(1);

  const auto [degrees_string, rest1] = Split(src, ':');
  if (degrees_string.empty() || rest1.size() < 5)
    return false;

  unsigned degrees;
  if (auto value = ParseInteger<unsigned>(degrees_string))
    // Limit angle to +/- 90 degrees for Latitude or +/- 180 degrees for Longitude
    degrees = std::min(*value, lat ? 90U : 180U);
  else
    return false;

  const auto minutes_string = rest1.substr(0, 2);
  unsigned minutes;
  if (!ParseIntegerTo(minutes_string, minutes))
    return false;

  if (minutes >= 60)
    return false;

  double seconds;
  if (rest1[2] == '.') {
    /* 000..999 */

    const auto fraction_string = rest1.substr(3);
    if (fraction_string.size() > 4)
      return false;

    if (auto value = ParseInteger<unsigned>(fraction_string)) {
      seconds = *value / (60 * pow(10, fraction_string.size()));
    } else
      return false;

    if (seconds > 1)
      return false;
  } else if (rest1[2] == ':') {
    /* 00..59 */

    const auto seconds_string = rest1.substr(3);
    if (seconds_string.size() != 2)
      return false;

    if (auto value = ParseInteger<unsigned>(seconds_string);
        value && *value < 60)
      seconds = *value / 3600.;
    else
      return false;
  } else
      return false;

  double value = degrees + minutes / 60. + seconds;
  if (negative)
    value = -value;

  dest = Angle::Degrees(value);
  return true;
}

static bool
ParseRunwayDirection(std::string_view src, Runway &dest)
{
  // WELT2000 written files contain a 4-digit runway direction specification
  // at the end of the comment, e.g. "123.50 0927"

  if (const auto after_space = SplitLast(src, ' ').second; !after_space.empty())
    src = after_space;

  src = Split(src, ' ').first;

  if (src.size() != 4)
    return false;

  const auto a1_ = ParseInteger<unsigned>(src.substr(0, 2));
  const auto a2_ = ParseInteger<unsigned>(src.substr(2, 2));
  if (!a1_ || !a2_)
    return false;

  unsigned a1 = *a1_ * 10;
  unsigned a2 = *a2_ * 10;

  if (a1 > 360 || a2 > 360)
    return false;

  if (a1 == 360)
    a1 = 0;
  if (a2 == 360)
    a2 = 0;

  // TODO: WELT2000 generates quite a few entries where
  //       a) a1 == a2 (accept those) and
  //       b) the angles are neither equal or reciprocal (discard those)
  //       Try finding a logic behind this behaviour.
  if (a1 != a2 && (a1 + 180) % 360 != a2)
    return false;

  dest.SetDirectionDegrees(a1);
  return true;
}

static bool
ParseAltitude(std::string_view src, double &dest)
{
  Unit unit = Unit::METER;
  if (src.ends_with('f') || src.ends_with('F'))
    unit = Unit::FEET;
  else if (!src.ends_with('m') && !src.ends_with('M'))
    return false;

  src.remove_suffix(1);

  // Parse string
  const auto value = ParseInteger<int>(src);
  if (!value)
    return false;

  // Convert to system unit if necessary
  dest = Units::ToSysUnit(*value, unit);
  return true;
}

static bool
ParseFlags(std::string_view src, Waypoint &dest)
{
  // A = Airport
  // T = Turnpoint
  // L = Landable
  // H = Home
  // S = Start point
  // F = Finish point
  // R = Restricted
  // W = Waypoint flag

  for (const char ch : src) {
    switch (ch) {
    case 'A':
    case 'a':
      dest.type = Waypoint::Type::AIRFIELD;
      break;
    case 'T':
    case 't':
      dest.flags.turn_point = true;
      break;
    case 'L':
    case 'l':
      // Don't downgrade!
      if (dest.type != Waypoint::Type::AIRFIELD)
        dest.type = Waypoint::Type::OUTLANDING;
      break;
    case 'H':
    case 'h':
      dest.flags.home = true;
      break;
    case 'S':
    case 's':
      dest.flags.start_point = true;
      break;
    case 'F':
    case 'f':
      dest.flags.finish_point = true;
      break;
    }
  }

  return true;
}

bool
WaypointReaderWinPilot::ParseLine(const char *line, Waypoints &waypoints)
{
  // If (end-of-file)
  if (line[0] == '\0')
    // -> return without error condition
    return true;

  // If comment
  if (line[0] == '*') {
    if (first) {
      first = false;
      welt2000_format = strstr(line, "WRITTEN BY WELT2000") != nullptr;
    }

    // -> return without error condition
    return true;
  }

  std::string_view rest{line};

  NextColumn(rest);

  GeoPoint location;

  // Latitude (e.g. 51:15.900N)
  if (!ParseAngle(NextColumn(rest), location.latitude, true))
    return false;

  // Longitude (e.g. 00715.900W)
  if (!ParseAngle(NextColumn(rest), location.longitude, false))
    return false;
  location.Normalize(); // ensure longitude is within -180:180

  Waypoint new_waypoint = factory.Create(location);

  // Altitude (e.g. 458M)
  /// @todo configurable behaviour
  if (ParseAltitude(NextColumn(rest), new_waypoint.elevation))
    new_waypoint.has_elevation = true;
  else
    factory.FallbackElevation(new_waypoint);

  // Waypoint Flags (e.g. AT)
  ParseFlags(NextColumn(rest), new_waypoint);

  // Name (e.g. KAMPLI)
  new_waypoint.name = std::string{string_converter.Convert(NextColumn(rest))};
  if (new_waypoint.name.empty())
    return false;

  // Description (e.g. 119.750 Airport)
  const auto comment = NextColumn(rest);
  new_waypoint.comment = std::string{string_converter.Convert(comment)};
  ParseRunwayDirection(comment, new_waypoint.runway);

  waypoints.Append(std::move(new_waypoint));
  return true;
}
