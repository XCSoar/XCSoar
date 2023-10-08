// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MapFile.hpp"
#include "ZipArchive.hpp"
#include "ZipReader.hpp"
#include "Profile/Profile.hpp"
#include "system/ConvertPathName.hpp"
#include "system/Path.hpp"

std::optional<ZipArchive>
OpenMapFile()
{
  auto path = Profile::GetPath(ProfileKeys::MapFile);
  if (path == nullptr)
    return std::nullopt;

  return ZipArchive{path};
}

std::optional<ZipReader>
OpenInMapFile(const char *filename)
{
  auto archive = OpenMapFile();
  if (!archive)
    return std::nullopt;

  return std::optional<ZipReader>{std::in_place, archive->get(), filename};
}
