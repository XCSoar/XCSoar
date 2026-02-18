// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "LocalPath.hpp"
#include "system/Path.hpp"

[[gnu::weak]] AllocatedPath
LocalPath([[maybe_unused]] Path file) noexcept
{
  return nullptr;
}

[[gnu::weak]] AllocatedPath
LocalPath([[maybe_unused]] const char *file) noexcept
{
  return nullptr;
}

[[gnu::weak]] void
VisitDataFiles([[maybe_unused]] const char *filter,
               [[maybe_unused]] File::Visitor &visitor)
{
}
