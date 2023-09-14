// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WaypointReaderBase.hpp"
#include "Operation/ProgressListener.hpp"
#include "io/LineReader.hpp"

void
WaypointReaderBase::Parse(Waypoints &way_points, TLineReader &reader,
                          ProgressListener &progress)
{
  const long filesize = std::max(reader.GetSize(), 1l);
  progress.SetProgressRange(100);

  // Read through the lines of the file
  TCHAR *line;
  for (unsigned i = 0; (line = reader.ReadLine()) != nullptr; i++) {
    // and parse them
    ParseLine(line, way_points);

    if ((i & 0x3f) == 0)
      progress.SetProgressPosition(reader.Tell() * 100 / filesize);
  }
}
