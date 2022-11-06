/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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
