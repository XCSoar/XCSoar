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
#include "io/ZipArchive.hpp"
#include "lib/fmt/PathFormatter.hpp"
#include "system/Path.hpp"

namespace WaypointGlue {

static bool
LoadWaypointFile(Waypoints &waypoints, Path path,
                 WaypointFileType file_type,
                 WaypointOrigin origin,
                 uint8_t file_num,
                 const RasterTerrain *terrain,
                 ProgressListener &progress) noexcept
try {
  ReadWaypointFile(path, file_type, waypoints,
                   WaypointFactory(origin, file_num, terrain),
                   progress);
  return true;
} catch (...) {
  LogFmt("Failed to read waypoint file: {}", path);
  LogError(std::current_exception());
  return false;
}

static bool
LoadWaypointFile(Waypoints &waypoints, Path path,
                 WaypointOrigin origin,
                 uint8_t file_num,
                 const RasterTerrain *terrain,
                 ProgressListener &progress) noexcept
try {
  ReadWaypointFile(path, waypoints,
                   WaypointFactory(origin, file_num, terrain),
                   progress);
  return true;
} catch (...) {
  LogFmt("Failed to read waypoint file: {}", path);
  LogError(std::current_exception());
  return false;
}

static bool
LoadWaypointFile(Waypoints &waypoints, struct zzip_dir *dir, const char *path,
                 WaypointFileType file_type,
                 WaypointOrigin origin,
                 uint8_t file_num,
                 const RasterTerrain *terrain,
                 ProgressListener &progressg) noexcept
try {
  ReadWaypointFile(dir, path, file_type, waypoints,
                   WaypointFactory(origin, file_num, terrain),
                   progressg);
  return true;
} catch (...) {
  LogFmt("Failed to read waypoint file: {}", path);
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
  uint8_t file_num = 0;
  for (const auto &path : paths) {
    found |= LoadWaypointFile(way_points, path, WaypointOrigin::PRIMARY,
                              file_num++, terrain, progress);
  }

  // ### WATCHED WAYPOINT/THIRD FILE ###
  paths = Profile::GetMultiplePaths(ProfileKeys::WatchedWaypointFileList,
                                    WAYPOINT_FILE_PATTERNS);
  file_num = 0;
  for (const auto &path : paths) {
    found |= LoadWaypointFile(way_points, path, WaypointOrigin::WATCHED,
                              file_num++, terrain, progress);
  }

  // ### MAP/FOURTH FILE ###

  // If no waypoint file found yet
  if (!found) {
    try {
      if (auto archive = OpenMapFile()) {
        found |= LoadWaypointFile(way_points, archive->get(), "waypoints.xcw",
                                  WaypointFileType::WINPILOT,
                                  WaypointOrigin::MAP,
                                  0, terrain, progress);

        found |= LoadWaypointFile(way_points, archive->get(), "waypoints.cup",
                                  WaypointFileType::SEEYOU,
                                  WaypointOrigin::MAP,
                                  0, terrain, progress);
      }
    } catch (...) {
      LogError(std::current_exception(),
               "Failed to load waypoints from map file");
    }
  }
  //Load user.cup
  LoadWaypointFile(way_points, LocalPath("user.cup"),
                   WaypointFileType::SEEYOU,
                   WaypointOrigin::USER, 0, terrain, progress);
  // Optimise the waypoint list after attaching new waypoints
  way_points.Optimise();

  LogFmt("LoadWaypoints: loaded {} waypoints", way_points.size());

  // Return whether waypoints have been loaded into the waypoint list
  return found;
}

} // namespace WaypointGlue
