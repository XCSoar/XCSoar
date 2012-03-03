/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "WaypointGlue.hpp"
#include "ComputerSettings.hpp"
#include "Blackboard/DeviceBlackboard.hpp"
#include "Profile/Profile.hpp"
#include "StringUtil.hpp"
#include "LogFile.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "Waypoint/Waypoints.hpp"
#include "WaypointReader.hpp"
#include "Language/Language.hpp"
#include "Components.hpp"
#include "NMEA/Aircraft.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"
#include "IO/TextWriter.hpp"
#include "OS/PathName.hpp"
#include "Waypoint/WaypointWriter.hpp"
#include "Operation/Operation.hpp"

#include <windef.h> /* for MAX_PATH */

namespace WaypointGlue {
  bool GetPath(int file_number, TCHAR *value);
  bool IsWritable(int file_number);
}

bool
WaypointGlue::GetPath(int file_number, TCHAR *value)
{
  const TCHAR *key;

  switch (file_number) {
  case 1:
    key = szProfileWaypointFile;
    break;
  case 2:
    key = szProfileAdditionalWaypointFile;
    break;
  case 3:
    key = szProfileWatchedWaypointFile;
    break;
  default:
    return false;
  }

  return Profile::GetPath(key, value);
}

bool
WaypointGlue::IsWritable(int file_number)
{
  TCHAR file[MAX_PATH];
  if (!GetPath(file_number, file))
    return false;

  return (MatchesExtension(file, _T(".dat")) ||
          MatchesExtension(file, _T(".xcw")));
}

bool
WaypointGlue::IsWritable()
{
  return IsWritable(1) || IsWritable(2) || IsWritable(3);
}

const Waypoint *
WaypointGlue::FindHomeId(Waypoints &waypoints,
                         PlacesOfInterestSettings &settings)
{
  if (settings.home_waypoint < 0)
    return NULL;

  const Waypoint *wp = waypoints.LookupId(settings.home_waypoint);
  if (wp == NULL) {
    settings.home_waypoint = -1;
    return NULL;
  }

  settings.home_location = wp->location;
  settings.home_location_available = true;
  waypoints.SetHome(wp->id);
  return wp;
}

const Waypoint *
WaypointGlue::FindHomeLocation(Waypoints &waypoints,
                               PlacesOfInterestSettings &settings)
{
  if (!settings.home_location_available)
    return NULL;

  const Waypoint *wp = waypoints.LookupLocation(settings.home_location,
                                                fixed(100));
  if (wp == NULL || !wp->IsAirport()) {
    settings.home_location_available = false;
    return NULL;
  }

  settings.home_waypoint = wp->id;
  waypoints.SetHome(wp->id);
  return wp;
}

const Waypoint *
WaypointGlue::FindFlaggedHome(Waypoints &waypoints,
                              PlacesOfInterestSettings &settings)
{
  const Waypoint *wp = waypoints.FindHome();
  if (wp == NULL)
    return NULL;

  settings.SetHome(*wp);
  return wp;
}

void
WaypointGlue::SetHome(Waypoints &way_points, const RasterTerrain *terrain,
                      ComputerSettings &settings,
                      const bool reset)
{
  LogStartUp(_T("SetHome"));

  if (reset)
    settings.poi.home_waypoint = -1;

  // check invalid home waypoint or forced reset due to file change
  const Waypoint *wp = FindHomeId(way_points, settings.poi);
  if (wp == NULL) {
    /* fall back to HomeLocation, try to find it in the waypoint
       database */
    wp = FindHomeLocation(way_points, settings.poi);
    if (wp == NULL)
      // search for home in waypoint list, if we don't have a home
      wp = FindFlaggedHome(way_points, settings.poi);
  }

  // check invalid task ref waypoint or forced reset due to file change
  if (reset || way_points.IsEmpty() ||
      !way_points.LookupId(settings.team_code.team_code_reference_waypoint))
    // set team code reference waypoint if we don't have one
    settings.team_code.team_code_reference_waypoint = settings.poi.home_waypoint;

  if (device_blackboard != NULL) {
    if (wp != NULL) {
      // OK, passed all checks now
      LogStartUp(_T("Start at home waypoint"));
      device_blackboard->SetStartupLocation(wp->location, wp->elevation);
    } else if (terrain != NULL) {
      // no home at all, so set it from center of terrain if available
      GeoPoint loc = terrain->GetTerrainCenter();
      LogStartUp(_T("Start at terrain center"));
      device_blackboard->SetStartupLocation(loc,
                                            fixed(terrain->GetTerrainHeight(loc)));
    }
  }
}

void
WaypointGlue::SaveHome(const ComputerSettings &settings)
{
  Profile::Set(szProfileHomeWaypoint, settings.poi.home_waypoint);
  if (settings.poi.home_location_available)
    Profile::SetGeoPoint(szProfileHomeLocation, settings.poi.home_location);

  Profile::Set(szProfileTeamcodeRefWaypoint,
               settings.team_code.team_code_reference_waypoint);
}

bool
WaypointGlue::LoadWaypointFile(int num, Waypoints &way_points,
                               const RasterTerrain *terrain,
                               OperationEnvironment &operation)
{
  // Get waypoint filename
  TCHAR path[MAX_PATH];
  if (!GetPath(num, path))
    return false;

  WaypointReader reader(path, num);

  // If waypoint file exists
  if (!reader.Error()) {
    // parse the file
    reader.SetTerrain(terrain);

    if (reader.Parse(way_points, operation))
      return true;

    LogStartUp(_T("Parse error in waypoint file %d"), num);
  } else {
    LogStartUp(_T("No waypoint file %d"), num);
  }

  return false;
}

bool
WaypointGlue::LoadMapFileWaypoints(int num, const TCHAR* key,
                                   Waypoints &way_points,
                                   const RasterTerrain *terrain,
                                   OperationEnvironment &operation)
{
  TCHAR path[MAX_PATH];

  // Get the map filename
  if (!Profile::GetPath(key, path))
    /* no map file configured */
    return true;

  TCHAR *tail = path + _tcslen(path);

  _tcscpy(tail, _T("/waypoints.xcw"));

  WaypointReader reader(path, num);

  // Test if waypoints.xcw can be loaded, otherwise try waypoints.cup
  if (reader.Error()) {
    // Get the map filename
    _tcscpy(tail, _T("/waypoints.cup"));
    reader.Open(path, num);
  }

  // If waypoint file inside map file exists
  if (!reader.Error()) {
    // parse the file
    reader.SetTerrain(terrain);
    if (reader.Parse(way_points, operation))
      return true;

    LogStartUp(_T("Parse error in map waypoint file"));
  } else {
    LogStartUp(_T("No waypoint file in the map file"));
  }

  return false;
}

bool
WaypointGlue::LoadWaypoints(Waypoints &way_points,
                            const RasterTerrain *terrain,
                            OperationEnvironment &operation)
{
  LogStartUp(_T("ReadWaypoints"));
  operation.SetText(_("Loading Waypoints..."));

  bool found = false;

  // Delete old waypoints
  way_points.Clear();

  // ### FIRST FILE ###
  found |= LoadWaypointFile(1, way_points, terrain, operation);

  // ### SECOND FILE ###
  found |= LoadWaypointFile(2, way_points, terrain, operation);

  // ### WATCHED WAYPOINT/THIRD FILE ###
  found |= LoadWaypointFile(3, way_points, terrain, operation);

  // ### MAP/FOURTH FILE ###

  // If no waypoint file found yet
  if (!found)
    found = LoadMapFileWaypoints(0, szProfileMapFile, way_points, terrain,
                                 operation);

  // Optimise the waypoint list after attaching new waypoints
  way_points.Optimise();

  // Return whether waypoints have been loaded into the waypoint list
  return found;
}

bool
WaypointGlue::SaveWaypointFile(const Waypoints &way_points, int num)
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
WaypointGlue::SaveWaypoints(const Waypoints &way_points)
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
