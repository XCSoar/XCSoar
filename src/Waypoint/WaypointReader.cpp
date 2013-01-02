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

#include "WaypointReader.hpp"

#include "WaypointReaderZander.hpp"
#include "WaypointReaderSeeYou.hpp"
#include "WaypointReaderWinPilot.hpp"
#include "WaypointReaderFS.hpp"
#include "WaypointReaderOzi.hpp"
#include "WaypointReaderCompeGPS.hpp"
#include "WaypointFileType.hpp"

#include "Terrain/RasterTerrain.hpp"
#include "Waypoint/Waypoints.hpp"
#include "OS/FileUtil.hpp"
#include "IO/ZipSource.hpp"
#include "IO/TextFile.hpp"

bool
WaypointReader::Parse(Waypoints &way_points, OperationEnvironment &operation)
{
  if (reader == NULL)
    return false;

  TLineReader *line_reader = OpenTextFile(path, ConvertLineReader::AUTO);
  if (line_reader == nullptr)
    return false;

  reader->Parse(way_points, *line_reader, operation);
  delete line_reader;
  return true;
}

void
WaypointReader::SetTerrain(const RasterTerrain* _terrain)
{
  if (reader != NULL)
    reader->SetTerrain(_terrain);
}

void
WaypointReader::Open(const TCHAR* filename, int the_filenum)
{
  delete reader;
  reader = NULL;

  // If filename is empty -> clear and return NULL pointer
  if (StringIsEmpty(filename))
    return;

  _tcscpy(path, filename);

  // Test if file exists
  bool compressed = false;
  if (!File::Exists(filename)) {
    compressed = true;
    // Test if file exists in zip archive
    ZipSource zip(filename);
    if (zip.error())
      // If the file doesn't exist return NULL pointer
      return;
  }

  switch (DetermineWaypointFileType(filename)) {
  case WaypointFileType::WINPILOT:
    reader = new WaypointReaderWinPilot(the_filenum, compressed);
    break;

  case WaypointFileType::SEEYOU:
    reader = new WaypointReaderSeeYou(the_filenum, compressed);
    break;

  case WaypointFileType::ZANDER:
    reader = new WaypointReaderZander(the_filenum, compressed);
    break;

  case WaypointFileType::FS:
    reader = new WaypointReaderFS(the_filenum, compressed);
    break;

  case WaypointFileType::OZI_EXPLORER:
    reader = new WaypointReaderOzi(the_filenum, compressed);
    break;

  case WaypointFileType::COMPE_GPS:
    reader = new WaypointReaderCompeGPS(the_filenum, compressed);
    break;

  default:
    break;
  }
}
