// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FileType.hpp"
#include "system/Path.hpp"
#include "Compatibility/path.h"

AllocatedPath
GetFileTypeDefaultDir(const FileType file_type)
{
  switch (file_type) {
  case FileType::RASP:
    return AllocatedPath::Build("weather", "rasp");

  case FileType::MAP:
    return AllocatedPath("maps");

  case FileType::AIRSPACE:
    return AllocatedPath("airspace");

  case FileType::WAYPOINT:
    return AllocatedPath("waypoints");

  case FileType::UNKNOWN:
  case FileType::WAYPOINTDETAILS:
  case FileType::FLARMNET:
  case FileType::IGC:
  case FileType::XCI:
  case FileType::TASK:
  case FileType::CHECKLIST:
    return nullptr;
  }

  return nullptr;
}
