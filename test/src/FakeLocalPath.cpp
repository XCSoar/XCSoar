// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "LocalPath.hpp"
#include "system/Path.hpp"

AllocatedPath
LocalPath([[maybe_unused]] Path file) noexcept
{
  return nullptr;
}

AllocatedPath
LocalPath([[maybe_unused]] const char *file) noexcept
{
  return nullptr;
}

void
VisitDataFiles([[maybe_unused]] const char *filter,
               [[maybe_unused]] File::Visitor &visitor)
{
}
