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

#include "WayPointGlue.hpp"
#include "SettingsComputer.hpp"
#include "DeviceBlackboard.hpp"
#include "Profile/Profile.hpp"
#include "StringUtil.hpp"
#include "UtilsFile.hpp"
#include "LogFile.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "Waypoint/Waypoints.hpp"
#include "WaypointReader.hpp"
#include "Language.hpp"
#include "ProgressGlue.hpp"
#include "Components.hpp"
#include "NMEA/Aircraft.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"
#include "IO/TextWriter.hpp"
#include "WayPoint/WaypointWriter.hpp"

#include <windef.h> /* for MAX_PATH */

namespace WayPointGlue {
  bool GetPath(int file_number, TCHAR *value);
  bool IsWritable(int file_number);
}

bool
WayPointGlue::GetPath(int file_number, TCHAR *value)
{
  const TCHAR *key;

  switch (file_number) {
  case 1:
    key = szProfileWayPointFile;
    break;
  case 2:
    key = szProfileAdditionalWayPointFile;
    break;
  case 3:
    key = szProfileWatchedWayPointFile;
    break;
  default:
    return false;
  }

  return Profile::GetPath(key, value);
}

bool
WayPointGlue::IsWritable(int file_number)
{
  TCHAR file[255];
  if (!GetPath(file_number, file))
    return false;

  return (MatchesExtension(file, _T(".dat")) ||
          MatchesExtension(file, _T(".xcw")));
}

/**
 * This functions checks if the home and teamcode waypoint
 * indices exist and if necessary tries to find new ones in the waypoint list
 * @param way_points Waypoint list
 * @param terrain RasterTerrain (for placing the aircraft
 * in the middle of the terrain if no home was found)
 * @param settings SETTING_COMPUTER (for determining the
 * special waypoint indices)
 * @param reset This should be true if the waypoint file was changed,
 * it resets all special waypoints indices
 */
void
WayPointGlue::SetHome(Waypoints &way_points, const RasterTerrain *terrain,
                      SETTINGS_COMPUTER &settings,
                      const bool reset)
{
  LogStartUp(_T("SetHome"));

  // check invalid home waypoint or forced reset due to file change
  const Waypoint *wp = reset ? NULL : way_points.lookup_id(settings.HomeWaypoint);
  if (wp == NULL && settings.HomeLocationAvailable) {
    /* fall back to HomeLocation, try to find it in the waypoint
       database */
    wp = way_points.lookup_location(settings.HomeLocation, fixed(100));
    if (wp != NULL && wp->is_airport())
      settings.SetHome(*wp);
  }

  if (wp != NULL) {
    // home waypoint found
    way_points.set_home(settings.HomeWaypoint);
  } else {
    // search for home in waypoint list, if we don't have a home
    wp = way_points.find_home();
    if (wp != NULL)
      settings.SetHome(*wp);
    else
      settings.ClearHome();
  }

  // check invalid task ref waypoint or forced reset due to file change
  if (reset || way_points.empty() ||
      !way_points.lookup_id(settings.TeamCodeRefWaypoint))
    // set team code reference waypoint if we don't have one
    settings.TeamCodeRefWaypoint = settings.HomeWaypoint;

  if (const Waypoint *wp = way_points.lookup_id(settings.HomeWaypoint)) {
    // OK, passed all checks now
    LogStartUp(_T("Start at home waypoint"));
    device_blackboard.SetStartupLocation(wp->Location, wp->Altitude);
  } else if (terrain != NULL) {
    // no home at all, so set it from center of terrain if available
    GeoPoint loc = terrain->GetTerrainCenter();
    LogStartUp(_T("Start at terrain center"));
    device_blackboard.SetStartupLocation(loc, fixed_zero);
  }

  // Save the home waypoint number in the resgistry
  Profile::Set(szProfileHomeWaypoint,settings.HomeWaypoint);
  if (settings.HomeLocationAvailable)
    Profile::SetGeoPoint(szProfileHomeLocation, settings.HomeLocation);

  Profile::Set(szProfileTeamcodeRefWaypoint,settings.TeamCodeRefWaypoint);
}

bool
WayPointGlue::LoadWaypointFile(int num, Waypoints &way_points,
                               const RasterTerrain *terrain)
{
  // Get waypoint filename
  TCHAR szFile[MAX_PATH];
  if (!GetPath(num, szFile))
    return false;

  WaypointReader reader(szFile, num);

  // If waypoint file exists
  if (!reader.Error()) {
    // parse the file
    reader.SetTerrain(terrain);
    if (reader.Parse(way_points))
      return true;

    LogStartUp(_T("Parse error in waypoint file %d"), num);
  } else {
    LogStartUp(_T("No waypoint file %d"), num);
  }

  return false;
}

bool
WayPointGlue::LoadMapFileWaypoints(int num, const TCHAR* key,
                                   Waypoints &way_points, const RasterTerrain *terrain)
{
  TCHAR szFile[MAX_PATH];

  // Get the map filename
  Profile::GetPath(key, szFile);
  _tcscat(szFile, _T("/"));
  _tcscat(szFile, _T("waypoints.xcw"));

  WaypointReader reader(szFile, num);

  // Test if waypoints.xcw can be loaded, otherwise try waypoints.cup
  if (reader.Error()) {
    // Get the map filename
    Profile::GetPath(key, szFile);
    _tcscat(szFile, _T("/"));
    _tcscat(szFile, _T("waypoints.cup"));

    reader.Open(szFile, num);
  }

  // If waypoint file inside map file exists
  if (!reader.Error()) {
    // parse the file
    reader.SetTerrain(terrain);
    if (reader.Parse(way_points))
      return true;

    LogStartUp(_T("Parse error in map waypoint file"));
  } else {
    LogStartUp(_T("No waypoint file in the map file"));
  }

  return false;
}

bool
WayPointGlue::LoadWaypoints(Waypoints &way_points,
                            const RasterTerrain *terrain)
{
  LogStartUp(_T("ReadWaypoints"));
  ProgressGlue::Create(_("Loading Waypoints..."));

  bool found = false;

  // Delete old waypoints
  way_points.clear();

  // ### FIRST FILE ###
  found |= LoadWaypointFile(1, way_points, terrain);

  // ### SECOND FILE ###
  found |= LoadWaypointFile(2, way_points, terrain);

  // ### WATCHED WAYPOINT/THIRD FILE ###
  found |= LoadWaypointFile(3, way_points, terrain);

  // ### MAP/FOURTH FILE ###

  // If no waypoint file found yet
  if (!found)
    found = LoadMapFileWaypoints(0, szProfileMapFile, way_points, terrain);

  // Optimise the waypoint list after attaching new waypoints
  way_points.optimise();

  // Return whether waypoints have been loaded into the waypoint list
  return found;
}

bool
WayPointGlue::SaveWaypointFile(const Waypoints &way_points, int num)
{
  if (!IsWritable(num)) {
    LogStartUp(_T("Waypoint file %d can not be written"), num);
    return false;
  }

  TCHAR file[255];
  GetPath(num, file);

  TextWriter writer(file);
  if (writer.error()) {
    LogStartUp(_T("Waypoint file %d can not be written"), num);
    return false;
  }

  WaypointWriter wp_writer(way_points, num);
  wp_writer.Save(writer);

  LogStartUp(_T("Waypoint file %d saved"), num);
  return true;
}

bool
WayPointGlue::SaveWaypoints(const Waypoints &way_points)
{
  bool result = false;
  LogStartUp(_T("SaveWaypoints"));

  // ### FIRST FILE ###
  result |= SaveWaypointFile(way_points, 1);

  // ### SECOND FILE ###
  result |= SaveWaypointFile(way_points, 2);

  // ### THIRD FILE ###
  result |= SaveWaypointFile(way_points, 3);

  return result;
}
