// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ConfiguredFile.hpp"
#include "MapFile.hpp"
#include "FileLineReader.hpp"
#include "ZipArchive.hpp"
#include "ZipLineReader.hpp"
#include "ConvertLineReader.hpp"
#include "Profile/Profile.hpp"
#include "LogFile.hpp"
#include "system/Path.hpp"

#include <zzip/zzip.h>

#include <cassert>
#include <string.h>

std::unique_ptr<NLineReader>
OpenConfiguredTextFileA(const char *profile_key)
try {
  assert(profile_key != nullptr);

  const auto path = Profile::GetPath(profile_key);
  if (path == nullptr)
    return nullptr;

  return std::make_unique<FileLineReaderA>(path);
} catch (...) {
  LogError(std::current_exception());
  return nullptr;
}

std::unique_ptr<TLineReader>
OpenConfiguredTextFile(const char *profile_key, Charset cs)
{
  assert(profile_key != nullptr);

  auto reader = OpenConfiguredTextFileA(profile_key);
  if (!reader)
    return nullptr;

  return std::make_unique<ConvertLineReader>(std::move(reader), cs);
}

static std::unique_ptr<NLineReader>
OpenMapTextFileA(const char *in_map_file)
try {
  assert(in_map_file != nullptr);

  auto archive = OpenMapFile();
  if (!archive)
    return nullptr;

  return std::make_unique<ZipLineReaderA>(archive->get(), in_map_file);
} catch (...) {
  LogError(std::current_exception());
  return nullptr;
}

static std::unique_ptr<TLineReader>
OpenMapTextFile(const char *in_map_file, Charset cs)
{
  assert(in_map_file != nullptr);

  auto reader = OpenMapTextFileA(in_map_file);
  if (!reader)
    return nullptr;

  return std::make_unique<ConvertLineReader>(std::move(reader), cs);
}

std::unique_ptr<TLineReader>
OpenConfiguredTextFile(const char *profile_key, const char *in_map_file,
                       Charset cs)
{
  assert(profile_key != nullptr);
  assert(in_map_file != nullptr);

  auto reader = OpenConfiguredTextFile(profile_key, cs);
  if (!reader)
    reader = OpenMapTextFile(in_map_file, cs);

  return reader;
}
