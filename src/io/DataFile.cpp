// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DataFile.hpp"
#include "FileReader.hxx"
#include "LocalPath.hpp"
#include "system/Path.hpp"
#include "util/StringCompare.hxx"

#include <cassert>

std::unique_ptr<Reader>
OpenDataFile(const char *name)
{
  assert(name != nullptr);
  assert(!StringIsEmpty(name));

  const auto path = LocalPath(name);
  return std::make_unique<FileReader>(path);
}
