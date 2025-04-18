// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Pen.hpp"
#include "ui/canvas/Brush.hpp"

class Font;

struct CrossSectionLook {
  /** Background color */
  Color background_color;

  /** Text color */
  Color text_color;

  Color sky_color;

  Color terrain_color;

  /** Brush used to draw the terrain polygon */
  Brush terrain_brush;

  Color sea_color;
  Brush sea_brush;

  /** Pen used to draw the grid */
  Pen grid_pen;

  Brush aircraft_brush;

  const Font *grid_font;

  void Initialise(const Font &grid_font);
};
