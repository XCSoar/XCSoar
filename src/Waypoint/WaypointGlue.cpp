// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WaypointGlue.hpp"
#include "Factory.hpp"
#include "Language/Language.hpp"
#include "LocalPath.hpp"
#include "LogFile.hpp"
#include "Operation/Operation.hpp"
#include "Patterns.hpp"
#include "Profile/Profile.hpp"
#include "Waypoint/Waypoints.hpp"
#include "WaypointFileType.hpp"
#include "WaypointReader.hpp"
#include "io/MapFile.hpp"
#include "io/WaypointDataFile.hpp"
#include "io/ZipArchive.hpp"
#include "system/Path.hpp"

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

  // ### FIRST FILE ###
  auto paths = Profile::GetMultiplePaths(ProfileKeys::WaypointFileList,
                                         WAYPOINT_FILE_PATTERNS);
  for (const auto &path : paths) {
    found |= LoadWaypointFile(way_points, path, WaypointOrigin::PRIMARY,
                              terrain, progress);
  }

  // ### WATCHED WAYPOINT/THIRD FILE ###
  paths = Profile::GetMultiplePaths(ProfileKeys::WatchedWaypointFileList,
                                    WAYPOINT_FILE_PATTERNS);
  for (const auto &path : paths) {
    found |= LoadWaypointFile(way_points, path, WaypointOrigin::WATCHED,
                              terrain, progress);
  }

  // ### WAYPOINT DATA ARCHIVES ###
  paths = Profile::GetMultiplePaths(ProfileKeys::WaypointDataFileList,
                                    _T("*.xcd\0"));
  for (const auto &path : paths) {
    try {
      if (auto archive = OpenWaypointDataFile(path)) {
        found |= LoadWaypointFile(way_points, archive->get(), "waypoints.cup",
                                  WaypointFileType::SEEYOU,
                                  WaypointOrigin::PRIMARY,
                                  terrain, progress);

        found |= LoadWaypointFile(way_points, archive->get(), "waypoints.dat",
                                  WaypointFileType::WINPILOT,
                                  WaypointOrigin::PRIMARY,
                                  terrain, progress);

        found |= LoadWaypointFile(way_points, archive->get(), "waypoints.wpz",
                                  WaypointFileType::ZANDER,
                                  WaypointOrigin::PRIMARY,
                                  terrain, progress);
      }
    } catch (...) {
      LogError(std::current_exception(),
               "Failed to load waypoints from waypoint data file");
    }
  }

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
  //Load user.cup
  LoadWaypointFile(way_points, LocalPath(_T("user.cup")),
                   WaypointFileType::SEEYOU,
                   WaypointOrigin::USER, terrain, progress);
  // Optimise the waypoint list after attaching new waypoints
  way_points.Optimise();

  // Return whether waypoints have been loaded into the waypoint list
  return found;
}

} // namespace WaypointGlue
