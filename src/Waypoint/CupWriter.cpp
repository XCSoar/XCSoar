// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CupWriter.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "io/BufferedOutputStream.hxx"
#include "Engine/Waypoint/Runway.hpp"

using std::string_view_literals::operator""sv;

static void
WriteAngleDMM(BufferedOutputStream &writer, const Angle angle, bool is_latitude)
{
  // Calculate degrees, minutes and decimal minutes
  const auto dmm = angle.ToDMM();

  // Save them into the buffer string
  writer.Fmt("{:0{}}{:02}.{:03}{}",
             dmm.degrees, is_latitude ? 2 : 3,
             dmm.minutes, dmm.decimal_minutes,
             is_latitude ? (dmm.positive ? 'N' : 'S') : (dmm.positive ? 'E' : 'W'));
}

static void
WriteAltitude(BufferedOutputStream &writer, double altitude)
{
  writer.Fmt("{}M", (int)altitude);
}

[[gnu::pure]]
static std::string_view
GetSeeYouFlags(const Waypoint &wp) noexcept
{
  switch (wp.type) {
  case Waypoint::Type::NORMAL:
    return "1"sv;

  case Waypoint::Type::OUTLANDING:
    return "3"sv;

  case Waypoint::Type::AIRFIELD:
    return wp.flags.home
      ? "4"sv
      // 2 or 5 no rule for this!
      : "2"sv;

  case Waypoint::Type::MOUNTAIN_PASS:
    return "6"sv;

  case Waypoint::Type::MOUNTAIN_TOP:
    return "7"sv;

  case Waypoint::Type::OBSTACLE:
    return "8"sv;

  case Waypoint::Type::VOR:
    return "9"sv;

  case Waypoint::Type::NDB:
    return "10"sv;

  case Waypoint::Type::TOWER:
    return "11"sv;

  case Waypoint::Type::DAM:
    return "12"sv;

  case Waypoint::Type::TUNNEL:
    return "13"sv;

  case Waypoint::Type::BRIDGE:
    return "14"sv;

  case Waypoint::Type::POWERPLANT:
    return "15"sv;

  case Waypoint::Type::CASTLE:
    return "16"sv;

  case Waypoint::Type::INTERSECTION:
    return "17"sv;

  case Waypoint::Type::MARKER:
    return "18"sv;

  case Waypoint::Type::REPORTING_POINT:
    return "19"sv;

  case Waypoint::Type::PGTAKEOFF:
    return "20"sv;

  case Waypoint::Type::PGLANDING:
    return "21"sv;

  case Waypoint::Type::THERMAL_HOTSPOT:
    break;
  }

  return {};
}

void
WriteCup(BufferedOutputStream &writer, const Waypoint &wp)
{
  // Write Title
  writer.Write('"');
  writer.Write(wp.name);
  writer.Write('"');
  writer.Write(',');

  // Write Code / Short Name
  writer.Write('"');
  writer.Write(wp.shortname);
  writer.Write('"');
  writer.Write(',');

  // Write Country
  writer.Write(',');

  // Write Latitude
  WriteAngleDMM(writer, wp.location.latitude, true);
  writer.Write(',');

  // Write Longitude
  WriteAngleDMM(writer, wp.location.longitude, false);
  writer.Write(',');

  // Write Elevation
  if (wp.has_elevation)
    WriteAltitude(writer, wp.elevation);
  writer.Write(',');

  // Write Style
  writer.Write(GetSeeYouFlags(wp));
  writer.Write(',');

  // Write Runway Direction
  if ((wp.type == Waypoint::Type::AIRFIELD ||
       wp.type == Waypoint::Type::OUTLANDING) &&
      wp.runway.IsDirectionDefined())
    writer.Fmt("{:03}", wp.runway.GetDirectionDegrees());

  writer.Write(',');

  // Write Runway Length
  if ((wp.type == Waypoint::Type::AIRFIELD ||
       wp.type == Waypoint::Type::OUTLANDING) &&
      wp.runway.IsLengthDefined())
    writer.Fmt("{:03}M", wp.runway.GetLength());

  writer.Write(',');

  // Write Airport Frequency
  if (wp.radio_frequency.IsDefined()) {
    const unsigned freq = wp.radio_frequency.GetKiloHertz();
    writer.Fmt("\"{}.{:03}\"", freq / 1000, freq % 1000);
  }

  writer.Write(',');

  // Write Description
  writer.Write('"');
  writer.Write(wp.comment);
  writer.Write('"');
  writer.Write('\n');
}

void
WriteCup(BufferedOutputStream &writer, const Waypoints &waypoints,
         WaypointOrigin origin)
{
  // Iterate through the waypoint list and save each waypoint with
  // the right file number to the BufferedOutputStream
  /// @todo JMW: iteration ordered by ID would be preferred
  for (auto i : waypoints) {
    const Waypoint &wp = *i;
    if (wp.origin == origin)
      WriteCup(writer, wp);
  }
}
