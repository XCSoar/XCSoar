// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "BackupPaths.hpp"
#include "util/StringCompare.hxx"

#include <algorithm>

namespace {

constexpr std::string_view kExcludedPaths[] = {
  "*.log",
  "*.tar",
  "cache/",
};

} // namespace

bool
IsBackupExcludedPath(std::string_view path) noexcept
{
  for (const auto pattern : kExcludedPaths) {
    if (pattern.ends_with('/')) {
      /* Directory: matches the name itself and all children. */
      const auto dir = pattern.substr(0, pattern.size() - 1);
      if (path == dir || path.starts_with(pattern))
        return true;
    } else if (WildcardMatchIgnoreCase(pattern.data(), path.data())) {
      return true;
    }
  }
  return false;
}

std::string
MakeArchiveNameIfUnderRoot(Path path, Path root) noexcept
{
  if (path == nullptr)
    return {};

  const Path relative = path.RelativeTo(root);
  if (relative == nullptr)
    return {};

  std::string name = relative.c_str();
  std::replace(name.begin(), name.end(), '\\', '/');
  return name;
}
