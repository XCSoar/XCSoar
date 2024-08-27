// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WaypointReaderSeeYou.hpp"
#include "Units/System.hpp"
#include "Waypoint/Waypoints.hpp"
#include "util/DecimalParser.hxx"
#include "util/IterableSplitString.hxx"
#include "util/NumberParser.hxx"
#include "io/StringConverter.hpp"
#include "io/BufferedCsvReader.hpp"

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
  if (RemoveSuffix(src, "ft"sv) || RemoveSuffix(src, "FT"sv))
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

bool ParseSeeYou(WaypointFactory factory, Waypoints &waypoints, BufferedReader &reader) {
  StringConverter string_converter;

  // 2018: name, code, country, lat, lon, elev, style, rwydir, rwylen, freq, desc
  // 2022: name, code, country, lat, lon, elev, style, rwdir, rwlen, rwwidth, freq, desc, userdata, pics
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
  unsigned iFrequency = 9;
  unsigned iDescription = 10;

  size_t params_num;
  std::array<std::string_view,14> params;

  bool tasks { false };
  bool first_line = true;

  while ( true ) {
    params_num = ReadCsvRecord(reader, params);

    // first line of file
    if (first_line) {
      first_line = false;

      // Empty file
      if (params_num == 0)
        return false;

      // Check whether this is a header (a line with only field names).
      if (StringIsEqualIgnoreCase(params[iLatitude],"lat"sv)) {

        /*
         * Newer cup/cupx specification adds rwwidth, shifts freq and desc
         * right, and adds userdata and pics.
         */
        if ( params_num > iRWWidth &&
             params[iRWWidth] == "rwwidth"sv ) {
          iFrequency = 10;
          iDescription = 11;
        }
        continue;
      }
    }

    // Tasks section
    tasks = params_num == 1 &&
      StringIsEqualIgnoreCase(params[0],"-----Related Tasks-----"sv);

    // End of file or start of task section
    if ( !params_num || tasks )
      break;

    // Skip blank lines and comments (comments are an extension)
    if ( (params_num == 1 && params[0].empty()) ||
         params[0].starts_with('*') )
      continue;

    // Latitude (e.g. 5115.900N)
    GeoPoint location;

    if ( params_num <= iLatitude ||
         !ParseAngle(params[iLatitude], location.latitude, true))
      continue;

    // Longitude (e.g. 00715.900W)
    if ( params_num <= iLongitude ||
         !ParseAngle(params[iLongitude], location.longitude, false))
      continue;

    location.Normalize(); // ensure longitude is within -180:180

    Waypoint new_waypoint = factory.Create(location);

    // Name (e.g. "Some Turnpoint")
    if ( params_num <= iName ||
         params[iName].empty() )
      continue;
    new_waypoint.name.assign(string_converter.Convert(params[iName]));

    // Elevation (e.g. 458.0m)
    /// @todo configurable behaviour
    if ( params_num > iElevation &&
         !params[iElevation].empty() &&
         ParseAltitude(params[iElevation], new_waypoint.elevation) )
      new_waypoint.has_elevation = true;
    else
      factory.FallbackElevation(new_waypoint);

    // Style (e.g. 5)
    if ( params_num > iStyle &&
         !params[iStyle].empty())
      ParseStyle(params[iStyle], new_waypoint.type);

    new_waypoint.flags.turn_point = true;

    // Short name (code) of waypoint
    if ( params_num <= iShortname )
      continue;
    new_waypoint.shortname.assign(string_converter.Convert(params[iShortname]));

    // Frequency & runway direction/length (for airports and landables)
    // and description (e.g. "Some Description")
    if ( new_waypoint.IsLandable() ) {
      if ( params_num > iFrequency &&
           !params[iFrequency].empty() )
        new_waypoint.radio_frequency = RadioFrequency::Parse(params[iFrequency]);

      // Runway length (e.g. 546.0m)
      double rwlen = -1;
      if ( params_num > iRWLen &&
           !params[iRWLen].empty() &&
           ParseDistance(params[iRWLen], rwlen) &&
           rwlen > 0 && rwlen <= 30000)
        new_waypoint.runway.SetLength(uround(rwlen));

      if ( params_num > iRWLen &&
           !params[iRWDir].empty()) {
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
    if ( params_num > iDescription &&
         params[iDescription].starts_with("Hotspot"sv) )
      new_waypoint.type = Waypoint::Type::THERMAL_HOTSPOT;

    if ( params_num > iDescription )
      new_waypoint.comment.assign(string_converter.Convert(params[iDescription]));

    if ( params_num > iUserData )
      new_waypoint.details.assign(string_converter.Convert(params[iUserData]));

    if ( params_num > iPics &&
         !params[iPics].empty() ) {
      for (const auto i : IterableSplitString(params[iPics], ';')) {
        new_waypoint.files_embed.emplace_front(string_converter.Convert(i));
      }
    }
    waypoints.Append(std::move(new_waypoint));
  }

  return tasks;
}
