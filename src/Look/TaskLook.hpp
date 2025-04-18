// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Pen.hpp"
#include "ui/canvas/Brush.hpp"
#include "ui/canvas/Icon.hpp"

struct TaskLook {
  Pen oz_current_pen;
  Pen oz_active_pen;
  Pen oz_inactive_pen;

  Pen leg_active_pen;
  Pen leg_inactive_pen;
  Pen arrow_active_pen;
  Pen arrow_inactive_pen;
  Pen isoline_pen;

  Pen bearing_pen;
  Pen best_cruise_track_pen;
  Brush best_cruise_track_brush;

  Pen highlight_pen;

  MaskedIcon target_icon;

  /**
   * Used by TaskProgressRenderer.
   */
  Brush hbGray, hbGreen, hbOrange, hbLightGray, hbNotReachableTerrain;

  void Initialise();
};
