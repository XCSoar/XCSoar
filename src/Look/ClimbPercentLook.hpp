// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Pen.hpp"
#include "ui/canvas/Brush.hpp"
#include "ui/canvas/Icon.hpp"

struct ClimbPercentLook {
  Brush brush_circling_climb;
  Brush brush_noncircling_climb;
  Brush brush_circling_descent;

  void Initialise();
};
