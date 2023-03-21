// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Pen.hpp"
#include "ui/canvas/Brush.hpp"

class Font;

struct VarioBarLook {
  Pen pen_climb;
  Brush brush_climb;
  Brush brush_climb_avg;

  Pen pen_sink;
  Brush brush_sink;
  Brush brush_sink_avg;

  Pen pen_mc;
  Brush brush_mc;

  const Font *font;

  void Initialise(const Font &font);
};
