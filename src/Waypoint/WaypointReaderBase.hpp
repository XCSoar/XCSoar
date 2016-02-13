/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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


#ifndef WAYPOINTFILE_HPP
#define WAYPOINTFILE_HPP

#include "Factory.hpp"

#include <tchar.h>

class Waypoints;
class TLineReader;
class OperationEnvironment;

class WaypointReaderBase 
{
protected:
  const WaypointFactory factory;

protected:
  explicit WaypointReaderBase(WaypointFactory _factory)
    :factory(_factory) {}

public:
  virtual ~WaypointReaderBase() {}

  /**
   * Parses a waypoint file into the given waypoint list
   * @param way_points The waypoint list to fill
   * @return True if the waypoint file parsing was okay, False otherwise
   */
  void Parse(Waypoints &way_points, TLineReader &reader,
             OperationEnvironment &operation);

protected:
  /**
   * Parse a file line
   * @param line The line to parse
   * @param linenum The line number in the file
   * @param way_points The waypoint list to fill
   * @return True if the line was parsed correctly or ignored, False if
   * parsing error occured
   */
  virtual bool ParseLine(const TCHAR* line, Waypoints &way_points) = 0;
};

#endif
