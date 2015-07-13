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

#include "WaypointGlue.hpp"
#include "Profile/Profile.hpp"
#include "LogFile.hpp"
#include "Waypoint/Waypoints.hpp"
#include "WaypointReader.hpp"
#include "Language/Language.hpp"
#include "IO/TextWriter.hpp"
#include "OS/PathName.hpp"
#include "Waypoint/WaypointWriter.hpp"
#include "Operation/Operation.hpp"
#include "WaypointFileType.hpp"

#include <windef.h> /* for MAX_PATH */

namespace WaypointGlue {
  static bool GetPath(WaypointOrigin origin, TCHAR *value);

  gcc_pure
  static bool IsWritable(WaypointOrigin origin);
}

bool
WaypointGlue::GetPath(WaypointOrigin origin, TCHAR *value)
{
  const char *key;

  switch (origin) {
  case WaypointOrigin::PRIMARY:
    key = ProfileKeys::WaypointFile;
    break;

  case WaypointOrigin::ADDITIONAL:
    key = ProfileKeys::AdditionalWaypointFile;
    break;

  case WaypointOrigin::WATCHED:
    key = ProfileKeys::WatchedWaypointFile;
    break;

  default:
    return false;
  }

  return Profile::GetPath(key, value);
}

bool
WaypointGlue::IsWritable(WaypointOrigin origin)
{
  TCHAR file[MAX_PATH];
  if (!GetPath(origin, file))
    return false;

  return (MatchesExtension(file, _T(".dat")) ||
          MatchesExtension(file, _T(".cup")) ||
          MatchesExtension(file, _T(".xcw")));
}

bool
WaypointGlue::IsWritable()
{
  return IsWritable(WaypointOrigin::PRIMARY) ||
    IsWritable(WaypointOrigin::ADDITIONAL) ||
    IsWritable(WaypointOrigin::WATCHED);
}

static bool
LoadWaypointFile(Waypoints &waypoints, const TCHAR *path,
                 WaypointOrigin origin,
                 const RasterTerrain *terrain, OperationEnvironment &operation)
{
  WaypointReader reader(path, WaypointFactory(origin, terrain));
  if (reader.Error()) {
    LogFormat(_T("Failed to open waypoint file: %s"), path);
    return false;
  }

  // parse the file
  if (!reader.Parse(waypoints, operation)) {
    LogFormat(_T("Failed to parse waypoint file: %s"), path);
    return false;
  }

  return true;
}

bool
WaypointGlue::LoadWaypoints(Waypoints &way_points,
                            const RasterTerrain *terrain,
                            OperationEnvironment &operation)
{
  LogFormat("ReadWaypoints");
  operation.SetText(_("Loading Waypoints..."));

  bool found = false;

  // Delete old waypoints
  way_points.Clear();

  TCHAR path[MAX_PATH];

  // ### FIRST FILE ###
  if (Profile::GetPath(ProfileKeys::WaypointFile, path))
    found |= LoadWaypointFile(way_points, path, WaypointOrigin::PRIMARY,
                              terrain, operation);

  // ### SECOND FILE ###
  if (Profile::GetPath(ProfileKeys::AdditionalWaypointFile, path))
    found |= LoadWaypointFile(way_points, path, WaypointOrigin::ADDITIONAL,
                              terrain, operation);

  // ### WATCHED WAYPOINT/THIRD FILE ###
  if (Profile::GetPath(ProfileKeys::WatchedWaypointFile, path))
    found |= LoadWaypointFile(way_points, path, WaypointOrigin::WATCHED,
                              terrain, operation);

  // ### MAP/FOURTH FILE ###

  // If no waypoint file found yet
  if (!found && Profile::GetPath(ProfileKeys::MapFile, path)) {
    TCHAR *tail = path + _tcslen(path);

    _tcscpy(tail, _T("/waypoints.xcw"));
    found |= LoadWaypointFile(way_points, path, WaypointOrigin::NONE,
                              terrain, operation);

    _tcscpy(tail, _T("/waypoints.cup"));
    found |= LoadWaypointFile(way_points, path, WaypointOrigin::NONE,
                              terrain, operation);
  }

  // Optimise the waypoint list after attaching new waypoints
  way_points.Optimise();

  // Return whether waypoints have been loaded into the waypoint list
  return found;
}

bool
WaypointGlue::SaveWaypointFile(const Waypoints &way_points,
                               WaypointOrigin origin)
{
  if (!IsWritable(origin)) {
    LogFormat("Waypoint file %d can not be written", int(origin));
    return false;
  }

  TCHAR file[255];
  GetPath(origin, file);

  TextWriter writer(file);
  if (!writer.IsOpen()) {
    LogFormat("Waypoint file %d can not be written", int(origin));
    return false;
  }

  WaypointWriter wp_writer(way_points, origin);
  wp_writer.Save(writer, DetermineWaypointFileType(file));

  LogFormat("Waypoint file %d saved", int(origin));
  return true;
}

bool
WaypointGlue::SaveWaypoints(const Waypoints &way_points)
{
  bool result = false;

  // ### FIRST FILE ###
  result |= SaveWaypointFile(way_points, WaypointOrigin::PRIMARY);

  // ### SECOND FILE ###
  result |= SaveWaypointFile(way_points, WaypointOrigin::ADDITIONAL);

  // ### THIRD FILE ###
  result |= SaveWaypointFile(way_points, WaypointOrigin::WATCHED);

  return result;
}
