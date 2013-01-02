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

#include "WaypointGlue.hpp"
#include "ComputerSettings.hpp"
#include "Profile/Profile.hpp"
#include "Util/StringUtil.hpp"
#include "LogFile.hpp"
#include "Waypoint/Waypoints.hpp"
#include "WaypointReader.hpp"
#include "Language/Language.hpp"
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
    key = ProfileKeys::WaypointFile;
    break;
  case 2:
    key = ProfileKeys::AdditionalWaypointFile;
    break;
  case 3:
    key = ProfileKeys::WatchedWaypointFile;
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

static bool
LoadWaypointFile(Waypoints &waypoints, const TCHAR *path, int file_num,
                 const RasterTerrain *terrain, OperationEnvironment &operation)
{
  WaypointReader reader(path, file_num);
  if (reader.Error()) {
    LogStartUp(_T("Failed to open waypoint file: %s"), path);
    return false;
  }

  // parse the file
  reader.SetTerrain(terrain);
  if (!reader.Parse(waypoints, operation)) {
    LogStartUp(_T("Failed to parse waypoint file: %s"), path);
    return false;
  }

  return true;
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

  TCHAR path[MAX_PATH];

  // ### FIRST FILE ###
  if (Profile::GetPath(ProfileKeys::WaypointFile, path))
    found |= LoadWaypointFile(way_points, path, 1, terrain, operation);

  // ### SECOND FILE ###
  if (Profile::GetPath(ProfileKeys::AdditionalWaypointFile, path))
    found |= LoadWaypointFile(way_points, path, 2, terrain, operation);

  // ### WATCHED WAYPOINT/THIRD FILE ###
  if (Profile::GetPath(ProfileKeys::WatchedWaypointFile, path))
    found |= LoadWaypointFile(way_points, path, 3, terrain, operation);

  // ### MAP/FOURTH FILE ###

  // If no waypoint file found yet
  if (!found && Profile::GetPath(ProfileKeys::MapFile, path)) {
    TCHAR *tail = path + _tcslen(path);

    _tcscpy(tail, _T("/waypoints.xcw"));
    found |= LoadWaypointFile(way_points, path, 0, terrain, operation);

    _tcscpy(tail, _T("/waypoints.cup"));
    found |= LoadWaypointFile(way_points, path, 0, terrain, operation);
  }

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
  if (!writer.IsOpen()) {
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
