// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DataFile.hpp"
#include "DataFilePath.hpp"
#include "FileReader.hxx"
#include "LocalPath.hpp"
#include "Repository/FileType.hpp"
#include "system/Path.hpp"
#include "util/StringCompare.hxx"

#include <cassert>

std::unique_ptr<Reader>
OpenDataFile(const char *name)
{
  assert(name != nullptr);
  assert(!StringIsEmpty(name));

  const auto path =
    ResolveLocalDataFile(LocalPath(name), ClassifyDataFilename(name));
  return std::make_unique<FileReader>(path);
}
