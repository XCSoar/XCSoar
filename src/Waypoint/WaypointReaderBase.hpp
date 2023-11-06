// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Factory.hpp"

class Waypoints;
class BufferedReader;

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
  void Parse(Waypoints &way_points, BufferedReader &reader);

protected:
  /**
   * Parse a file line
   * @param line The line to parse
   * @param linenum The line number in the file
   * @param way_points The waypoint list to fill
   * @return True if the line was parsed correctly or ignored, False if
   * parsing error occured
   */
  virtual bool ParseLine(const char *line, Waypoints &way_points) = 0;
};
