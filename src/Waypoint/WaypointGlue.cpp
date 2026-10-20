// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DataFilePath.hpp"
#include "WaypointGlue.hpp"
#include "Factory.hpp"
#include "Language/Language.hpp"
#include "LogFile.hpp"
#include "Operation/Operation.hpp"
#include "Profile/Profile.hpp"
#include "Repository/FileType.hpp"
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

static bool
LoadPrimaryFiles(Waypoints &way_points, const RasterTerrain *terrain,
                 ProgressListener &progress) noexcept
{
  bool found = false;
  uint8_t file_num = 0;
  const auto paths = Profile::GetMultiplePaths(
      ProfileKeys::WaypointFileList,
      GetFileTypePatterns(FileType::WAYPOINT));
  for (const auto &path : paths)
    found |= LoadWaypointFile(way_points, path, WaypointOrigin::PRIMARY,
                              file_num++, terrain, progress);
  return found;
}

static bool
LoadWatchedFiles(Waypoints &way_points, const RasterTerrain *terrain,
                 ProgressListener &progress) noexcept
{
  bool found = false;
  uint8_t file_num = 0;
  const auto paths = Profile::GetMultiplePaths(
      ProfileKeys::WatchedWaypointFileList,
      GetFileTypePatterns(FileType::WAYPOINT));
  for (const auto &path : paths)
    found |= LoadWaypointFile(way_points, path, WaypointOrigin::WATCHED,
                              file_num++, terrain, progress);
  return found;
}

static bool
LoadMapFiles(Waypoints &way_points, const RasterTerrain *terrain,
             ProgressListener &progress) noexcept
{
  bool found = false;
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
  return found;
}

static bool
HasOrigin(const Waypoints &way_points, WaypointOrigin origin) noexcept
{
  for (const auto &wp : way_points)
    if (wp->origin == origin)
      return true;
  return false;
}

static void
UpdateMapMembers(Waypoints &way_points, const RasterTerrain *terrain,
                 ProgressListener &progress) noexcept
{
  const bool have_files =
    HasOrigin(way_points, WaypointOrigin::PRIMARY) ||
    HasOrigin(way_points, WaypointOrigin::WATCHED);
  way_points.EraseOrigin(WaypointOrigin::MAP);
  if (!have_files)
    LoadMapFiles(way_points, terrain, progress);
}

bool
LoadWaypoints(Waypoints &way_points, const RasterTerrain *terrain,
              ProgressListener &progress)
{
  way_points.Clear();

  bool found = LoadPrimaryFiles(way_points, terrain, progress);
  found |= LoadWatchedFiles(way_points, terrain, progress);
  if (!found)
    found |= LoadMapFiles(way_points, terrain, progress);

  LoadWaypointFile(way_points,
                   ResolveTypedDataFilePath(FileType::WAYPOINT, "user.cup"),
                   WaypointFileType::SEEYOU,
                   WaypointOrigin::USER, 0, terrain, progress);
  way_points.Optimise();

  LogFmt("LoadWaypoints: loaded {} waypoints", way_points.size());
  return found;
}

void
ReloadWaypointFiles(Waypoints &way_points, const RasterTerrain *terrain,
                    ProgressListener &progress,
                    bool primary, bool watched, bool map_file)
{
  if (primary) {
    way_points.EraseOrigin(WaypointOrigin::PRIMARY);
    LoadPrimaryFiles(way_points, terrain, progress);
  }

  if (watched) {
    way_points.EraseOrigin(WaypointOrigin::WATCHED);
    LoadWatchedFiles(way_points, terrain, progress);
  }

  if (primary || watched || map_file)
    UpdateMapMembers(way_points, terrain, progress);

  way_points.Optimise();
  LogFmt("ReloadWaypointFiles: {} waypoints", way_points.size());
}

} // namespace WaypointGlue
