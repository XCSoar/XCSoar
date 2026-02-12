// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FileType.hpp"
#include "system/Path.hpp"
#include "Compatibility/path.h"

AllocatedPath
GetFileTypeDefaultDir(const FileType file_type)
{
  AllocatedPath dest_dir;
  if (file_type == FileType::RASP) {
    dest_dir = AllocatedPath::Build(_T("weather"), _T("rasp"));
  } else if (file_type == FileType::MAP) {
    dest_dir = AllocatedPath(_T("maps"));
  } else if (file_type == FileType::AIRSPACE) {
    dest_dir = AllocatedPath(_T("airspace"));
  } else if (file_type == FileType::WAYPOINT) {
    dest_dir = AllocatedPath(_T("waypoints"));
  } else {
    dest_dir = nullptr;
  }
  return dest_dir;
}
