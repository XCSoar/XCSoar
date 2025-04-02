// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project


#ifndef WEATHER_SKYSIGHT_REGIONS_HPP
#define WEATHER_SKYSIGHT_REGIONS_HPP

#include <tchar.h>

struct SkysightRegionDetails
{
  const TCHAR *name;
  const char *id;
};

extern const SkysightRegionDetails skysight_region_defaults[];



#endif
