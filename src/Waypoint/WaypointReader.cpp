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

#include "WaypointReader.hpp"
#include "WaypointReaderZander.hpp"
#include "WaypointReaderSeeYou.hpp"
#include "WaypointReaderWinPilot.hpp"
#include "WaypointReaderFS.hpp"
#include "WaypointReaderOzi.hpp"
#include "WaypointReaderCompeGPS.hpp"
#include "WaypointFileType.hpp"
#include "IO/ZipLineReader.hpp"
#include "IO/FileLineReader.hpp"

#include <memory>

static WaypointReaderBase *
CreateWaypointReader(WaypointFileType type, WaypointFactory factory)
{
  switch (type) {
  case WaypointFileType::UNKNOWN:
    break;

  case WaypointFileType::WINPILOT:
    return new WaypointReaderWinPilot(factory);

  case WaypointFileType::SEEYOU:
    return new WaypointReaderSeeYou(factory);

  case WaypointFileType::ZANDER:
    return new WaypointReaderZander(factory);

  case WaypointFileType::FS:
    return new WaypointReaderFS(factory);

  case WaypointFileType::OZI_EXPLORER:
    return new WaypointReaderOzi(factory);

  case WaypointFileType::COMPE_GPS:
    return new WaypointReaderCompeGPS(factory);
  }

  return nullptr;
}

bool
ReadWaypointFile(Path path, WaypointFileType file_type,
                 Waypoints &way_points,
                 WaypointFactory factory, OperationEnvironment &operation)
try {
  std::unique_ptr<WaypointReaderBase> reader(CreateWaypointReader(file_type,
                                                                  factory));
  if (!reader)
    return false;

  FileLineReader line_reader(path, Charset::AUTO);
  reader->Parse(way_points, line_reader, operation);
  return true;
} catch (const std::runtime_error &) {
  return false;
}

bool
ReadWaypointFile(Path path, Waypoints &way_points,
                 WaypointFactory factory, OperationEnvironment &operation)
{
  return ReadWaypointFile(path, DetermineWaypointFileType(path),
                          way_points, factory, operation);
}

bool
ReadWaypointFile(struct zzip_dir *dir, const char *path,
                 WaypointFileType file_type, Waypoints &way_points,
                 WaypointFactory factory, OperationEnvironment &operation)
try {
  std::unique_ptr<WaypointReaderBase> reader(CreateWaypointReader(file_type,
                                                                  factory));
  if (!reader)
    return false;

  ZipLineReader line_reader(dir, path, Charset::AUTO);
  reader->Parse(way_points, line_reader, operation);
  return true;
} catch (const std::runtime_error &e) {
  return false;
}
