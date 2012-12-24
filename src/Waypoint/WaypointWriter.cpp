/*
Copyright_License {

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

#include "Waypoint/WaypointWriter.hpp"

#include "Engine/Waypoint/Waypoints.hpp"
#include "IO/TextWriter.hpp"

void
WaypointWriter::Save(TextWriter &writer)
{
  // Iterate through the waypoint list and save each waypoint with
  // the right file number to the TextWriter
  /// @todo JMW: iteration ordered by ID would be preferred
  for (auto it = waypoints.begin(); it != waypoints.end(); ++it) {
    const Waypoint& wp = *it;
    if (wp.file_num == file_number)
      WriteWaypoint(writer, wp);
  }
}

void
WaypointWriter::WriteWaypoint(TextWriter &writer, const Waypoint& wp)
{
  // Write the waypoint id
  writer.Format("%u,", wp.original_id > 0 ? wp.original_id : wp.id);

  // Write the latitude
  WriteAngle(writer, wp.location.latitude, true);
  writer.Write(',');

  // Write the longitude id
  WriteAngle(writer, wp.location.longitude, false);
  writer.Write(',');

  // Write the altitude id
  WriteAltitude(writer, wp.elevation);
  writer.Write(',');

  // Write the waypoint flags
  WriteFlags(writer, wp);
  writer.Write(',');

  // Write the waypoint name
  writer.Write(wp.name.c_str());
  writer.Write(',');

  // Write the waypoint description
  writer.WriteLine(wp.comment.c_str());
}

void
WaypointWriter::WriteAngle(TextWriter &writer, const Angle angle,
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
WaypointWriter::WriteAltitude(TextWriter &writer, fixed altitude)
{
  writer.Format("%dM", (int)altitude);
}

void
WaypointWriter::WriteFlags(TextWriter &writer, const Waypoint &wp)
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

