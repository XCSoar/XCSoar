/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "Waypoint/WaypointWriter.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "IO/TextWriter.hpp"
#include "Engine/Waypoint/Runway.hpp"

void
WaypointWriter::Save(TextWriter &writer, WaypointFileType type)
{
  // Iterate through the waypoint list and save each waypoint with
  // the right file number to the TextWriter
  /// @todo JMW: iteration ordered by ID would be preferred
  for (auto it = waypoints.begin(); it != waypoints.end(); ++it) {
    const Waypoint& wp = *it;
    if (wp.file_num == file_number)
      WriteWaypoint(writer, wp, type);
  }
}

void
WaypointWriter::WriteWaypoint(TextWriter &writer, const Waypoint &wp, WaypointFileType type)
{
  switch (type) {
  case WaypointFileType::WINPILOT:
    WriteWinPilot(writer, wp);
    break;

  case WaypointFileType::SEEYOU:
    WriteSeeYou(writer, wp);
    break;

  case WaypointFileType::UNKNOWN:
  case WaypointFileType::ZANDER:
  case WaypointFileType::FS:
  case WaypointFileType::OZI_EXPLORER:
  case WaypointFileType::COMPE_GPS:
    gcc_unreachable();
    break;
  }
}

void
WaypointWriter::WriteWinPilot(TextWriter &writer, const Waypoint &wp)
{
  // Write the waypoint id
  writer.Format("%u,", wp.original_id > 0 ? wp.original_id : wp.id);

  // Write the latitude
  WriteAngleDMS(writer, wp.location.latitude, true);
  writer.Write(',');

  // Write the longitude id
  WriteAngleDMS(writer, wp.location.longitude, false);
  writer.Write(',');

  // Write the altitude id
  WriteAltitude(writer, wp.elevation);
  writer.Write(',');

  // Write the waypoint flags
  WriteWinPilotFlags(writer, wp);
  writer.Write(',');

  // Write the waypoint name
  writer.Write(wp.name.c_str());
  writer.Write(',');

  // Write the waypoint description
  writer.WriteLine(wp.comment.c_str());
}

void
WaypointWriter::WriteSeeYou(TextWriter &writer, const Waypoint &wp)
{
  // Write Title
  writer.Format(_T("\"%s\","), wp.name.c_str());

  // Write Code
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
  writer.FormatLine(_T("\"%s\""), wp.comment.c_str());
}

void
WaypointWriter::WriteAngleDMS(TextWriter &writer, const Angle angle,
                           bool is_latitude)
{
  // Calculate degrees, minutes and seconds
  unsigned deg, min, sec;
  bool is_positive;
  angle.ToDMS(deg, min, sec, is_positive);

  // Save them into the buffer string
  writer.Format(is_latitude ? "%02u:%02u:%02u" : "%03u:%02u:%02u",
                deg, min, sec);

  // Attach the buffer string to the output
  if (is_latitude)
    writer.Write(is_positive ? "N" : "S");
  else
    writer.Write(is_positive ? "E" : "W");
}

void
WaypointWriter::WriteAngleDMM(TextWriter &writer, const Angle angle,
                              bool is_latitude)
{
  // Calculate degrees, minutes and decimal minutes
  unsigned deg, min, mmm;
  bool is_positive;
  angle.ToDMM(deg, min, mmm, is_positive);

  // Save them into the buffer string
  writer.Format(is_latitude ? "%02u%02u.%03u" : "%03u%02u.%03u",
                deg, min, mmm);

  // Attach the buffer string to the output
  if (is_latitude)
    writer.Write(is_positive ? "N" : "S");
  else
    writer.Write(is_positive ? "E" : "W");
}

void
WaypointWriter::WriteAltitude(TextWriter &writer, fixed altitude)
{
  writer.Format("%dM", (int)altitude);
}

void
WaypointWriter::WriteWinPilotFlags(TextWriter &writer, const Waypoint &wp)
{
  if (wp.IsAirport())
    writer.Write('A');
  if (wp.flags.turn_point)
    writer.Write('T');
  if (wp.IsLandable())
    writer.Write('L');
  if (wp.flags.home)
    writer.Write('H');
  if (wp.flags.start_point)
    writer.Write('S');
  if (wp.flags.finish_point)
    writer.Write('F');

  // set as turnpoint by default if nothing else
  if (!wp.flags.turn_point &&
      !wp.IsLandable() &&
      !wp.flags.home &&
      !wp.flags.start_point &&
      !wp.flags.finish_point)
    writer.Write('T');
}

void
WaypointWriter::WriteSeeYouFlags(TextWriter &writer, const Waypoint &wp)
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

  case Waypoint::Type::TOWER:
    // 11 or 16 no rule for this!
    writer.Write("11");
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

  case Waypoint::Type::THERMAL_HOTSPOT:
    break;
  }
}

