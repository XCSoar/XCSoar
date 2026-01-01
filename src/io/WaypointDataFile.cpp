// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WaypointDataFile.hpp"
#include "ZipArchive.hpp"
#include "ZipReader.hpp"
#include "system/ConvertPathName.hpp"
#include "system/Path.hpp"

std::optional<ZipArchive>
OpenWaypointDataFile(Path path)
{
  if (path == nullptr)
    return std::nullopt;

  return ZipArchive{path};
}

std::optional<ZipReader>
OpenInWaypointDataFile(Path archive_path, const char *filename)
{
  auto archive = OpenWaypointDataFile(archive_path);
  if (!archive)
    return std::nullopt;

  return std::optional<ZipReader>{std::in_place, archive->get(), filename};
}
