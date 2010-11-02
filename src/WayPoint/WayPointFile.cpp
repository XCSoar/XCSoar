/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#include "WayPointFile.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "Waypoint/Waypoints.hpp"
#include "UtilsFile.hpp"
#include "OS/FileUtil.hpp"
#include "ProgressGlue.hpp"
#include "IO/FileLineReader.hpp"
#include "IO/ZipLineReader.hpp"
#include "IO/TextWriter.hpp"

WayPointFile::WayPointFile(const TCHAR* file_name, const int _file_num,
                           const bool _compressed): 
  file_num(_file_num),
  compressed(_compressed)
{
  _tcscpy(file, file_name);
}


size_t
WayPointFile::extractParameters(const TCHAR *src, TCHAR *dst,
                                const TCHAR **arr, size_t sz)
{
  TCHAR c, *p;
  size_t i = 0;

  _tcscpy(dst, src);
  p = dst;

  do {
    arr[i++] = p;
    p = _tcschr(p, _T(','));
    if (!p)
      break;
    c = *p;
    *p++ = _T('\0');
  } while (i != sz && c != _T('\0'));

  return i;
}

short
WayPointFile::AltitudeFromTerrain(GeoPoint &location,
                                  const RasterTerrain &terrain)
{
  return terrain.GetTerrainHeight(location);
}


void
WayPointFile::add_waypoint(Waypoints &way_points,
                           const Waypoint &new_waypoint)
{
  // Append the new waypoint to the waypoint list and
  // return successful line parse
  Waypoint wp(new_waypoint);
  way_points.append(wp);
  return;
}

void
WayPointFile::check_altitude(Waypoint &new_waypoint,
                             const RasterTerrain *terrain,
                             bool alt_ok)
{
  if (terrain == NULL || alt_ok)
    return;

  // Load waypoint altitude from terrain
  const short t_alt = AltitudeFromTerrain(new_waypoint.Location, *terrain);
  if (t_alt == RasterTerrain::TERRAIN_INVALID) {
    new_waypoint.Altitude = fixed_zero;
  } else { // TERRAIN_VALID
    new_waypoint.Altitude = (fixed)t_alt;
  }
}

bool
WayPointFile::Parse(Waypoints &way_points, 
                    const RasterTerrain *terrain)
{
  // If no file loaded yet -> return false
  if (file[0] == 0)
    return false;

  ProgressGlue::SetRange(25);

  // If normal file
  if (!compressed) {
    // Try to open waypoint file
    FileLineReader reader(file);
    if (reader.error())
      return false;

    double filesize = std::max(reader.size(), 1l);

    // Read through the lines of the file
    TCHAR *line;
    for (unsigned i = 0; (line = reader.read()) != NULL; i++) {
      // and parse them
      parseLine(line, i, way_points, terrain);

      unsigned status = reader.tell() * 25 / filesize;
      ProgressGlue::SetValue(status);
    }
  // If compressed file inside map file
  } else {
    // convert path to ascii
    ZipLineReader reader(file);
    if (reader.error())
      return false;

    double filesize = std::max(reader.size(), 1l);

    // Read through the lines of the file
    TCHAR *line;
    for (unsigned i = 0; (line = reader.read()) != NULL; i++) {
      // and parse them
      parseLine(line, i, way_points, terrain);

      unsigned status = reader.tell() * 25 / filesize;
      ProgressGlue::SetValue(status);
    }
  }

  return true;
}

bool
WayPointFile::Save(const Waypoints &way_points)
{
  // No filename -> return
  if (file[0] == 0)
    return false;
  // Not writable -> return
  if (!IsWritable())
    return false;
  // Compressed file -> return
  if (compressed)
    return false;

  // Try to open waypoint file for writing
  TextWriter writer(file);
  if (writer.error())
    return false;

  saveFile(writer, way_points);

  // and tell everyone we saved successfully
  return true;
}



#include "WayPointFileZander.hpp"
#include "WayPointFileSeeYou.hpp"
#include "WayPointFileWinPilot.hpp"

WayPointFile*
WayPointFile::create(const TCHAR* filename, int the_filenum)
{
  bool compressed = false;

  // If filename is empty -> clear and return false
  if (string_is_empty(filename)) {
    return NULL;
  }

  // check existence of file
  if (!File::Exists(filename)) {
    ZipSource zip(filename);
    if (zip.error()) {
      // File does not exist, fail
      return NULL;
    } else {
      // File does exist inside map file -> save compressed flag
      compressed = true;
    }
  }

  // If WinPilot waypoint file -> save type and return true
  if (MatchesExtension(filename, _T(".dat")) ||
      MatchesExtension(filename, _T(".xcw"))) {
    return new WayPointFileWinPilot(filename, the_filenum, compressed);
  }

  // If SeeYou waypoint file -> save type and return true
  if (MatchesExtension(filename, _T(".cup"))) {
    return new WayPointFileSeeYou(filename, the_filenum, compressed);
  }

  // If Zander waypoint file -> save type and return true
  if (MatchesExtension(filename, _T(".wpz"))) {
    return new WayPointFileZander(filename, the_filenum, compressed);
  }

  // unknown
  return NULL;
}
