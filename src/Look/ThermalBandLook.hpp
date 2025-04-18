// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Pen.hpp"
#include "ui/canvas/Brush.hpp"

struct ThermalBandLook {
  bool inverse;

  // elements for drawing active thermal bands
  Pen pen_active;
  Brush brush_active;

  // elements for drawing inactive thermal band
  Pen pen_inactive;
  Brush brush_inactive;

  // pens used for drawing the MC setting
  Pen white_pen, black_pen;

  // pen used for drawing the working band
  Pen working_band_pen;

  void Initialise(bool inverse, Color sky_color);
};
