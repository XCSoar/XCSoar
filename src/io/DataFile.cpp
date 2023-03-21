// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DataFile.hpp"
#include "FileReader.hxx"
#include "FileLineReader.hpp"
#include "ConvertLineReader.hpp"
#include "LocalPath.hpp"
#include "system/Path.hpp"
#include "util/StringCompare.hxx"

#include <cassert>

std::unique_ptr<Reader>
OpenDataFile(const TCHAR *name)
{
  assert(name != nullptr);
  assert(!StringIsEmpty(name));

  const auto path = LocalPath(name);
  return std::make_unique<FileReader>(path);
}

std::unique_ptr<TLineReader>
OpenDataTextFile(const TCHAR *name, Charset cs)
{
  return std::make_unique<ConvertLineReader>(OpenDataTextFileA(name), cs);
}

std::unique_ptr<NLineReader>
OpenDataTextFileA(const TCHAR *name)
{
  assert(name != nullptr);
  assert(!StringIsEmpty(name));

  const auto path = LocalPath(name);
  return std::make_unique<FileLineReaderA>(path);
}
