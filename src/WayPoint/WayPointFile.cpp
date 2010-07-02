/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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
#include "Terrain/RasterMap.hpp"
#include "Waypoint/Waypoints.hpp"
#include "Dialogs.h"
#include "Profile.hpp"
#include "Language.hpp"
#include "Profile.hpp"
#include "LocalPath.hpp"
#include "StringUtil.hpp"
#include "UtilsFile.hpp"
#include "Interface.hpp"
#include "IO/FileLineReader.hpp"
#include "IO/ZipLineReader.hpp"
#include "IO/TextWriter.hpp"

#include "wcecompat/ts_string.h"

int WayPointFile::WaypointsOutOfRangeSetting = 0;
int WayPointFile::WaypointOutOfTerrainRangeDialogResult = 0;
bool WayPointFile::initialised_range_setting = false;

WayPointFile::WayPointFile(const TCHAR* file_name, const int _file_num,
                           const bool _compressed): 
  file_num(_file_num),
  compressed(_compressed)
{
  _tcscpy(file, file_name);

  if (!initialised_range_setting) {
    if (WaypointsOutOfRangeSetting == 1)
      WaypointOutOfTerrainRangeDialogResult = wpTerrainBoundsYesAll;
    if (WaypointsOutOfRangeSetting == 2)
      WaypointOutOfTerrainRangeDialogResult = wpTerrainBoundsNoAll;
    initialised_range_setting = true;
  }
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
WayPointFile::AltitudeFromTerrain(GEOPOINT &location,
                                  const RasterTerrain &terrain)
{
  // If terrain not loaded yet -> return INVALID
  if (!terrain.GetMap())
    return RasterTerrain::TERRAIN_INVALID;

  // Get terrain height
  RasterRounding rounding(*terrain.GetMap());
  short alt = terrain.GetTerrainHeight(location, rounding);

  // If terrain altitude okay -> return terrain altitude
  if (alt > RasterTerrain::TERRAIN_INVALID)
    return alt;

  return RasterTerrain::TERRAIN_INVALID;
}


void
WayPointFile::add_waypoint_if_in_range(Waypoints &way_points, 
                                       const Waypoint &new_waypoint,
                                       const RasterTerrain *terrain)
{
  // if waypoint out of terrain range and should not be included
  // -> return without error condition
  if (terrain != NULL && !checkWaypointInTerrainRange(new_waypoint, *terrain))
    return;

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
  if (terrain == NULL)
    return;

  // Load waypoint altitude from terrain
  const short t_alt = AltitudeFromTerrain(new_waypoint.Location, *terrain);
  if (t_alt == RasterTerrain::TERRAIN_INVALID) {
    if (!alt_ok)
      new_waypoint.Altitude = fixed_zero;
  } else { // TERRAIN_VALID
    if (!alt_ok || abs((fixed)t_alt - new_waypoint.Altitude) > fixed(100))
    new_waypoint.Altitude = (fixed)t_alt;
  }
}

bool
WayPointFile::checkWaypointInTerrainRange(const Waypoint &way_point,
                                          const RasterTerrain &terrain)
{
  TCHAR sTmp[250];

  // If (YesToAll) -> include waypoint
  if (WaypointOutOfTerrainRangeDialogResult == wpTerrainBoundsYesAll)
    return true;

  // If (No Terrain) -> include waypoint
  if (!terrain.isTerrainLoaded())
    return true;

  // If (Waypoint in Terrain range) -> include waypoint
  if (terrain.WaypointIsInTerrainRange(way_point.Location))
    return true;

  // If (NoToAll) -> dont include waypoint
  if (WaypointOutOfTerrainRangeDialogResult == wpTerrainBoundsNoAll)
    return false;

  // Open Dialogbox
  _stprintf(sTmp,
            gettext(_T("Waypoint \"%s\" \r\nout of Terrain bounds\r\n\r\nLoad anyway?")),
            way_point.Name.c_str());

  WaypointOutOfTerrainRangeDialogResult = dlgWaypointOutOfTerrain(sTmp);

  // Execute result
  switch (WaypointOutOfTerrainRangeDialogResult) {
  case wpTerrainBoundsYesAll:
    Profile::Set(szProfileWaypointsOutOfRange, 1);
    Profile::Save();
    return true;

  case wpTerrainBoundsNoAll:
    Profile::Set(szProfileWaypointsOutOfRange, 2);
    Profile::Save();
    return false;

  case wpTerrainBoundsYes:
    return true;

  default:
  case mrCancel:
  case wpTerrainBoundsNo:
    WaypointOutOfTerrainRangeDialogResult = wpTerrainBoundsNo;
    return false;
  }
}


bool
WayPointFile::Parse(Waypoints &way_points, 
                    const RasterTerrain *terrain)
{
  // If no file loaded yet -> return false
  if (file[0] == 0)
    return false;

  XCSoarInterface::CreateProgressDialog(gettext(TEXT("Loading Waypoints...")));

  // If normal file
  if (!compressed) {
    // Try to open waypoint file
    FileLineReader reader(file);
    if (reader.error())
      return false;

    double filesize = std::max(reader.size(), 1l);
    XCSoarInterface::SetProgressDialogMaxValue(100);

    // Read through the lines of the file
    TCHAR *line;
    for (unsigned i = 0; (line = reader.read()) != NULL; i++) {
      // and parse them
      parseLine(line, i, way_points, terrain);

      unsigned status = reader.tell() * 100 / filesize;
      XCSoarInterface::SetProgressDialogValue(status);
    }
  // If compressed file inside map file
  } else {
    // convert path to ascii
    ZipLineReader reader(file);
    if (reader.error())
      return false;

    double filesize = std::max(reader.size(), 1l);
    XCSoarInterface::SetProgressDialogMaxValue(100);

    // Read through the lines of the file
    TCHAR *line;
    for (unsigned i = 0; (line = reader.read()) != NULL; i++) {
      // and parse them
      parseLine(line, i, way_points, terrain);

      unsigned status = reader.tell() * 100 / filesize;
      XCSoarInterface::SetProgressDialogValue(status);
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

  TCHAR file[256];

  // Copy the filename to the internal field
  _tcscpy(file, filename);
  // and convert it to filepath
  ExpandLocalPath(file);

  // Convert the filepath from unicode to ascii for zzip files
  char path_ascii[MAX_PATH];
  unicode2ascii(file, path_ascii, sizeof(path_ascii));

  // check existence of file
  if (!FileExists(file)) {
    if (!FileExistsZipped(path_ascii)) {
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
    return new WayPointFileWinPilot(file, the_filenum, compressed);
  }

  // If SeeYou waypoint file -> save type and return true
  if (MatchesExtension(filename, _T(".cup"))) {
    return new WayPointFileSeeYou(file, the_filenum, compressed);
  }

  // If Zander waypoint file -> save type and return true
  if (MatchesExtension(filename, _T(".wpz"))) {
    return new WayPointFileZander(file, the_filenum, compressed);
  }

  // unknown
  return NULL;
}
