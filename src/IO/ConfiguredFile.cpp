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

#include "ConfiguredFile.hpp"
#include "MapFile.hpp"
#include "FileLineReader.hpp"
#include "ZipArchive.hpp"
#include "ZipLineReader.hpp"
#include "ConvertLineReader.hpp"
#include "Profile/Profile.hpp"
#include "LogFile.hpp"
#include "OS/Path.hpp"

#include <zzip/zzip.h>

#include <assert.h>
#include <string.h>

std::unique_ptr<NLineReader>
OpenConfiguredTextFileA(const char *profile_key)
try {
  assert(profile_key != nullptr);

  const auto path = Profile::GetPath(profile_key);
  if (path.IsNull())
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
