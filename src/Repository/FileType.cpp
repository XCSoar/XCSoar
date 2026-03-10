// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FileType.hpp"
#include "system/Path.hpp"
#include "Compatibility/path.h"

const char *
GetFileTypePatterns(const FileType file_type) noexcept
{
  switch (file_type) {
  case FileType::AIRSPACE:
    return "*.txt\0*.air\0*.sua\0";

  case FileType::WAYPOINT:
    return "*.dat\0*.xcw\0*.cup\0*.cupx\0*.wpz\0*.wpt\0";

  case FileType::WAYPOINTDETAILS:
    return "*.txt\0";

  case FileType::MAP:
    return "*.xcm\0*.lkm\0";

  case FileType::FLARMNET:
    return "*.fln\0";

  case FileType::FLARMDB:
    return "xcsoar-flarm.txt\0flarm-msg-data.csv\0";

  case FileType::IGC:
    return "*.igc\0";

  case FileType::NMEA:
    return "*.nmea\0";

  case FileType::RASP:
    return "*-rasp*.dat\0";

  case FileType::XCI:
    return "*.xci\0";

  case FileType::LUA:
    return "*.lua\0";

  case FileType::TASK:
    return "*.tsk\0*.cup\0*.igc\0";

  case FileType::CHECKLIST:
    return "*.xcc\0xcsoar-checklist.txt\0";

  case FileType::PROFILE:
    return "*.prf\0";

  case FileType::PLANE:
    return "*.xcp\0";

  case FileType::UNKNOWN:
  case FileType::COUNT:
    return "\0";
  }

  return "\0";
}

AllocatedPath
GetFileTypeDefaultDir(const FileType file_type)
{
  switch (file_type) {
  case FileType::AIRSPACE:
    return AllocatedPath("airspace");

  case FileType::WAYPOINT:
    return AllocatedPath("waypoints");

  case FileType::WAYPOINTDETAILS:
    return AllocatedPath::Build("waypoints", "details");

  case FileType::MAP:
    return AllocatedPath("maps");

  case FileType::FLARMDB:
  case FileType::FLARMNET:
    return AllocatedPath("flarm");

  case FileType::RASP:
    return AllocatedPath::Build("weather", "rasp");

  case FileType::TASK:
    return AllocatedPath("tasks");

  case FileType::CHECKLIST:
    return AllocatedPath("checklists");

  case FileType::IGC:
  case FileType::NMEA:
    return AllocatedPath("logs");

  case FileType::PLANE:
    return AllocatedPath("planes");

  case FileType::XCI:
    return AllocatedPath("input");

  case FileType::LUA:
    return AllocatedPath("lua");

  case FileType::PROFILE:
  case FileType::UNKNOWN:
  case FileType::COUNT:
    return nullptr;
  }

  return nullptr;
}
