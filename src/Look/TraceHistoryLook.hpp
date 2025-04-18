// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Pen.hpp"
#include "ui/canvas/Brush.hpp"

struct TraceHistoryLook {
  bool inverse;

  Pen axis_pen;

  Pen line_pen;

  void Initialise(bool inverse);
};
