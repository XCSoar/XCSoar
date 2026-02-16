// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WaypointFileType.hpp"

#include "WaypointReaderFS.hpp"
#include "WaypointReaderOzi.hpp"
#include "WaypointReaderCompeGPS.hpp"
#include "system/Path.hpp"
#include "io/FileReader.hxx"

WaypointFileType
DetermineWaypointFileType(Path path) noexcept
{
  // If WinPilot waypoint file -> save type and return true
  if (path.EndsWithIgnoreCase(".dat") ||
      path.EndsWithIgnoreCase(".xcw"))
    return WaypointFileType::WINPILOT;

  // If SeeYou waypoint file -> save type and return true
  if (path.EndsWithIgnoreCase(".cup"))
    return WaypointFileType::SEEYOU;

  // If Zander waypoint file -> save type and return true
  if (path.EndsWithIgnoreCase(".wpz"))
    return WaypointFileType::ZANDER;

  // If FS waypoint file -> save type and return true
  if (path.EndsWithIgnoreCase(".wpt")) {
    try {
      FileReader r{path};
      char buffer[4096];
      const std::size_t length = r.Read(std::as_writable_bytes(std::span{buffer}));
      const std::string_view contents{buffer, length};

      if (WaypointReaderFS::VerifyFormat(contents))
        return WaypointFileType::FS;

      if (WaypointReaderOzi::VerifyFormat(contents))
        return WaypointFileType::OZI_EXPLORER;

      if (WaypointReaderCompeGPS::VerifyFormat(contents))
        return WaypointFileType::COMPE_GPS;
    } catch (...) {
    }

    return WaypointFileType::UNKNOWN;
  }

  return WaypointFileType::UNKNOWN;
}
