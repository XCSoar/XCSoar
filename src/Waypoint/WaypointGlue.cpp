/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "Factory.hpp"
#include "WaypointFileType.hpp"
#include "Profile/Profile.hpp"
#include "LogFile.hpp"
#include "Waypoint/Waypoints.hpp"
#include "WaypointReader.hpp"
#include "Language/Language.hpp"
#include "LocalPath.hpp"
#include "Operation/Operation.hpp"
#include "OS/Path.hpp"
#include "IO/MapFile.hpp"
#include "IO/ZipArchive.hpp"

static bool
LoadWaypointFile(Waypoints &waypoints, Path path,
                 WaypointFileType file_type,
                 WaypointOrigin origin,
                 const RasterTerrain *terrain, OperationEnvironment &operation)
{
  if (!ReadWaypointFile(path, file_type, waypoints,
                        WaypointFactory(origin, terrain),
                        operation)) {
    LogFormat(_T("Failed to read waypoint file: %s"), path.c_str());
    return false;
  }

  return true;
}

static bool
LoadWaypointFile(Waypoints &waypoints, Path path,
                 WaypointOrigin origin,
                 const RasterTerrain *terrain, OperationEnvironment &operation)
{
  if (!ReadWaypointFile(path, waypoints,
                        WaypointFactory(origin, terrain),
                        operation)) {
    LogFormat(_T("Failed to read waypoint file: %s"), path.c_str());
    return false;
  }

  return true;
}

static bool
LoadWaypointFile(Waypoints &waypoints, struct zzip_dir *dir, const char *path,
                 WaypointFileType file_type,
                 WaypointOrigin origin,
                 const RasterTerrain *terrain, OperationEnvironment &operation)
{
  if (!ReadWaypointFile(dir, path, file_type, waypoints,
                        WaypointFactory(origin, terrain),
                        operation)) {
    LogFormat("Failed to read waypoint file: %s", path);
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

  LoadWaypointFile(way_points, LocalPath(_T("user.cup")),
                   WaypointFileType::SEEYOU,
                   WaypointOrigin::USER, terrain, operation);

  // ### FIRST FILE ###
  auto path = Profile::GetPath(ProfileKeys::WaypointFile);
  if (!path.IsNull())
    found |= LoadWaypointFile(way_points, path, WaypointOrigin::PRIMARY,
                              terrain, operation);

  // ### SECOND FILE ###
  path = Profile::GetPath(ProfileKeys::AdditionalWaypointFile);
  if (!path.IsNull())
    found |= LoadWaypointFile(way_points, path, WaypointOrigin::ADDITIONAL,
                              terrain, operation);

  // ### WATCHED WAYPOINT/THIRD FILE ###
  path = Profile::GetPath(ProfileKeys::WatchedWaypointFile);
  if (!path.IsNull())
    found |= LoadWaypointFile(way_points, path, WaypointOrigin::WATCHED,
                              terrain, operation);

  // ### MAP/FOURTH FILE ###

  // If no waypoint file found yet
  if (!found) {
    auto archive = OpenMapFile();
    if (archive) {
      found |= LoadWaypointFile(way_points, archive->get(), "waypoints.xcw",
                                WaypointFileType::WINPILOT,
                                WaypointOrigin::MAP,
                                terrain, operation);

      found |= LoadWaypointFile(way_points, archive->get(), "waypoints.cup",
                                WaypointFileType::SEEYOU,
                                WaypointOrigin::MAP,
                                terrain, operation);
    }
  }

  // Optimise the waypoint list after attaching new waypoints
  way_points.Optimise();

  // Return whether waypoints have been loaded into the waypoint list
  return found;
}
