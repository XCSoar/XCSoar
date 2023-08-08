// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CupWriter.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "io/BufferedOutputStream.hxx"
#include "Engine/Waypoint/Runway.hpp"

static void
WriteAngleDMM(BufferedOutputStream &writer, const Angle angle, bool is_latitude)
{
  // Calculate degrees, minutes and decimal minutes
  const auto dmm = angle.ToDMM();

  // Save them into the buffer string
  writer.Format(is_latitude ? "%02u%02u.%03u" : "%03u%02u.%03u",
                dmm.degrees, dmm.minutes, dmm.decimal_minutes);

  // Attach the buffer string to the output
  if (is_latitude)
    writer.Write(dmm.positive ? "N" : "S");
  else
    writer.Write(dmm.positive ? "E" : "W");
}

static void
WriteAltitude(BufferedOutputStream &writer, double altitude)
{
  writer.Format("%dM", (int)altitude);
}

static void
WriteSeeYouFlags(BufferedOutputStream &writer, const Waypoint &wp)
{
  switch (wp.type) {
  case Waypoint::Type::NORMAL:
    writer.Write('1');
    break;

  case Waypoint::Type::OUTLANDING:
    writer.Write('3');
    break;

  case Waypoint::Type::AIRFIELD:
    if (wp.flags.home)
      writer.Write('4');
    else // 2 or 5 no rule for this!
      writer.Write('2');
    break;

  case Waypoint::Type::MOUNTAIN_PASS:
    writer.Write('6');
    break;

  case Waypoint::Type::MOUNTAIN_TOP:
    writer.Write('7');
    break;

  case Waypoint::Type::OBSTACLE:
    writer.Write('8');
    break;

  case Waypoint::Type::VOR:
    writer.Write('9');
    break;

  case Waypoint::Type::NDB:
    writer.Write("10");
    break;

  case Waypoint::Type::TOWER:
    writer.Write("11");
    break;

  case Waypoint::Type::DAM:
    writer.Write("12");
    break;

  case Waypoint::Type::TUNNEL:
    writer.Write("13");
    break;

  case Waypoint::Type::BRIDGE:
    writer.Write("14");
    break;

  case Waypoint::Type::POWERPLANT:
    writer.Write("15");
    break;

  case Waypoint::Type::CASTLE:
    writer.Write("16");
    break;

  case Waypoint::Type::INTERSECTION:
    writer.Write("17");
    break;

  case Waypoint::Type::MARKER:
    writer.Write("18");
    break;

  case Waypoint::Type::REPORTING_POINT:
    writer.Write("19");
    break;

  case Waypoint::Type::PGTAKEOFF:
    writer.Write("20");
    break;

  case Waypoint::Type::PGLANDING:
    writer.Write("21");
    break;

  case Waypoint::Type::THERMAL_HOTSPOT:
    break;
  }
}

void
WriteCup(BufferedOutputStream &writer, const Waypoint &wp)
{
  // Write Title
  writer.Write('"');
  writer.Write(wp.name.c_str());
  writer.Write('"');
  writer.Write(',');

  // Write Code / Short Name
  writer.Write('"');
  writer.Write(wp.shortname.c_str());
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
  WriteSeeYouFlags(writer, wp);
  writer.Write(',');

  // Write Runway Direction
  if ((wp.type == Waypoint::Type::AIRFIELD ||
       wp.type == Waypoint::Type::OUTLANDING) &&
      wp.runway.IsDirectionDefined())
    writer.Format("%03u", wp.runway.GetDirectionDegrees());

  writer.Write(',');

  // Write Runway Length
  if ((wp.type == Waypoint::Type::AIRFIELD ||
       wp.type == Waypoint::Type::OUTLANDING) &&
      wp.runway.IsLengthDefined())
    writer.Format("%03uM", wp.runway.GetLength());

  writer.Write(',');

  // Write Airport Frequency
  if (wp.radio_frequency.IsDefined()) {
    const unsigned freq = wp.radio_frequency.GetKiloHertz();
    writer.Format("\"%u.%03u\"", freq / 1000, freq % 1000);
  }

  writer.Write(',');

  // Write Description
  writer.Write('"');
  writer.Write(wp.comment.c_str());
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
