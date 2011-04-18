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

#include "WaypointReader.hpp"

#include "WayPointFileZander.hpp"
#include "WayPointFileSeeYou.hpp"
#include "WayPointFileWinPilot.hpp"
#include "WayPointFileFS.hpp"

#include "Terrain/RasterTerrain.hpp"
#include "Waypoint/Waypoints.hpp"
#include "OS/FileUtil.hpp"
#include "UtilsFile.hpp"
#include "IO/ZipSource.hpp"

bool
WaypointReader::Parse(Waypoints &way_points)
{
  if (file == NULL)
    return false;

  return file->Parse(way_points);
}

void
WaypointReader::SetTerrain(const RasterTerrain* _terrain)
{
  if (file != NULL)
    file->SetTerrain(_terrain);
}

void
WaypointReader::Open(const TCHAR* filename, int the_filenum)
{
  delete file;
  file = NULL;

  // If filename is empty -> clear and return NULL pointer
  if (string_is_empty(filename))
    return;

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

  // If WinPilot waypoint file -> save type and return true
  if (MatchesExtension(filename, _T(".dat")) ||
      MatchesExtension(filename, _T(".xcw")))
    file = new WayPointFileWinPilot(filename, the_filenum, compressed);

  // If SeeYou waypoint file -> save type and return true
  else if (MatchesExtension(filename, _T(".cup")))
    file = new WayPointFileSeeYou(filename, the_filenum, compressed);

  // If Zander waypoint file -> save type and return true
  else if (MatchesExtension(filename, _T(".wpz")))
    file = new WayPointFileZander(filename, the_filenum, compressed);

  // If FS waypoint file -> save type and return true
  else if (MatchesExtension(filename, _T(".wpt")))
    file = new WayPointFileFS(filename, the_filenum, compressed);
}

WaypointReader::WaypointReader()
  :file(NULL) {}

WaypointReader::WaypointReader(const TCHAR* filename, int the_filenum)
  :file(NULL)
{
  Open(filename, the_filenum);
}

WaypointReader::~WaypointReader()
{
  delete file;
}
