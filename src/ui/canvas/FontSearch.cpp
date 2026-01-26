// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FontSearch.hpp"
#include "system/FileUtil.hpp"
#include "system/Path.hpp"

namespace FontSearch {

AllocatedPath
FindInSearchPaths(const char *const *search_paths, Path suffix) noexcept
{
  for (const char *const* i = search_paths; *i != nullptr; ++i) {
    const Path path(*i);

    auto full_path = AllocatedPath::Build(path, suffix);
    if (File::Exists(full_path))
      return full_path;
  }

  return nullptr;
}

AllocatedPath
FindFile(const char *const *search_paths, const char *const *list) noexcept
{
  for (const char *const* i = list; *i != nullptr; ++i) {
    const Path path(*i);

    if (path.IsAbsolute()) {
      if (File::Exists(path))
        return path;
    } else {
      auto result = FindInSearchPaths(search_paths, path);
      if (result != nullptr)
        return result;
    }
  }

  return nullptr;
}

} // namespace FontSearch
