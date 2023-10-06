// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WaypointReaderBase.hpp"
#include "io/BufferedReader.hxx"

void
WaypointReaderBase::Parse(Waypoints &way_points, BufferedReader &reader)
{
  // Read through the lines of the file
  char *line;
  while ((line = reader.ReadLine()) != nullptr) {
    // and parse them
    ParseLine(line, way_points);
  }
}
