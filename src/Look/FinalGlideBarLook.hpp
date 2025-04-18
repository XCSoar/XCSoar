// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Pen.hpp"
#include "ui/canvas/Brush.hpp"

class Font;

struct FinalGlideBarLook {
  Pen pen_above;
  Brush brush_above;
  Brush brush_above_mc0;

  Pen pen_below;
  Brush brush_below;
  Brush brush_below_mc0;

  Pen pen_below_landable;
  Brush brush_below_landable;
  Brush brush_below_landable_mc0;

  const Font *font;

  void Initialise(const Font &font);
};
