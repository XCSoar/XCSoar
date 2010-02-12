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

#include "WayPointParser.h"
#include "DeviceBlackboard.hpp"
#include "Dialogs.h"
#include "Language.hpp"
#include "Registry.hpp"
#include "Profile.hpp"
#include "LocalPath.hpp"
#include "UtilsText.hpp"
#include "StringUtil.hpp"
#include "RasterTerrain.h"
#include "RasterMap.h"
#include "LogFile.hpp"
#include "Waypoint/Waypoints.hpp"
#include "UtilsFile.hpp"
#include "Units.hpp"

#include <algorithm>
using std::min;
using std::max;

#include <tchar.h>
#include <stdio.h>

#ifdef HAVE_POSIX
#include <errno.h>
#endif

#include "wcecompat/ts_string.h"

int WayPointParser::WaypointsOutOfRangeSetting = 0;

/**
 * This functions checks if the home, alternate 1/2 and teamcode waypoint
 * indices exist and if necessary tries to find new ones in the waypoint list
 * @param way_points Waypoint list
 * @param terrain RasterTerrain (for placing the aircraft
 * in the middle of the terrain if no home was found)
 * @param settings SETTING_COMPUTER (for determining the
 * special waypoint indices)
 * @param reset This should be true if the waypoint file was changed,
 * it resets all special waypoints indices
 * @param set_location If true, the SetStartupLocation function will be called
 */
void
SetHome(const Waypoints &way_points, const RasterTerrain *terrain,
        SETTINGS_COMPUTER &settings,
        const bool reset, const bool set_location)
{
  StartupStore(TEXT("SetHome\n"));

  // check invalid home waypoint or forced reset due to file change
  if (reset || way_points.empty() ||
      !way_points.lookup_id(settings.HomeWaypoint)) {
    // search for home in waypoint list, if we don't have a home
    settings.HomeWaypoint = -1;

    const Waypoint* wp = way_points.find_home();
    if (wp)
      settings.HomeWaypoint = wp->id;
  }

  // reset Alternates
  if (reset || way_points.empty() ||
      !way_points.lookup_id(settings.Alternate1) ||
      !way_points.lookup_id(settings.Alternate2)) {
    settings.Alternate1 = -1;
    settings.Alternate2 = -1;
  }

  // check invalid task ref waypoint or forced reset due to file change
  if (reset || way_points.empty() ||
      !way_points.lookup_id(settings.TeamCodeRefWaypoint))
    // set team code reference waypoint if we don't have one
    settings.TeamCodeRefWaypoint = settings.HomeWaypoint;

  if (set_location) {
    if (const Waypoint *wp = way_points.lookup_id(settings.HomeWaypoint)) {
      // OK, passed all checks now
      StartupStore(TEXT("Start at home waypoint\n"));
      device_blackboard.SetStartupLocation(wp->Location, wp->Altitude);
    } else if (terrain != NULL) {
      // no home at all, so set it from center of terrain if available
      GEOPOINT loc;
      if (terrain->GetTerrainCenter(&loc)) {
        StartupStore(TEXT("Start at terrain center\n"));
        device_blackboard.SetStartupLocation(loc, 0);
      }
    }
  }

  //
  // Save the home waypoint number in the resgistry
  //
  // VENTA3> this is probably useless, since HomeWayPoint &c were currently
  //         just loaded from registry.
  SetToRegistry(szRegistryHomeWaypoint,settings.HomeWaypoint);
  SetToRegistry(szRegistryAlternate1,settings.Alternate1);
  SetToRegistry(szRegistryAlternate2,settings.Alternate2);
  SetToRegistry(szRegistryTeamcodeRefWaypoint,settings.TeamCodeRefWaypoint);
}

WayPointParser::WayPointParser()
{
  if (WaypointsOutOfRangeSetting == 1)
    WaypointOutOfTerrainRangeDialogResult = wpTerrainBoundsYesAll;
  if (WaypointsOutOfRangeSetting == 2)
    WaypointOutOfTerrainRangeDialogResult = wpTerrainBoundsNoAll;

  ClearFile();
}

bool
WayPointParser::ReadWaypoints(Waypoints &way_points,
                              const RasterTerrain *terrain)
{
  StartupStore(TEXT("ReadWaypoints\n"));

  TCHAR szFile[MAX_PATH];
  bool loaded = false;
  WayPointParser parser;

  // Delete old waypoints
  CloseWaypoints(way_points);

  // ### FIRST FILE ###

  // Get first waypoint filename
  GetRegistryString(szRegistryWayPointFile, szFile, MAX_PATH);
  // and clear registry setting (if loading goes totally wrong)
  SetRegistryString(szRegistryWayPointFile, TEXT("\0"));

  // If waypoint file exists
  if (parser.SetFile(szFile, true, 0)) {
    // parse the file
    if (parser.Parse(way_points, terrain)) {
      // reset the registry to the actual file name
      SetRegistryString(szRegistryWayPointFile, szFile);

      // Set waypoints writable flag
      way_points.set_file0_writable(parser.IsWritable());
      loaded = true;
    } else {
      StartupStore(TEXT("Parse error in waypoint file 1\n"));
    }
  } else {
    StartupStore(TEXT("No waypoint file 1\n"));
  }

  // ### SECOND FILE ###

  // Get second waypoint filename
  GetRegistryString(szRegistryAdditionalWayPointFile, szFile, MAX_PATH);
  // and clear registry setting (if loading goes totally wrong)
  SetRegistryString(szRegistryAdditionalWayPointFile, TEXT("\0"));

  // If waypoint file exists
  if (parser.SetFile(szFile, true, 1)) {
    // parse the file
    if (parser.Parse(way_points, terrain)) {
      // reset the registry to the actual file name
      SetRegistryString(szRegistryWayPointFile, szFile);

      // Set waypoints writable flag
      way_points.set_file0_writable(!loaded && parser.IsWritable());
      loaded = true;
    } else {
      StartupStore(TEXT("Parse error in waypoint file 2\n"));
    }
  } else {
    StartupStore(TEXT("No waypoint file 2\n"));
  }

  // ### MAP/THIRD FILE ###

  // If no waypoint file loaded yet
  if (!loaded) {
    // Get the map filename
    GetRegistryString(szRegistryMapFile, szFile, MAX_PATH);
    _tcscat(szFile, TEXT("/"));
    _tcscat(szFile, TEXT("waypoints.xcw"));

    // If waypoint file inside map file exists
    if (parser.SetFile(szFile, true, 2)) {
      // parse the file
      if (parser.Parse(way_points, terrain)) {
        // reset the registry to the actual file name
        SetRegistryString(szRegistryWayPointFile, szFile);

        // Set waypoints writable flag
        way_points.set_file0_writable(false);
        loaded = true;
      } else {
        StartupStore(TEXT("Parse error in map waypoint file\n"));
      }
    } else {
      StartupStore(TEXT("No waypoint file in the map file\n"));
    }
  }

  // Optimise the waypoint list after attaching new waypoints
  way_points.optimise();

  // Return whether waypoints have been loaded into the waypoint list
  return loaded;
}

void
WayPointParser::SaveWaypoints(Waypoints &way_points)
{
  StartupStore(TEXT("SaveWaypoints\n"));

  TCHAR szFile[MAX_PATH];
  WayPointParser parser;

  // ### FIRST FILE ###

  // Get first waypoint filename
  GetRegistryString(szRegistryWayPointFile, szFile, MAX_PATH);

  // If waypoint file seems okay
  if (parser.SetFile(szFile, false, 0)) {
    // Save the file
    if (!parser.Save(way_points))
      StartupStore(TEXT("Save error in waypoint file 1\n"));
  } else {
    StartupStore(TEXT("Waypoint file 1 can not be written\n"));
  }

  // ### SECOND FILE ###

  // Get second waypoint filename
  GetRegistryString(szRegistryAdditionalWayPointFile, szFile, MAX_PATH);

  // If waypoint file seems okay
  if (parser.SetFile(szFile, false, 1)) {
    // Save the file
    if (!parser.Save(way_points))
      StartupStore(TEXT("Save error in waypoint file 2\n"));
  } else {
    StartupStore(TEXT("Waypoint file 2 can not be written\n"));
  }
}

void
WayPointParser::CloseWaypoints(Waypoints &way_points)
{
  // Clear the waypoint list
  way_points.clear();
}

bool
WayPointParser::SetFile(TCHAR* filename, bool returnOnFileMissing, int filenum)
{
  // If filename is empty -> clear and return false
  if (string_is_empty(filename)) {
    ClearFile();
    return false;
  }

  // Save the filenumber
  this->filenum = filenum;

  // Copy the filename to the internal field
  _tcscpy(file, filename);
  // and convert it to filepath
  ExpandLocalPath(file);

  // Convert the filepath from unicode to ascii for zzip files
  char path_ascii[MAX_PATH];
  unicode2ascii(file, path_ascii, sizeof(path_ascii));

  // If file does not exist -> clear and return true
  if (returnOnFileMissing &&
      !FileExists(file) &&
      !FileExistsZipped(path_ascii)) {
    ClearFile();
    return false;
  }

  // If file does not exist but exists inside map file -> save compressed flag
  if (!FileExists(file) &&
      FileExistsZipped(path_ascii))
    compressed = true;

  // If WinPilot waypoint file -> save type and return true
  if (MatchesExtension(filename, _T(".dat")) ||
      MatchesExtension(filename, _T(".xcw"))) {
    filetype = ftWinPilot;
    return true;
  }

  // If SeeYou waypoint file -> save type and return true
  if (MatchesExtension(filename, _T(".cup"))) {
    filetype = ftSeeYou;
    return true;
  }

  // If Zander waypoint file -> save type and return true
  if (MatchesExtension(filename, _T(".wpz"))) {
    filetype = ftZander;
    return true;
  }

  // If unknown file -> clear and return false
  ClearFile();
  return false;
}

void
WayPointParser::ClearFile()
{
  file[0] = 0;
  compressed = false;
}

bool
WayPointParser::Parse(Waypoints &way_points, const RasterTerrain *terrain)
{
  // If no file loaded yet -> return false
  if (file[0] == 0)
    return false;

  TCHAR line[255];

  // If normal file
  if (!compressed) {
    // Try to open waypoint file
    FILE *fp;
    fp = _tfopen(file, _T("rt"));
    if (fp == NULL)
      return false;

    // Read through the lines of the file
    for (unsigned i = 0; ReadStringX(fp, 255, line); i++) {
      // and parse them
      parseLine(line, i, way_points, terrain);
    }

    fclose(fp);

  // If compressed file inside map file
  } else {
    // convert path to ascii
    char path_ascii[MAX_PATH];
    unicode2ascii(file, path_ascii, sizeof(path_ascii));

    // Try to open compressed waypoint file inside map file
    ZZIP_FILE *fp;
    fp = zzip_fopen(path_ascii, "rt");
    if (fp == NULL)
      return false;

    // Read through the lines of the file
    for (unsigned i = 0; ReadString(fp, 255, line); i++) {
      // and parse them
      parseLine(line, i, way_points, terrain);
    }

    zzip_fclose(fp);
  }

  return true;
}

bool
WayPointParser::Save(Waypoints &way_points)
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
  FILE *fp;
  fp = _tfopen(file, _T("wt"));
  if (fp == NULL)
    return false;

  // Call the saveFile function depending on the file type
  switch (filetype) {
    case ftWinPilot:
      saveFileWinPilot(fp, way_points);
      break;
  }

  // Close the file
  fclose(fp);

  // and tell everyone we saved successfully
  return true;
}

bool
WayPointParser::parseLine(const TCHAR* line, unsigned linenum,
    Waypoints &way_points, const RasterTerrain *terrain)
{
  // Hand the parsing over to the right parser depending on the file type
  switch (filetype) {
    case ftWinPilot:
      return parseLineWinPilot(line, linenum, way_points, terrain);
    case ftSeeYou:
      return parseLineSeeYou(line, linenum, way_points, terrain);
    case ftZander:
      return parseLineZander(line, linenum, way_points, terrain);
  }

  // Return false if the file type is not known
  return false;
}


void
WayPointParser::setDefaultFlags(WaypointFlags& dest, bool turnpoint)
{
  dest.Airport = false;
  dest.TurnPoint = turnpoint;
  dest.LandPoint = false;
  dest.Home = false;
  dest.StartPoint = false;
  dest.FinishPoint = false;
  dest.Restricted = false;
  dest.WaypointFlag = false;
}

bool
WayPointParser::checkWaypointInTerrainRange(const Waypoint &way_point,
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
  _stprintf(sTmp, gettext(_T(
      "Waypoint #%d \"%s\" \r\nout of Terrain bounds\r\n\r\nLoad anyway?")),
      way_point.id, way_point.Name.c_str());

  WaypointOutOfTerrainRangeDialogResult = dlgWaypointOutOfTerrain(sTmp);

  // Execute result
  switch (WaypointOutOfTerrainRangeDialogResult) {
  case wpTerrainBoundsYesAll:
    SetToRegistry(szRegistryWaypointsOutOfRange, 1);
    Profile::StoreRegistry();
    return true;

  case wpTerrainBoundsNoAll:
    SetToRegistry(szRegistryWaypointsOutOfRange, 2);
    Profile::StoreRegistry();
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

size_t
WayPointParser::extractParameters(const TCHAR *src, TCHAR *dst,
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

double
WayPointParser::AltitudeFromTerrain(GEOPOINT &location,
                                    const RasterTerrain &terrain)
{
  double alt = TERRAIN_INVALID;

  // If terrain not loaded yet -> return INVALID
  if (!terrain.GetMap())
    return TERRAIN_INVALID;

  // Get terrain height
  RasterRounding rounding(*terrain.GetMap(), 0, 0);
  alt = terrain.GetTerrainHeight(location, rounding);

  // If terrain altitude okay -> return terrain altitude
  if (alt > TERRAIN_INVALID)
    return alt;

  return TERRAIN_INVALID;
}
