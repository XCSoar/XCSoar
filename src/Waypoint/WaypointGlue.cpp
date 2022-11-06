/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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
#include "system/Path.hpp"
#include "io/MapFile.hpp"
#include "io/ZipArchive.hpp"

namespace WaypointGlue {

static bool
LoadWaypointFile(Waypoints &waypoints, Path path,
                 WaypointFileType file_type,
                 WaypointOrigin origin,
                 const RasterTerrain *terrain,
                 ProgressListener &progress) noexcept
try {
  ReadWaypointFile(path, file_type, waypoints,
                   WaypointFactory(origin, terrain),
                   progress);
  return true;
} catch (...) {
  LogFormat(_T("Failed to read waypoint file: %s"), path.c_str());
  LogError(std::current_exception());
  return false;
}

static bool
LoadWaypointFile(Waypoints &waypoints, Path path,
                 WaypointOrigin origin,
                 const RasterTerrain *terrain,
                 ProgressListener &progress) noexcept
try {
  ReadWaypointFile(path, waypoints,
                   WaypointFactory(origin, terrain),
                   progress);
  return true;
} catch (...) {
  LogFormat(_T("Failed to read waypoint file: %s"), path.c_str());
  LogError(std::current_exception());
  return false;
}

static bool
LoadWaypointFile(Waypoints &waypoints, struct zzip_dir *dir, const char *path,
                 WaypointFileType file_type,
                 WaypointOrigin origin,
                 const RasterTerrain *terrain,
                 ProgressListener &progressg) noexcept
try {
  ReadWaypointFile(dir, path, file_type, waypoints,
                   WaypointFactory(origin, terrain),
                   progressg);
  return true;
} catch (...) {
  LogFormat(_T("Failed to read waypoint file: %s"), path);
  LogError(std::current_exception());
  return false;
}

bool
LoadWaypoints(Waypoints &way_points, const RasterTerrain *terrain,
              ProgressListener &progress)
{
  bool found = false;

  // Delete old waypoints
  way_points.Clear();

  LoadWaypointFile(way_points, LocalPath(_T("user.cup")),
                   WaypointFileType::SEEYOU,
                   WaypointOrigin::USER, terrain, progress);

  // ### FIRST FILE ###
  auto path = Profile::GetPath(ProfileKeys::WaypointFile);
  if (path != nullptr)
    found |= LoadWaypointFile(way_points, path, WaypointOrigin::PRIMARY,
                              terrain, progress);

  // ### SECOND FILE ###
  path = Profile::GetPath(ProfileKeys::AdditionalWaypointFile);
  if (path != nullptr)
    found |= LoadWaypointFile(way_points, path, WaypointOrigin::ADDITIONAL,
                              terrain, progress);

  // ### WATCHED WAYPOINT/THIRD FILE ###
  path = Profile::GetPath(ProfileKeys::WatchedWaypointFile);
  if (path != nullptr)
    found |= LoadWaypointFile(way_points, path, WaypointOrigin::WATCHED,
                              terrain, progress);

  // ### MAP/FOURTH FILE ###

  // If no waypoint file found yet
  if (!found) {
    try {
      if (auto archive = OpenMapFile()) {
        found |= LoadWaypointFile(way_points, archive->get(), "waypoints.xcw",
                                  WaypointFileType::WINPILOT,
                                  WaypointOrigin::MAP,
                                  terrain, progress);

        found |= LoadWaypointFile(way_points, archive->get(), "waypoints.cup",
                                  WaypointFileType::SEEYOU,
                                  WaypointOrigin::MAP,
                                  terrain, progress);
      }
    } catch (...) {
      LogError(std::current_exception(),
               "Failed to load waypoints from map file");
    }
  }

  // Optimise the waypoint list after attaching new waypoints
  way_points.Optimise();

  // Return whether waypoints have been loaded into the waypoint list
  return found;
}

} // namespace WaypointGlue
