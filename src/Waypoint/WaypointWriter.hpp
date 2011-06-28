/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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


#ifndef WAYPOINT_WRITER_HPP
#define WAYPOINT_WRITER_HPP

#include "fixed.hpp"

class Waypoint;
class Waypoints;
class TextWriter;
class Angle;

/** 
 * Waypoint file writer for WinPilot format
 */
class WaypointWriter
{
private:
  const Waypoints &waypoints;
  int file_number;

public:
  WaypointWriter(const Waypoints &_waypoints, int _file_number)
    :waypoints(_waypoints), file_number(_file_number) {}

  void Save(TextWriter &writer);

private:
  static void WriteWaypoint(TextWriter &writer, const Waypoint &wp);
  static void WriteAngle(TextWriter &writer, const Angle &angle,
                         bool is_latitude);
  static void WriteAltitude(TextWriter &writer, fixed altitude);
  static void WriteFlags(TextWriter &writer, const Waypoint &wp);
};

#endif
