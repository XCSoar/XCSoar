// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Map.hpp"

void
ProfileMap::Set(const char *key, const char *value) noexcept
{
  auto i = map.try_emplace(key, value);
  if (!i.second) {
    /* exists already */

    if (i.first->second.compare(value) == 0)
      /* not modified, don't set the "modified" flag */
      return;

    i.first->second.assign(value);
  }

  SetModified();
}
