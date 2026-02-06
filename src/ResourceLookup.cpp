// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ResourceLookup.hpp"
#include "Resources.hpp"
#include "util/StringAPI.hxx"

ResourceId
LookupResourceByName(const char *name) noexcept
{
  struct Entry {
    const char *name;
    ResourceId id;
  };

  static constexpr Entry entries[] = {
#include "ResourceLookup_entries.cpp"
  };

  for (const auto &e : entries)
    if (StringIsEqual(name, e.name))
      return e.id;

  return ResourceId::Null();
}
