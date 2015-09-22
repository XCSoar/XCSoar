/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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
#include "OS/FileUtil.hpp"
#include "IO/ZipSource.hpp"
#include "IO/TextFile.hpp"
#include "IO/LineReader.hpp"

#include <string.h>

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

static WaypointReaderBase *
CreateWaypointReader(const TCHAR *path, WaypointFactory factory)
{
  return CreateWaypointReader(DetermineWaypointFileType(path), factory);
}

bool
ReadWaypointFile(const TCHAR *path, Waypoints &way_points,
                 WaypointFactory factory, OperationEnvironment &operation)
{
  auto *reader = CreateWaypointReader(path, factory);
  if (reader == nullptr)
    return false;

  bool success = false;

  auto *line_reader = OpenTextFile(path, Charset::AUTO);
  if (line_reader != nullptr) {
    reader->Parse(way_points, *line_reader, operation);
    delete line_reader;
    success = true;
  }

  delete reader;
  return success;
}
