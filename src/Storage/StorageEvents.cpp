// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Storage/StorageEvents.hpp"
#include "Language/Language.hpp"
#include "util/Compiler.h"

#include <fmt/format.h>

static std::string
join_names(const std::vector<std::shared_ptr<StorageDevice>> &devices)
{
  std::string names;
  for (size_t i = 0; i < devices.size(); ++i) {
    if (i)
      names += ", ";
    names += devices[i]->Name();
  }
  return names;
}

std::string
StorageEventInfo::Format() const
{
  if (devices.empty())
    return std::string();

  const std::string names = join_names(devices);

  switch (type) {
  case StorageEvent::Available:
    return fmt::vformat(_("Storage inserted: {}"),
                        fmt::make_format_args(names));

  case StorageEvent::Removed:
    return fmt::vformat(_("Storage removed: {}"),
                        fmt::make_format_args(names));

  case StorageEvent::AccessGranted:
    return fmt::vformat(_("Storage access granted: {}"),
                        fmt::make_format_args(names));

  case StorageEvent::COUNT:
    break;
  }

  gcc_unreachable();
}
