// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "LocalPath.hpp"
#include "system/Path.hpp"

AllocatedPath
LocalPath(Path file) noexcept
{
  return AllocatedPath(file);
}

AllocatedPath
LocalPath(const char *file) noexcept
{
  return file != nullptr ? AllocatedPath(file) : nullptr;
}

Path
RelativePath(Path path) noexcept
{
  return path;
}

Path
GetCachePath() noexcept
{
  return Path("cache");
}

AllocatedPath
MakeCacheDirectory(const char *name) noexcept
{
  return name != nullptr
    ? AllocatedPath::Build(Path("cache"), Path(name))
    : AllocatedPath("cache");
}

void
VisitDataFiles([[maybe_unused]] const char *filter,
               [[maybe_unused]] File::Visitor &visitor)
{
}
