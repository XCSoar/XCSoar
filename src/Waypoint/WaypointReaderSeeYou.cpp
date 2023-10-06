// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WaypointReaderSeeYou.hpp"
#include "CupParser.hpp"
#include "Units/System.hpp"
#include "Waypoint/Waypoints.hpp"
#include "util/DecimalParser.hxx"
#include "util/IterableSplitString.hxx"
#include "util/NumberParser.hxx"

#include <stdlib.h>

using std::string_view_literals::operator""sv;

static bool
ParseAngle(std::string_view src, Angle &dest, const bool lat) noexcept
{
  bool negative = false;
  if (src.ends_with(lat ? 'S' : 'W'))
    negative = true;
  else if (!src.ends_with(lat ? 'N' : 'E'))
    return false;

  src.remove_suffix(1);

  const auto [degrees_minutes_string, fraction_string] = Split(src, '.');
  if (fraction_string.size() != 3)
    return false;

  unsigned degrees_minutes, fraction;

  if (auto value = ParseInteger<unsigned>(degrees_minutes_string))
    degrees_minutes = *value;
  else
    return false;

  // Limit angle to +/- 90 degrees for Latitude or +/- 180 degrees for Longitude
  const unsigned degrees = std::min(degrees_minutes / 100, lat ? 90U : 180U);
  const unsigned minutes = degrees_minutes % 100;

  if (auto value = ParseInteger<unsigned>(fraction_string))
    fraction = *value;
  else
    return false;

  auto value = static_cast<double>(degrees) + minutes / 60. + fraction / 60000.;
  if (negative)
    value = -value;

  // Save angle
  dest = Angle::Degrees(value);
  return true;
}

static bool
ParseAltitude(std::string_view src, double &dest) noexcept
{
  Unit unit = Unit::METER;
  if (src.ends_with('f') || src.ends_with('F'))
    unit = Unit::FEET;
  else if (!src.ends_with('m') && !src.ends_with('M'))
    return false;

  src.remove_suffix(1);

  // Parse string
  const auto value = ParseDecimal(src);
  if (!value)
    return false;

  // Convert to system unit if necessary
  dest = Units::ToSysUnit(*value, unit);
  return true;
}

static bool
ParseDistance(std::string_view src, double &dest)
{
  Unit unit = Unit::METER;
  if (RemoveSuffix(src, "ml"sv) || RemoveSuffix(src, "ML"sv))
    unit = Unit::STATUTE_MILES;
  else if (RemoveSuffix(src, "nm"sv) || RemoveSuffix(src, "NM"sv))
    unit = Unit::NAUTICAL_MILES;
  else if (RemoveSuffix(src, "ft"sv) || RemoveSuffix(src, "FT"sv))
    unit = Unit::FEET;
  else if (!RemoveSuffix(src, "m"sv) && !RemoveSuffix(src, "M"sv))
    return false;

  // Parse string
  const auto value = ParseDecimal(src);
  if (!value)
    return false;

  // Convert to system unit if necessary
  dest = Units::ToSysUnit(*value, unit);
  return true;
}

static bool
ParseStyle(std::string_view src, Waypoint::Type &type)
{
  // 1 - Normal
  // 2 - AirfieldGrass
  // 3 - Outlanding
  // 4 - GliderSite
  // 5 - AirfieldSolid ...

  // Parse string
  unsigned style;

  if (auto value = ParseInteger<unsigned>(src))
    style = *value;
  else
    return false;

  // Update flags
  switch (style) {
  case 3:
    type = Waypoint::Type::OUTLANDING;
    break;
  case 2:
  case 4:
  case 5:
    type = Waypoint::Type::AIRFIELD;
    break;
  case 6:
    type = Waypoint::Type::MOUNTAIN_PASS;
    break;
  case 7:
    type = Waypoint::Type::MOUNTAIN_TOP;
    break;
  case 8:
    type = Waypoint::Type::OBSTACLE;
    break;
  case 9:
    type = Waypoint::Type::VOR;
    break;
  case 10:
    type = Waypoint::Type::NDB;
   break;
  case 11:
    type = Waypoint::Type::TOWER;
    break;
  case 12:
    type = Waypoint::Type::DAM;
    break;
  case 13:
    type = Waypoint::Type::TUNNEL;
    break;
  case 14:
    type = Waypoint::Type::BRIDGE;
    break;
  case 15:
    type = Waypoint::Type::POWERPLANT;
    break;
  case 16:
    type = Waypoint::Type::CASTLE;
    break;
  case 17:
    type = Waypoint::Type::INTERSECTION;
    break;
  case 18:
    type = Waypoint::Type::MARKER;
    break;
  case 19:
    type = Waypoint::Type::REPORTING_POINT;
    break;
  case 20:
    type = Waypoint::Type::PGTAKEOFF;
    break;
  case 22:
    type = Waypoint::Type::PGLANDING;
    break;
  }
  return true;
}

bool
WaypointReaderSeeYou::ParseLine(const char *line, Waypoints &waypoints)
{
  enum {
    iName = 0,
    iShortname = 1,
    iLatitude = 3,
    iLongitude = 4,
    iElevation = 5,
    iStyle = 6,
    iRWDir = 7,
    iRWLen = 8,
    iRWWidth = 9,
    iUserData = 12,
    iPics = 13
  };

  // If (end-of-file or comment)
  if (StringIsEmpty(line) || *line == '*')
    // -> return without error condition
    return true;

  // If task marker is reached ignore all following lines
  if (StringStartsWith(line, "-----Related Tasks-----"))
    ignore_following = true;
  if (ignore_following)
    return true;

  // Get fields
  std::array<std::string_view, 20> params;
  CupSplitColumns(line, params);

  if (first) {
    first = false;
    if (line[0] != '"') {
      /*
       * If the first line doesn't begin with a quotation mark, it
       * doesn't describe a waypoint. It probably contains field names.
       */
      if (params[iRWWidth] == "rwwidth"sv) {
        /*
         * The name of the 10th field is "rwwidth" (runway width).
         * This field doesn't exist in "typical" SeeYou (*.cup) waypoint
         * files but is in files saved by at least some versions of
         * SeeYou Mobile. If the rwwidth field exists, the frequency and
         * description fields are shifted one position to the right.
         */
        iFrequency = 10;
        iDescription = 11;
      } else {
        iFrequency = 9;
        iDescription = 10;
      }
      return true;
    }
  }

  // Check if the basic fields are provided
  if (params[iLatitude].data() == nullptr)
    return false;

  GeoPoint location;

  // Latitude (e.g. 5115.900N)
  if (!ParseAngle(params[iLatitude], location.latitude, true))
    return false;

  // Longitude (e.g. 00715.900W)
  if (!ParseAngle(params[iLongitude], location.longitude, false))
    return false;

  location.Normalize(); // ensure longitude is within -180:180

  Waypoint new_waypoint = factory.Create(location);

  // Name (e.g. "Some Turnpoint")
  if (params[iName].empty())
    return false;
  new_waypoint.name.assign(string_converter.Convert(params[iName]));

  // Elevation (e.g. 458.0m)
  /// @todo configurable behaviour
  if (!params[iElevation].empty() &&
      ParseAltitude(params[iElevation], new_waypoint.elevation))
    new_waypoint.has_elevation = true;
  else
    factory.FallbackElevation(new_waypoint);

  // Style (e.g. 5)
  if (!params[iStyle].empty())
    ParseStyle(params[iStyle], new_waypoint.type);

  new_waypoint.flags.turn_point = true;

  // Short name (code) of waypoint 
  new_waypoint.shortname.assign(string_converter.Convert(params[iShortname]));

  // Frequency & runway direction/length (for airports and landables)
  // and description (e.g. "Some Description")
  if (new_waypoint.IsLandable()) {
    if (!params[iFrequency].empty())
      new_waypoint.radio_frequency = RadioFrequency::Parse(params[iFrequency]);

    // Runway length (e.g. 546.0m)
    double rwlen = -1;
    if (!params[iRWLen].empty() && ParseDistance(params[iRWLen], rwlen) &&
        rwlen > 0 && rwlen <= 30000)
      new_waypoint.runway.SetLength(uround(rwlen));

    if (!params[iRWDir].empty()) {
      if (auto value = ParseInteger<unsigned>(params[iRWDir])) {
        unsigned direction = *value;

        if (direction <= 360) {
          if (direction == 360)
            direction = 0;

          new_waypoint.runway.SetDirectionDegrees(direction);
        }
      }
    }
  }

  /*
   * This convention was introduced by the OpenAIP project
   * (http://www.openaip.net/), since no waypoint type exists for
   * thermal hotspots.
   */
  if (params[iDescription].starts_with("Hotspot"sv))
    new_waypoint.type = Waypoint::Type::THERMAL_HOTSPOT;

  new_waypoint.comment.assign(string_converter.Convert(params[iDescription]));
  new_waypoint.details.assign(string_converter.Convert(params[iUserData]));

  if (!params[iPics].empty()) {
    for (const auto i : IterableSplitString(params[iPics], ';')) {
      new_waypoint.files_embed.emplace_front(string_converter.Convert(i));
    }
  }
  waypoints.Append(std::move(new_waypoint));
  return true;
}
