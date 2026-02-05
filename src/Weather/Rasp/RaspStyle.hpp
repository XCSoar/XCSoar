// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <tchar.h>

struct ColorRamp;

struct RaspStyle {
  const char *name;
  const ColorRamp *color_ramp;
  unsigned height_scale;
  bool do_water;
};

extern const RaspStyle rasp_styles[];
