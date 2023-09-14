// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WaypointFileType.hpp"

#include "WaypointReaderFS.hpp"
#include "WaypointReaderOzi.hpp"
#include "WaypointReaderCompeGPS.hpp"
#include "system/Path.hpp"
#include "io/FileLineReader.hpp"

#include <stdexcept>

template<class R>
[[gnu::pure]]
static bool
VerifyFormat(Path path) noexcept
try {
  FileLineReader reader(path, Charset::UTF8);
  return R::VerifyFormat(reader);
} catch (const std::runtime_error &) {
  return false;
}

WaypointFileType
DetermineWaypointFileType(Path path) noexcept
{
  // If WinPilot waypoint file -> save type and return true
  if (path.EndsWithIgnoreCase(_T(".dat")) ||
      path.EndsWithIgnoreCase(_T(".xcw")))
    return WaypointFileType::WINPILOT;

  // If SeeYou waypoint file -> save type and return true
  if (path.EndsWithIgnoreCase(_T(".cup")))
    return WaypointFileType::SEEYOU;

  // If Zander waypoint file -> save type and return true
  if (path.EndsWithIgnoreCase(_T(".wpz")))
    return WaypointFileType::ZANDER;

  // If FS waypoint file -> save type and return true
  if (path.EndsWithIgnoreCase(_T(".wpt"))) {
    if (VerifyFormat<WaypointReaderFS>(path))
      return WaypointFileType::FS;

    if (VerifyFormat<WaypointReaderOzi>(path))
      return WaypointFileType::OZI_EXPLORER;

    if (VerifyFormat<WaypointReaderCompeGPS>(path))
      return WaypointFileType::COMPE_GPS;
  }

  return WaypointFileType::UNKNOWN;
}
