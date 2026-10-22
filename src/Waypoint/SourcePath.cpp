// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SourcePath.hpp"
#include "Engine/Waypoint/Origin.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "Profile/Current.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"

#include <string_view>

static std::string_view
OriginToProfileKey(WaypointOrigin origin) noexcept
{
  switch (origin) {
  case WaypointOrigin::PRIMARY:
    return ProfileKeys::WaypointFileList;
  case WaypointOrigin::WATCHED:
    return ProfileKeys::WatchedWaypointFileList;
  case WaypointOrigin::MAP:
    return ProfileKeys::MapFile;
  default:
    return {};
  }
}

AllocatedPath
GetWaypointSourcePath(const Waypoint &waypoint) noexcept
{
  const auto key = OriginToProfileKey(waypoint.origin);
  if (key.empty())
    return {};

  auto paths = Profile::GetMultiplePaths(key, nullptr);
  if (waypoint.file_num < paths.size())
    return std::move(paths[waypoint.file_num]);

  return {};
}
