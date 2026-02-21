// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RaspStyle.hpp"
#include "util/StringAPI.hxx"

const RaspStyle &
LookupWeatherTerrainStyle(const char *name)
{
  if (name == nullptr)
    name = "";

  const auto *i = rasp_styles;
  while (i->name != nullptr && !StringIsEqual(i->name, name))
    ++i;

  if(i->name == nullptr) {
    // If no exact match, try matching the short codes
    // to the end of the field name
    if(StringLength(name) >= 3) {
      const char *short_name =
        &name[StringLength(name) - 3];
      i = rasp_colormaps_general;
      while (i->name != nullptr
             && !StringIsEqual(i->name, short_name))
        ++i;
    }
  }
  return *i;
}
