// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <tchar.h>

struct SkysightRegionDetails
{
  const TCHAR *name;
  const char *id;
};

extern const SkysightRegionDetails skysight_region_defaults[];
