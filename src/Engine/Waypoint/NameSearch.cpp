// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NameSearch.hpp"
#include "Waypoint.hpp"
#include "util/StringAPI.hxx"
#include "util/StringCompare.hxx"
#include "util/StringUtil.hpp"

bool
WaypointMatchesNormalisedSubstring(const Waypoint &wp,
                                   const char *needle) noexcept
{
  if (StringIsEmpty(needle))
    return true;

  char haystack[NAME_SEARCH_BUFFER_SIZE];

  if (wp.name.length() < NAME_SEARCH_BUFFER_SIZE) {
    NormalizeSearchString(haystack, wp.name);
    if (StringFind(haystack, needle) != nullptr)
      return true;
  }

  if (!wp.shortname.empty() &&
      wp.shortname.length() < NAME_SEARCH_BUFFER_SIZE) {
    NormalizeSearchString(haystack, wp.shortname);
    if (StringFind(haystack, needle) != nullptr)
      return true;
  }

  return false;
}
