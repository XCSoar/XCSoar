// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ConfiguredFile.hpp"
#include "MapFile.hpp"
#include "FileReader.hxx"
#include "FileLineReader.hpp"
#include "ZipArchive.hpp"
#include "ZipLineReader.hpp"
#include "ConvertLineReader.hpp"
#include "Profile/Profile.hpp"
#include "system/Path.hpp"

#include <zzip/zzip.h>

#include <cassert>

std::unique_ptr<FileReader>
OpenConfiguredFile(std::string_view profile_key)
{
  const auto path = Profile::GetPath(profile_key);
  if (path == nullptr)
    return nullptr;

  return std::make_unique<FileReader>(path);
}

std::unique_ptr<NLineReader>
OpenConfiguredTextFileA(std::string_view profile_key)
{
  const auto path = Profile::GetPath(profile_key);
  if (path == nullptr)
    return nullptr;

  return std::make_unique<FileLineReaderA>(path);
}

std::unique_ptr<TLineReader>
OpenConfiguredTextFile(std::string_view profile_key, Charset cs)
{
  auto reader = OpenConfiguredTextFileA(profile_key);
  if (!reader)
    return nullptr;

  return std::make_unique<ConvertLineReader>(std::move(reader), cs);
}

static std::unique_ptr<NLineReader>
OpenMapTextFileA(const char *in_map_file)
{
  assert(in_map_file != nullptr);

  auto archive = OpenMapFile();
  if (!archive)
    return nullptr;

  return std::make_unique<ZipLineReaderA>(archive->get(), in_map_file);
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
OpenConfiguredTextFile(std::string_view profile_key, const char *in_map_file,
                       Charset cs)
{
  assert(in_map_file != nullptr);

  auto reader = OpenConfiguredTextFile(profile_key, cs);
  if (!reader)
    reader = OpenMapTextFile(in_map_file, cs);

  return reader;
}
