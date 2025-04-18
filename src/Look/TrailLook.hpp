// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Brush.hpp"
#include "ui/canvas/Pen.hpp"

struct TrailSettings;

struct TrailLook {
  static constexpr unsigned NUMSNAILCOLORS = 15;

  unsigned trail_widths[NUMSNAILCOLORS];
  Brush trail_brushes[NUMSNAILCOLORS];
  Pen trail_pens[NUMSNAILCOLORS];
  Pen scaled_trail_pens[NUMSNAILCOLORS];

  Pen trace_pen;

  void Initialise(const TrailSettings &settings);
};
