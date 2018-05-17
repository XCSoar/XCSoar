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

#include "WaypointFileType.hpp"

#include "WaypointReaderFS.hpp"
#include "WaypointReaderOzi.hpp"
#include "WaypointReaderCompeGPS.hpp"
#include "OS/Path.hpp"
#include "IO/FileLineReader.hpp"

#include <stdexcept>

template<class R>
gcc_pure
static bool
VerifyFormat(Path path)
try {
  FileLineReader reader(path, Charset::UTF8);
  return R::VerifyFormat(reader);
} catch (const std::runtime_error &) {
  return false;
}

WaypointFileType
DetermineWaypointFileType(Path path)
{
  // If WinPilot waypoint file -> save type and return true
  if (path.MatchesExtension(_T(".dat")) ||
      path.MatchesExtension(_T(".xcw")))
    return WaypointFileType::WINPILOT;

  // If SeeYou waypoint file -> save type and return true
  if (path.MatchesExtension(_T(".cup")))
    return WaypointFileType::SEEYOU;

  // If Zander waypoint file -> save type and return true
  if (path.MatchesExtension(_T(".wpz")))
    return WaypointFileType::ZANDER;

  // If FS waypoint file -> save type and return true
  if (path.MatchesExtension(_T(".wpt"))) {
    if (VerifyFormat<WaypointReaderFS>(path))
      return WaypointFileType::FS;

    if (VerifyFormat<WaypointReaderOzi>(path))
      return WaypointFileType::OZI_EXPLORER;

    if (VerifyFormat<WaypointReaderCompeGPS>(path))
      return WaypointFileType::COMPE_GPS;
  }

  return WaypointFileType::UNKNOWN;
}
