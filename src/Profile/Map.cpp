// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Map.hpp"

void
ProfileMap::Set(std::string_view key, const char *value) noexcept
{
  const auto i = map.lower_bound(key);
  if (i != map.end() && i->first == key) {
    /* exists already */

    if (i->second == value)
      /* not modified, don't set the "modified" flag */
      return;

    i->second.assign(value);
  } else {
    map.emplace_hint(i, key, value);
  }

  SetModified();
}
