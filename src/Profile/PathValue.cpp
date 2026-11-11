// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Compatibility/path.h"
#include "Current.hpp"
#include "Profile.hpp"
#include "DataFilePath.hpp"
#include "LocalPath.hpp"
#include "Map.hpp"
#include "system/Path.hpp"
#include "util/StringAPI.hxx"
#include "util/StringCompare.hxx"
#include "util/StringPointer.hxx"

#ifdef HAVE_POSIX
#include <fnmatch.h>
#endif

#include "Language/Language.hpp"
#include "util/IterableSplitString.hxx"

#include <string>

#include <windef.h> /* for MAX_PATH */

AllocatedPath
ProfileMap::GetPath(std::string_view key) const noexcept
{
  char buffer[MAX_PATH];
  if (!Get(key, std::span{buffer}))
      return nullptr;

  if (StringIsEmpty(buffer))
    return nullptr;

  return ResolveLocalDataFile(ExpandLocalPath(Path(buffer)));
}

std::vector<AllocatedPath>
ProfileMap::GetMultiplePaths(std::string_view key, const char *patterns) const
{

  std::vector<AllocatedPath> paths;
  BasicStringBuffer<char, MAX_PATH> buffer;

  if (!Get(key, buffer)) return paths;

  if (buffer.empty()) return paths;

  for (auto i : TIterableSplitString(buffer.c_str(), '|')) {

    if (i.empty()) continue;

    std::string file_string(i);

    Path path(file_string.c_str());

    size_t length;
    const char *patterns_iterator = patterns;
    if (patterns == nullptr) {
      paths.push_back(ResolveLocalDataFile(
        ExpandLocalPath(AllocatedPath(path))));
      continue;
    }
    while ((length = strlen(patterns_iterator)) > 0) {
#ifdef HAVE_POSIX
      if (!fnmatch(patterns_iterator, path.c_str(), 0))
#else
      if (StringEndsWithIgnoreCase(path.c_str(), patterns_iterator + 1))
#endif
      {
        paths.push_back(ResolveLocalDataFile(
          ExpandLocalPath(AllocatedPath(path))));
        break;
      }
      patterns_iterator += length + 1;
    }
  }

  return paths;
}

bool
ProfileMap::SetMultiplePaths(std::string_view key,
                             std::span<const Path> values) noexcept
{
  std::string joined;

  for (Path value : values) {
    const auto contracted = ContractLocalPath(value);
    if (contracted != nullptr)
      value = contracted;

    if (value.empty())
      continue;

    if (!joined.empty())
      joined.push_back('|');

    joined += value.c_str();
  }

  if (StringIsEqual(Get(key, ""), joined.c_str()))
    return false;

  Set(key, joined.c_str());
  return true;
}

bool
ProfileMap::GetPathIsEqual(std::string_view key, Path value) const noexcept
{
  const auto saved_value = GetPath(key);
  if (saved_value == nullptr)
    return false;

  return saved_value == value;
}

[[gnu::pure]]
static Path
BackslashBaseName(const char *p) noexcept
{
  if (DIR_SEPARATOR != '\\') {
    const auto *backslash = StringFindLast(p, '\\');
    if (backslash != NULL)
      p = backslash + 1;
  }

  return Path(p).GetBase();
}

StringPointer<char>
ProfileMap::GetPathBase(std::string_view key) const noexcept
{
  const auto *path = Get(key);
  if (path != nullptr)
    path = BackslashBaseName(path).c_str();

  return path;
}

bool
ProfileMap::SetPath(std::string_view key, Path value) noexcept
{
  AllocatedPath contracted = nullptr;
  const char *new_value = "";

  if (value != nullptr && !StringIsEmpty(value.c_str())) {
    contracted = ContractLocalPath(value);
    new_value = contracted != nullptr ? contracted.c_str() : value.c_str();
  }

  if (StringIsEqual(Get(key, ""), new_value))
    return false;

  Set(key, new_value);
  return true;
}

AllocatedPath
Profile::GetPath(std::string_view key) noexcept
{
  return map.GetPath(key);
}

std::vector<AllocatedPath>
Profile::GetMultiplePaths(std::string_view key, const char *patterns)
{
  return map.GetMultiplePaths(key, patterns);
}

bool
Profile::SetMultiplePaths(std::string_view key,
                          std::span<const Path> values) noexcept
{
  return map.SetMultiplePaths(key, values);
}

bool
Profile::GetPathIsEqual(std::string_view key, Path value) noexcept
{
  return map.GetPathIsEqual(key, value);
}

bool
Profile::SetPath(std::string_view key, Path value) noexcept
{
  return map.SetPath(key, value);
}
